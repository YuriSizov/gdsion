/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_stream.h"

#include "chip/siopm_ref_table.h"

void SiOPMStream::resize(int p_length) {
	buffer.resize_zeroed(p_length);
}

void SiOPMStream::clear() {
	for (int i = 0; i < buffer.size(); i++) {
		buffer.write[i] = 0;
	}
}

void SiOPMStream::limit() {
	// Limit buffered signals between -1 and 1.
	for (int i = 0; i < buffer.size(); i++) {
		buffer.write[i] = CLAMP(buffer[i], -1, 1);
	}
}

void SiOPMStream::quantize(int p_bitrate) {
	double r = 1 << p_bitrate;
	double ir = 2.0 / r;
	for (int i = 0; i < buffer.size(); i++) {
		int n = buffer[i] * r; // Truncate the double-precision value before shifting.
		buffer.write[i] = (n >> 1) * ir;
	}
}

// Writes into the buffer are placed at an offset which the sequencer and the channels maintain across the
// frame, and either end of that arithmetic can leave the buffer if the bookkeeping is ever inconsistent: a
// negative buffer index, or a length that runs past the end of the frame. Vector's bounds check turns both
// into a trap that takes the process down, in exported builds as well, so clamp the range to what the buffer
// can hold. The worst case is then a dropped or truncated write for one channel in one frame, and channel
// buffer indices are reset at the top of the next one.
int SiOPMStream::_clamp_write_length(int p_offset, int p_length) const {
	int frames = buffer.size() >> 1;

	if (unlikely(p_offset < 0 || p_offset >= frames)) {
		WARN_PRINT_ONCE("SiOPMStream: Buffer offset is outside of the buffer, dropping the write.");
		return 0;
	}
	if (unlikely(p_length < 0)) {
		WARN_PRINT_ONCE("SiOPMStream: Write length is negative, dropping the write.");
		return 0;
	}
	if (unlikely(p_length > frames - p_offset)) {
		WARN_PRINT_ONCE("SiOPMStream: Write runs past the end of the buffer, truncating it.");
		return frames - p_offset;
	}

	return p_length;
}

void SiOPMStream::write(SinglyLinkedList<int>::Element *p_data_start, int p_offset, int p_length, double p_volume, int p_pan) {
	int length = _clamp_write_length(p_offset, p_length);
	if (length == 0) {
		return;
	}

	double volume = p_volume * SiOPMRefTable::get_instance()->i2n;
	int buffer_size = (p_offset + length) << 1;

	if (channels == 2) { // stereo
		double (&pan_table)[129] = SiOPMRefTable::get_instance()->pan_table;
		double volume_left = pan_table[128 - p_pan] * volume;
		double volume_right = pan_table[p_pan] * volume;

		SinglyLinkedList<int>::Element *current = p_data_start;
		for (int i = p_offset << 1; i < buffer_size;) {
			buffer.write[i] += current->value * volume_left;
			i++;
			buffer.write[i] += current->value * volume_right;
			i++;

			current = current->next();
		}
	} else if (channels == 1) { // mono
		SinglyLinkedList<int>::Element *current = p_data_start;
		for (int i = p_offset << 1; i < buffer_size;) {
			buffer.write[i] += current->value * volume;
			i++;
			buffer.write[i] += current->value * volume;
			i++;

			current = current->next();
		}
	}
}

void SiOPMStream::write_stereo(SinglyLinkedList<int>::Element *p_left_start, SinglyLinkedList<int>::Element *p_right_start, int p_offset, int p_length, double p_volume, int p_pan) {
	int length = _clamp_write_length(p_offset, p_length);
	if (length == 0) {
		return;
	}

	double volume = p_volume * SiOPMRefTable::get_instance()->i2n;
	int buffer_size = (p_offset + length) << 1;

	if (channels == 2) { // stereo
		double (&pan_table)[129] = SiOPMRefTable::get_instance()->pan_table;
		double volume_left = pan_table[128 - p_pan] * p_volume;
		double volume_right = pan_table[p_pan] * p_volume;

		SinglyLinkedList<int>::Element *current_left = p_left_start;
		SinglyLinkedList<int>::Element *current_right = p_right_start;

		for (int i = p_offset << 1; i < buffer_size;) {
			buffer.write[i] += current_left->value * volume_left;
			i++;
			buffer.write[i] += current_right->value * volume_right;
			i++;

			current_left = current_left->next();
			current_right = current_right->next();
		}
	} else if (channels == 1) { // mono
		volume *= 0.5;

		SinglyLinkedList<int>::Element *current_left = p_left_start;
		SinglyLinkedList<int>::Element *current_right = p_right_start;

		for (int i = p_offset << 1; i < buffer_size;) {
			buffer.write[i] += (current_left->value + current_right->value) * volume;
			i++;
			buffer.write[i] += (current_left->value + current_right->value) * volume;
			i++;

			current_left = current_left->next();
			current_right = current_right->next();
		}
	}
}

void SiOPMStream::write_from_vector(Vector<double> *p_data, int p_start_data, int p_start_buffer, int p_length, double p_volume, int p_pan, int p_sample_channel_count) {
	int length = _clamp_write_length(p_start_buffer, p_length);
	if (length == 0) {
		return;
	}

	// The sample data has a cursor of its own, which can run past the end of the data in the same way.
	int data_frames = (p_sample_channel_count == 2 ? p_data->size() >> 1 : p_data->size());
	if (unlikely(p_start_data < 0 || p_start_data >= data_frames)) {
		WARN_PRINT_ONCE("SiOPMStream: Sample offset is outside of the sample data, dropping the write.");
		return;
	}
	if (unlikely(length > data_frames - p_start_data)) {
		WARN_PRINT_ONCE("SiOPMStream: Write runs past the end of the sample data, truncating it.");
		length = data_frames - p_start_data;
	}

	double volume = p_volume;

	if (channels == 2) {
		double (&pan_table)[129] = SiOPMRefTable::get_instance()->pan_table;

		if (p_sample_channel_count == 2) { // stereo data to stereo buffer
			double volume_left = pan_table[128 - p_pan] * volume;
			double volume_right = pan_table[p_pan] * volume;
			int buffer_size = (p_start_data + length) << 1;

			for (int j = p_start_data << 1, i = p_start_buffer << 1; j < buffer_size;) {
				buffer.write[i] += (*p_data)[j] * volume_left;
				j++;
				i++;
				buffer.write[i] += (*p_data)[j] * volume_right;
				j++;
				i++;
			}
		} else { // mono data to stereo buffer
			double volume_left = pan_table[128 - p_pan] * volume * 0.707;
			double volume_right = pan_table[p_pan] * volume * 0.707;
			int buffer_size = p_start_data + length;

			for (int j = p_start_data, i = p_start_buffer << 1; j < buffer_size; j++) {
				buffer.write[i] += (*p_data)[j] * volume_left;
				i++;
				buffer.write[i] += (*p_data)[j] * volume_right;
				i++;
			}
		}
	} else if (channels == 1) {
		if (p_sample_channel_count == 2) { // stereo data to mono buffer
			volume *= 0.5;
			int buffer_size = (p_start_data + length) << 1;

			for (int j = p_start_data << 1, i = p_start_buffer << 1; j < buffer_size;) {
				buffer.write[i] += ((*p_data)[j] + (*p_data)[j + 1]) * volume;
				i++;
				buffer.write[i] += ((*p_data)[j] + (*p_data)[j + 1]) * volume;
				i++;
				j += 2;
			}
		} else { // mono data to mono buffer
			int buffer_size = p_start_data + length;

			for (int j = p_start_data, i = p_start_buffer << 1; j < buffer_size; j++) {
				buffer.write[i] += (*p_data)[j] * volume;
				i++;
				buffer.write[i] += (*p_data)[j] * volume;
				i++;
			}
		}
	}
}

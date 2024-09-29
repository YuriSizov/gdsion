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

void SiOPMStream::write(SinglyLinkedList<int> *p_data, int p_start, int p_length, double p_volume, int p_pan) {
	double volume = p_volume * SiOPMRefTable::get_instance()->i2n;
	int buffer_size = (p_start + p_length) << 1;

	if (channels == 2) { // stereo
		double (&pan_table)[129] = SiOPMRefTable::get_instance()->pan_table;
		double volume_left = pan_table[128 - p_pan] * volume;
		double volume_right = pan_table[p_pan] * volume;

		SinglyLinkedList<int> *current = p_data;
		for (int i = p_start << 1; i < buffer_size;) {
			buffer.write[i] += current->get()->value * volume_left;
			i++;
			buffer.write[i] += current->get()->value * volume_right;
			i++;

			current = current->next();
		}
	} else if (channels == 1) { // mono
		SinglyLinkedList<int> *current = p_data;
		for (int i = p_start << 1; i < buffer_size;) {
			buffer.write[i] += current->get()->value * volume;
			i++;
			buffer.write[i] += current->get()->value * volume;
			i++;

			current = current->next();
		}
	}
}

void SiOPMStream::write_stereo(SinglyLinkedList<int> *p_left, SinglyLinkedList<int> *p_right, int p_start, int p_length, double p_volume, int p_pan) {
	double volume = p_volume * SiOPMRefTable::get_instance()->i2n;
	int buffer_size = (p_start + p_length) << 1;

	if (channels == 2) { // stereo
		double (&pan_table)[129] = SiOPMRefTable::get_instance()->pan_table;
		double volume_left = pan_table[128 - p_pan] * p_volume;
		double volume_right = pan_table[p_pan] * p_volume;

		SinglyLinkedList<int> *current_left = p_left;
		SinglyLinkedList<int> *current_right = p_right;

		for (int i = p_start << 1; i < buffer_size;) {
			buffer.write[i] += current_left->get()->value * volume_left;
			i++;
			buffer.write[i] += current_right->get()->value * volume_right;
			i++;

			current_left = current_left->next();
			current_right = current_right->next();
		}
	} else if (channels == 1) { // mono
		volume *= 0.5;

		SinglyLinkedList<int> *current_left = p_left;
		SinglyLinkedList<int> *current_right = p_right;

		for (int i = p_start << 1; i < buffer_size;) {
			buffer.write[i] += (current_left->get()->value + current_right->get()->value) * volume;
			i++;
			buffer.write[i] += (current_left->get()->value + current_right->get()->value) * volume;
			i++;

			current_left = current_left->next();
			current_right = current_right->next();
		}
	}
}

void SiOPMStream::write_from_vector(Vector<double> *p_data, int p_start_data, int p_start_buffer, int p_length, double p_volume, int p_pan, int p_sample_channel_count) {
	double volume = p_volume;

	if (channels == 2) {
		double (&pan_table)[129] = SiOPMRefTable::get_instance()->pan_table;

		if (p_sample_channel_count == 2) { // stereo data to stereo buffer
			double volume_left = pan_table[128 - p_pan] * volume;
			double volume_right = pan_table[p_pan] * volume;
			int buffer_size = (p_start_data + p_length) << 1;

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
			int buffer_size = p_start_data + p_length;

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
			int buffer_size = (p_start_data + p_length) << 1;

			for (int j = p_start_data << 1, i = p_start_buffer << 1; j < buffer_size;) {
				buffer.write[i] += ((*p_data)[j] + (*p_data)[j + 1]) * volume;
				i++;
				buffer.write[i] += ((*p_data)[j] + (*p_data)[j + 1]) * volume;
				i++;
				j += 2;
			}
		} else { // mono data to mono buffer
			int buffer_size = p_start_data + p_length;

			for (int j = p_start_data, i = p_start_buffer << 1; j < buffer_size; j++) {
				buffer.write[i] += (*p_data)[j] * volume;
				i++;
				buffer.write[i] += (*p_data)[j] * volume;
				i++;
			}
		}
	}
}

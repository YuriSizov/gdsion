/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_sound_chip.h"

#include <godot_cpp/core/class_db.hpp>
#include "chip/channels/siopm_channel_manager.h"
#include "chip/siopm_operator_params.h"
#include "chip/siopm_stream.h"

Vector<double> *SiOPMSoundChip::get_output_buffer_ptr() {
	return output_stream->get_buffer_ptr();
}

int SiOPMSoundChip::get_channel_count() const {
	return output_stream->get_channel_count();
}

SinglyLinkedList<int> *SiOPMSoundChip::get_pipe(int p_pipe_num, int p_index) {
	ERR_FAIL_INDEX_V(p_pipe_num, _pipe_buffer_pager.size(), nullptr);
	ERR_FAIL_INDEX_V(p_index, _pipe_buffer_pager[p_pipe_num].size(), nullptr);

	return _pipe_buffer_pager[p_pipe_num][p_index];
}

void SiOPMSoundChip::begin_process() {
	output_stream->clear();
}

void SiOPMSoundChip::end_process() {
	// Limit output level in the range between -1 ~ 1.
	output_stream->limit();
	if (_bitrate != 0) {
		output_stream->quantize(_bitrate);
	}
}

//

void SiOPMSoundChip::initialize(int p_channel_count, int p_bitrate, int p_buffer_length) {
	_bitrate = p_bitrate;

	// Reset stream slot.
	for (int i = 0; i < STREAM_SEND_SIZE; i++) {
		stream_slot.write[i] = nullptr;
	}
	stream_slot.write[0] = output_stream;

	// Reallocate buffer.
	if (_buffer_length != p_buffer_length) {
		_buffer_length = p_buffer_length;
		output_stream->resize(_buffer_length << 1);

		for (int i = 0; i < PIPE_SIZE; i++) {
			if (_pipe_buffer[i]) {
				SinglyLinkedList<int>::free_ring(_pipe_buffer[i]);
			}

			_pipe_buffer.write[i] = SinglyLinkedList<int>::alloc_ring(_buffer_length);
			_pipe_buffer_pager.write[i] = SinglyLinkedList<int>::create_ring_pager(_pipe_buffer[i]);
		}
	}

	pcm_volume = 4;
	sampler_volume = 2;

	SiOPMChannelManager::initialize_all_channels();
}

void SiOPMSoundChip::reset() {
	SiOPMChannelManager::reset_all_channels();
}

void SiOPMSoundChip::_bind_methods() {
	BIND_CONSTANT(STREAM_SEND_SIZE);
}

SiOPMSoundChip::SiOPMSoundChip() {
	init_operator_params = memnew(SiOPMOperatorParams);

	output_stream = memnew(SiOPMStream);

	stream_slot.resize_zeroed(STREAM_SEND_SIZE);
	stream_slot.fill(nullptr);

	zero_buffer = SinglyLinkedList<int>::alloc_ring(1);
	_pipe_buffer.resize_zeroed(PIPE_SIZE);
	_pipe_buffer_pager.resize_zeroed(PIPE_SIZE);

	SiOPMChannelManager::initialize(this);
}

SiOPMSoundChip::~SiOPMSoundChip() {
	memdelete(init_operator_params);
	memdelete(output_stream);

	SinglyLinkedList<int>::free_ring(zero_buffer);
	SiOPMChannelManager::finalize();
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_module.h"

#include "processor/channels/siopm_channel_manager.h"
#include "processor/siopm_operator_params.h"
#include "processor/siopm_stream.h"

Vector<double> SiOPMModule::get_output_buffer() const {
	return output_stream->get_buffer();
}

void SiOPMModule::set_output_buffer(Vector<double> p_buffer) {
	output_stream->set_buffer(p_buffer);
}

int SiOPMModule::get_channel_count() const {
	return output_stream->get_channel_count();
}

SinglyLinkedList<int> *SiOPMModule::get_pipe(int p_pipe_num, int p_index) {
	ERR_FAIL_INDEX_V(p_pipe_num, _pipe_buffer_pager.size(), nullptr);
	ERR_FAIL_INDEX_V(p_index, _pipe_buffer_pager[p_pipe_num].size(), nullptr);

	return _pipe_buffer_pager[p_pipe_num][p_index];
}

void SiOPMModule::begin_process() {
	output_stream->clear();
}

void SiOPMModule::end_process() {
	// Limit output level in the range between -1 ~ 1.
	output_stream->limit();
	if (_bitrate != 0) {
		output_stream->quantize(_bitrate);
	}
}

//

void SiOPMModule::initialize(int p_channel_count, int p_bitrate, int p_buffer_length) {
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

void SiOPMModule::reset() {
	SiOPMChannelManager::reset_all_channels();
}

SiOPMModule::SiOPMModule() {
	init_operator_params = memnew(SiOPMOperatorParams);

	output_stream = memnew(SiOPMStream);

	stream_slot.resize_zeroed(STREAM_SEND_SIZE);
	stream_slot.fill(nullptr);

	zero_buffer = SinglyLinkedList<int>::alloc_ring(1);
	_pipe_buffer.resize_zeroed(PIPE_SIZE);
	_pipe_buffer_pager.resize_zeroed(PIPE_SIZE);

	SiOPMChannelManager::initialize(this);
}

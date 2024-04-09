/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_filter_base.h"

const double SiFilterBase::THRESHOLD = 0.0000152587890625;

int SiFilterBase::prepare_process() {
	_left.clear();
	_right.clear();

	return 2;
}

double SiFilterBase::_process_channel(ChannelValues p_channel, double p_input) {
	double output = _b0 * p_input + _b1 * p_channel.in1 + _b2 * p_channel.in2 - _a1 * p_channel.out1 - _a2 * p_channel.out2;
	output = CLAMP(output, -1, 1);

	p_channel.in2  = p_channel.in1;
	p_channel.in1  = p_input;
	p_channel.out2 = p_channel.out1;
	p_channel.out1 = output;

	return output;
}

int SiFilterBase::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	_left.check_threshold();
	_right.check_threshold();

	if (p_channels == 2) {
		for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
			r_buffer->write[i] = _process_channel(_left, (*r_buffer)[i]);
			r_buffer->write[i + 1] = _process_channel(_right, (*r_buffer)[i + 1]);
		}
	} else {
		for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
			double value = _process_channel(_left, (*r_buffer)[i]);;

			r_buffer->write[i] = value;
			r_buffer->write[i + 1] = value;
		}
	}

	return p_channels;
}

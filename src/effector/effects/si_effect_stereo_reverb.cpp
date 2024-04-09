/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_stereo_reverb.h"

void SiEffectStereoReverb::set_params(double p_delay1, double p_delay2, double p_feedback, double p_wet) {
	double delay1 = CLAMP(p_delay1, 0.01, 0.99);
	double delay2 = CLAMP(p_delay2, 0.01, 0.99);

	_pointer_write = (_pointer_read0 + DELAY_BUFFER_FILTER) & DELAY_BUFFER_FILTER;
	_pointer_read1 = (int)(_pointer_read0 + DELAY_BUFFER_FILTER * (1 - delay1)) & DELAY_BUFFER_FILTER;
	_pointer_read2 = (int)(_pointer_read0 + DELAY_BUFFER_FILTER * (1 - delay2)) & DELAY_BUFFER_FILTER;

	double feedback = CLAMP(p_feedback, -0.99, 0.99);
	_feedback0 = feedback * 0.2;
	_feedback1 = feedback * 0.3;
	_feedback2 = feedback * 0.5;

	_wet = p_wet;
}

int SiEffectStereoReverb::prepare_process() {
	_delay_buffer_left.fill(0);
	_delay_buffer_right.fill(0);

	return 2;
}

void SiEffectStereoReverb::_process_channel(Vector<double> *r_buffer, int p_buffer_index, Vector<double> *r_delay_buffer) {
	double value = (*r_delay_buffer)[_pointer_read0] * _feedback0;
	value += (*r_delay_buffer)[_pointer_read1] * _feedback1;
	value += (*r_delay_buffer)[_pointer_read2] * _feedback2;
	r_delay_buffer->write[_pointer_write] = (*r_buffer)[p_buffer_index] - value;

	r_buffer->write[p_buffer_index] *= 1 - _wet;
	r_buffer->write[p_buffer_index] += value * _wet;
}

int SiEffectStereoReverb::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		_process_channel(r_buffer, i, &_delay_buffer_left);
		_process_channel(r_buffer, i + 1, &_delay_buffer_right);

		_pointer_write = (_pointer_write + 1) & DELAY_BUFFER_FILTER;
		_pointer_read0 = (_pointer_read0 + 1) & DELAY_BUFFER_FILTER;
		_pointer_read1 = (_pointer_read1 + 1) & DELAY_BUFFER_FILTER;
		_pointer_read2 = (_pointer_read2 + 1) & DELAY_BUFFER_FILTER;
	}

	return p_channels;
}

void SiEffectStereoReverb::set_by_mml(Vector<double> p_args) {
	double delay1   = _get_mml_arg(p_args, 0, 70) / 100.0;
	double delay2   = _get_mml_arg(p_args, 1, 40) / 100.0;
	double feedback = _get_mml_arg(p_args, 2, 80) / 100.0;
	double wet      = _get_mml_arg(p_args, 3, 100) / 100.0;

	set_params(delay1, delay2, feedback, wet);
}

void SiEffectStereoReverb::reset() {
	set_params();
}

SiEffectStereoReverb::SiEffectStereoReverb(double p_delay1, double p_delay2, double p_feedback, double p_wet) :
		SiEffectBase() {
	_delay_buffer_left.resize_zeroed(1 << DELAY_BUFFER_BITS);
	_delay_buffer_right.resize_zeroed(1 << DELAY_BUFFER_BITS);

	set_params(p_delay1, p_delay2, p_feedback, p_wet);
}

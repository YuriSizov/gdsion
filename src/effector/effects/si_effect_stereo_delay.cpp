/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_stereo_delay.h"

void SiEffectStereoDelay::set_params(double p_delay_time, double p_feedback, bool p_cross, double p_wet) {
	int offset = (int)(p_delay_time * 44.1);
	if (offset > DELAY_BUFFER_FILTER) {
		offset = DELAY_BUFFER_FILTER;
	}

	_pointer_write = (_pointer_read + offset) & DELAY_BUFFER_FILTER;

	_feedback = p_feedback;
	if (_feedback >= 1) {
		_feedback = 0.9990234375;
	} else if (_feedback <= -1) {
		_feedback = -0.9990234375;
	}

	_wet = p_wet;
	_cross = p_cross;
}

int SiEffectStereoDelay::prepare_process() {
	_delay_buffer_left.fill(0);
	_delay_buffer_right.fill(0);

	return 2;
}

void SiEffectStereoDelay::_process_channel(Vector<double> *r_buffer, int p_buffer_index, Vector<double> *p_read_buffer, Vector<double> *r_write_buffer) {
	double value = (*p_read_buffer)[_pointer_read];
	r_write_buffer->write[_pointer_write] = (*r_buffer)[p_buffer_index] - value * _feedback;

	r_buffer->write[p_buffer_index] *= 1 - _wet;
	r_buffer->write[p_buffer_index] += value * _wet;
}

int SiEffectStereoDelay::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	for (int i = start_index; i < (start_index + length); i += 2) {
		_process_channel(r_buffer, i,     (_cross ? &_delay_buffer_right : &_delay_buffer_left), &_delay_buffer_left);
		_process_channel(r_buffer, i + 1, (_cross ? &_delay_buffer_left : &_delay_buffer_right), &_delay_buffer_right);

		_pointer_write = (_pointer_write + 1) & DELAY_BUFFER_FILTER;
		_pointer_read  = (_pointer_read  + 1) & DELAY_BUFFER_FILTER;
	}

	return p_channels;
}

void SiEffectStereoDelay::set_by_mml(Vector<double> p_args) {
	double delay_time = _get_mml_arg(p_args, 0, 250);
	double feedback   = _get_mml_arg(p_args, 1, 25) / 100.0;
	int cross         = _get_mml_arg(p_args, 2, 0);
	double wet        = _get_mml_arg(p_args, 3, 100) / 100.0;

	set_params(delay_time, feedback, cross == 1, wet);
}

void SiEffectStereoDelay::reset() {
	set_params();
}

void SiEffectStereoDelay::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "delay_time", "feedback", "is_cross", "wet"), &SiEffectStereoDelay::set_params, DEFVAL(250), DEFVAL(0.25), DEFVAL(false), DEFVAL(0.25));
}

SiEffectStereoDelay::SiEffectStereoDelay(double p_delay_time, double p_feedback, bool p_cross, double p_wet) :
		SiEffectBase() {
	_delay_buffer_left.resize_zeroed(1 << DELAY_BUFFER_BITS);
	_delay_buffer_right.resize_zeroed(1 << DELAY_BUFFER_BITS);

	set_params(p_delay_time, p_feedback, p_cross, p_wet);
}

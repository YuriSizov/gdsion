/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_stereo_expander.h"

void SiEffectStereoExpander::set_params(double p_stereo_width, double p_rotation, bool p_phase_invert) {
	_monoralize = (p_stereo_width == 0 && p_rotation == 0 && !p_phase_invert);

	double half_width   = p_stereo_width * 0.7853981633974483; // = pi()/4
	double center_angle = (p_rotation + 0.5) * 1.5707963267948965;
	double left_angle   = center_angle - half_width;
	double right_angle  = center_angle + half_width;
	double invert = (p_phase_invert ? -1 : 1);

	_left_to_left   = Math::cos(left_angle);
	_right_to_left  = Math::sin(left_angle);
	_left_to_right  = Math::cos(right_angle) * invert;
	_right_to_right = Math::sin(right_angle) * invert;

	double x = _left_to_left + _left_to_right;
	double y = _right_to_left + _right_to_right;
	double l = Math::sqrt(x * x + y * y);
	if (l > 0.01) {
		l = 1 / l;
		_left_to_left   *= l;
		_right_to_left  *= l;
		_left_to_right  *= l;
		_right_to_right *= l;
	}
}

int SiEffectStereoExpander::prepare_process() {
	return 2;
}

int SiEffectStereoExpander::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	if (_monoralize) {
		for (int i = start_index; i < (start_index + length); i += 2) {
			double value = (*r_buffer)[i] + (*r_buffer)[i + 1];
			value *= 0.7071067811865476;

			r_buffer->write[i] = value;
			r_buffer->write[i + 1] = value;
		}

		return 1;
	}

	for (int i = start_index; i < (start_index + length); i += 2) {
		double value_left = (*r_buffer)[i];
		double value_right = (*r_buffer)[i + 1];

		r_buffer->write[i]     = value_left * _left_to_left + value_right * _right_to_left;
		r_buffer->write[i + 1] = value_left * _left_to_right + value_right * _right_to_right;
	}

	return 2;
}

void SiEffectStereoExpander::set_by_mml(Vector<double> p_args) {
	double stereo_width = _get_mml_arg(p_args, 0, 140) / 100.0;
	double rotation     = _get_mml_arg(p_args, 1, 0) / 100.0;
	int phase_invert    = _get_mml_arg(p_args, 2, 0);

	set_params(stereo_width, rotation, phase_invert != 0);
}

void SiEffectStereoExpander::reset() {
	set_params();
}

SiEffectStereoExpander::SiEffectStereoExpander(double p_stereo_width, double p_rotation, bool p_phase_invert) :
		SiEffectBase() {
	set_params(p_stereo_width, p_rotation, p_phase_invert);
}

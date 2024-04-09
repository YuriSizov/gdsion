/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_distortion.h"

const double SiEffectDistortion::THRESHOLD = 0.0000152587890625;

void SiEffectDistortion::set_params(double p_pre_gain, double p_post_gain, double p_lpf_frequency, double p_lpf_slope) {
	_limit = Math::pow(2, -p_post_gain / 6.0);
	_pre_scale = Math::pow(2, -p_pre_gain / 6.0) * _limit;
	_filter_enabled = p_lpf_frequency > 0;

	if (_filter_enabled) {
		// TODO: Pick better names for these variables.

		double omg = p_lpf_frequency * 0.00014247585730565955; // 2*pi/44100
		double cos = Math::cos(omg);
		double sin = Math::sin(omg);

		double ang = 0.34657359027997264 * p_lpf_slope * omg / sin;
		double alp = sin * (Math::exp(ang) - Math::exp(-ang)) * 0.5; // log(2)*0.5
		double ia0 = 1 / (1 + alp);

		_a1 = -2 * cos * ia0;
		_a2 = (1 - alp) * ia0;
		_b1 = (1 - cos) * ia0;
		_b0 = _b1 * 0.5;
		_b2 = _b1 * 0.5;
	}
}

int SiEffectDistortion::prepare_process() {
	_in1 = 0;
	_in2 = 0;
	_out1 = 0;
	_out2 = 0;

	return 1;
}

int SiEffectDistortion::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	if (_out1 < THRESHOLD) {
		_out1 = 0;
		_out2 = 0;
	}

	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		double value = CLAMP((*r_buffer)[i] * _pre_scale, -_limit, _limit);

		double output = value;
		if (_filter_enabled) {
			output = _b0 * value + _b1 * _in1 + _b2 * _in2 - _a1 * _out1 - _a2 * _out2;

			_in2  = _in1;
			_in1  = value;

			_out2 = _out1;
			_out1 = output;
		}

		r_buffer->write[i] = output;
		r_buffer->write[i + 1] = output;
	}

	return 1;
}

void SiEffectDistortion::set_by_mml(Vector<double> p_args) {
	double pre_gain       = _get_mml_arg(p_args, 0, -60);
	double post_gain      = _get_mml_arg(p_args, 1, 18);
	double lpf_frequency  = _get_mml_arg(p_args, 2, 2400);
	double lpf_slope      = _get_mml_arg(p_args, 3, 1);

	set_params(pre_gain, post_gain, lpf_frequency, lpf_slope);
}

void SiEffectDistortion::reset() {
	set_params();
}

SiEffectDistortion::SiEffectDistortion(double p_pre_gain, double p_post_gain, double p_lpf_frequency, double p_lpf_slope) :
		SiEffectBase() {
	set_params(p_pre_gain, p_post_gain, p_lpf_frequency, p_lpf_slope);
}

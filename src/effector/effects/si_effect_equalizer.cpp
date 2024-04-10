/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_equalizer.h"

void SiEffectEqualizer::set_params(double p_low_gain, double p_mid_gain, double p_high_gain, double p_low_frequency, double p_high_frequency) {
	_low_gain = p_low_gain;
	_mid_gain = p_mid_gain;
	_high_gain = p_high_gain;

	_low_frequency = 2.0 * Math::sin(p_low_frequency * 0.00007123792865282977); // 3.141592653589793 / 44100
	_high_frequency = 2.0 * Math::sin(p_high_frequency * 0.00007123792865282977);
}

int SiEffectEqualizer::prepare_process() {
	_left.clear();
	_right.clear();

	return 2;
}

double SiEffectEqualizer::_process_channel(PipeChannel p_channel, double p_value) {
		p_channel.f1p0 += (_low_frequency * (p_value        - p_channel.f1p0)) + 2.3283064370807974e-10;
		p_channel.f1p1 += (_low_frequency * (p_channel.f1p0 - p_channel.f1p1));
		p_channel.f1p2 += (_low_frequency * (p_channel.f1p1 - p_channel.f1p2));
		p_channel.f1p3 += (_low_frequency * (p_channel.f1p2 - p_channel.f1p3));

		p_channel.f2p0 += (_high_frequency * (p_value        - p_channel.f2p0)) + 2.3283064370807974e-10;
		p_channel.f2p1 += (_high_frequency * (p_channel.f2p0 - p_channel.f2p1));
		p_channel.f2p2 += (_high_frequency * (p_channel.f2p1 - p_channel.f2p2));
		p_channel.f2p3 += (_high_frequency * (p_channel.f2p2 - p_channel.f2p3));

		double value_low  = p_channel.f1p3;
		double value_high = p_channel.sdm3 - p_channel.f2p3;
		double value_mid  = p_channel.sdm3 - (value_high + value_low);

		p_channel.sdm3 = p_channel.sdm2;
		p_channel.sdm2 = p_channel.sdm1;
		p_channel.sdm1 = p_value;

		return value_low * _low_gain + value_mid * _mid_gain + value_high * _high_gain;
}

void SiEffectEqualizer::_process_mono(Vector<double> *r_buffer, int p_start_index, int p_length) {
	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		double value = _process_channel(_left, (*r_buffer)[i]);

		r_buffer->write[i] = value;
		r_buffer->write[i + 1] = value;
	}
}

void SiEffectEqualizer::_process_stereo(Vector<double> *r_buffer, int p_start_index, int p_length) {
	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		double value_left = _process_channel(_left, (*r_buffer)[i]);
		r_buffer->write[i] = value_left;

		double value_right = _process_channel(_right, (*r_buffer)[i + 1]);
		r_buffer->write[i + 1] = value_right;
	}
}

int SiEffectEqualizer::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	if (p_channels == 1) {
		_process_mono(r_buffer, start_index, length);
	} else {
		_process_stereo(r_buffer, start_index, length);
	}

	return p_channels;
}

void SiEffectEqualizer::set_by_mml(Vector<double> p_args) {
	double low_gain       = _get_mml_arg(p_args, 0, 100) / 100.0;
	double mid_gain       = _get_mml_arg(p_args, 1, 100) / 100.0;
	double high_gain      = _get_mml_arg(p_args, 2, 100) / 100.0;
	double low_frequency  = _get_mml_arg(p_args, 3, 880);
	double high_frequency = _get_mml_arg(p_args, 4, 5000);

	set_params(low_gain, mid_gain, high_gain, low_frequency, high_frequency);
}

void SiEffectEqualizer::reset() {
	set_params();
}

SiEffectEqualizer::SiEffectEqualizer(double p_low_gain, double p_mid_gain, double p_high_gain, double p_low_frequency, double p_high_frequency) :
		SiEffectBase() {
	set_params(p_low_gain, p_mid_gain, p_high_gain, p_low_frequency, p_high_frequency);
}

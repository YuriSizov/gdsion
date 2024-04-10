/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_downsampler.h"

void SiEffectDownsampler::set_params(int p_frequency_shift, int p_bitrate, int p_channel_count) {
	_frequency_shift = p_frequency_shift;
	_bit_conv0 = 1 << p_bitrate;
	_bit_conv1 = 1 / _bit_conv0;
	_channel_count = p_channel_count;
}

int SiEffectDownsampler::prepare_process() {
	return 2;
}

void SiEffectDownsampler::_process_mono(Vector<double> *r_buffer, int p_start_index, int p_length) {
	int sample_size = 2 << _frequency_shift;
	double bc0 = _bit_conv0 / sample_size;

	for (int i = p_start_index; i < (p_start_index + p_length); i += sample_size) {
		double value = 0;
		for (int j = 0; j < sample_size; j++) {
			value += (*r_buffer)[i + j];
		}

		value = (int)(value * bc0) * _bit_conv1;

		for (int j = 0; j < sample_size; j++) {
			r_buffer->write[i + j] = value;
		}
	}
}

void SiEffectDownsampler::_process_stereo(Vector<double> *r_buffer, int p_start_index, int p_length) {
	int sample_size = 1 << _frequency_shift; // Counted in pairs.
	double bc0 = _bit_conv0 / sample_size;

	for (int i = p_start_index; i < (p_start_index + p_length); i += 2 * sample_size) {
		double value_left = 0;
		double value_right = 0;
		for (int j = 0; j < sample_size; j++) {
			value_left  += (*r_buffer)[i + 2 * j];
			value_right += (*r_buffer)[i + 2 * j + 1];
		}

		value_left  = (int)(value_left * bc0) * _bit_conv1;
		value_right = (int)(value_right * bc0) * _bit_conv1;

		for (int j = 0; j < sample_size; j++) {
			r_buffer->write[i + 2 * j] = value_left;
			r_buffer->write[i + 2 * j + 1] = value_right;
		}
	}
}

int SiEffectDownsampler::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	if (_channel_count == 1) {
		_process_mono(r_buffer, start_index, length);
	} else {
		_process_stereo(r_buffer, start_index, length);
	}

	return _channel_count;
}

void SiEffectDownsampler::set_by_mml(Vector<double> p_args) {
	int frequency_shift = _get_mml_arg(p_args, 0, 0);
	int bitrate         = _get_mml_arg(p_args, 1, 16);
	int channel_count   = _get_mml_arg(p_args, 2, 2);

	set_params(frequency_shift, bitrate, channel_count);
}

void SiEffectDownsampler::reset() {
	set_params();
}

void SiEffectDownsampler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "frequency_shift", "bitrate", "channel_count"), &SiEffectDownsampler::set_params, DEFVAL(0), DEFVAL(16), DEFVAL(2));
}

SiEffectDownsampler::SiEffectDownsampler(int p_frequency_shift, int p_bitrate, int p_channel_count) :
		SiEffectBase() {
	set_params(p_frequency_shift, p_bitrate, p_channel_count);
}

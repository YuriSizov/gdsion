/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_compressor.h"

void SiEffectCompressor::set_params(double p_threshold, double p_window_time, double p_attack_time, double p_release_time, double p_max_gain, double p_mixing_level) {
	_threshold_squared = p_threshold * p_threshold;

	_window_samples = (int)(p_window_time * 44.1);
	_window_rms_averaging = 1.0 / _window_samples;

	_attack_rate = 0.5;
	if (p_attack_time != 0) {
		_attack_rate = Math::pow(2, -1.0 / (p_attack_time * 44.1));
	}

	_release_rate = 2.0;
	if (p_release_time != 0) {
		_release_rate = Math::pow(2, 1.0 / (p_release_time * 44.1));
	}

	_max_gain = Math::pow(2, -p_max_gain / 6.0);
	_mixing_level = p_mixing_level;
}

int SiEffectCompressor::prepare_process() {
	if (_window_rms_list) {
		SinglyLinkedList<double>::free_list(_window_rms_list);
	}
	_window_rms_list = SinglyLinkedList<double>::alloc_list(_window_samples, 0.0, true);
	_window_rms_total = 0;
	_gain = 2;

	return 2;
}

int SiEffectCompressor::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	for (int i = start_index; i < (start_index + length); i += 2) {
		double value_left = (*r_buffer)[i];
		double value_right = (*r_buffer)[i + 1];

		_window_rms_list = _window_rms_list->next();
		_window_rms_total -= _window_rms_list->value;
		_window_rms_list->value = value_left * value_left + value_right * value_right;
		_window_rms_total += _window_rms_list->value;

		double rms_value = _window_rms_total * _window_rms_averaging;
		_gain *= (rms_value > _threshold_squared ? _attack_rate : _release_rate);
		if (_gain > _max_gain) {
			_gain = _max_gain;
		}

		value_left = CLAMP(value_left * _gain, -1, 1);
		value_right = CLAMP(value_right * _gain, -1, 1);

		r_buffer->write[i] = value_left * _mixing_level;
		r_buffer->write[i + 1] = value_right * _mixing_level;
	}

	return p_channels;
}

void SiEffectCompressor::set_by_mml(Vector<double> p_args) {
	double threshold    = _get_mml_arg(p_args, 0, 70) / 100.0;
	double window_time  = _get_mml_arg(p_args, 1, 50);
	double attack_time  = _get_mml_arg(p_args, 2, 20);
	double release_time = _get_mml_arg(p_args, 3, 20);
	double max_gain     = _get_mml_arg(p_args, 4, -6);
	double mixing_level = _get_mml_arg(p_args, 5, 50) / 100.0;

	set_params(threshold, window_time, attack_time, release_time, max_gain, mixing_level);
}

void SiEffectCompressor::reset() {
	set_params();
}

void SiEffectCompressor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "threshold", "window_time", "attack_time", "release_time", "max_gain", "mixing_level"), &SiEffectCompressor::set_params, DEFVAL(0.7), DEFVAL(50), DEFVAL(20), DEFVAL(20), DEFVAL(-6), DEFVAL(0.5));
}

SiEffectCompressor::SiEffectCompressor(double p_threshold, double p_window_time, double p_attack_time, double p_release_time, double p_max_gain, double p_mixing_level) :
		SiEffectBase() {
	set_params(p_threshold, p_window_time, p_attack_time, p_release_time, p_max_gain, p_mixing_level);
}

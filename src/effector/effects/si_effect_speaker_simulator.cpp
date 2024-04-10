/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_speaker_simulator.h"

void SiEffectSpeakerSimulator::set_params(double p_hardness) {
	_spring_coef = 1.0 - p_hardness * p_hardness;
	if (_spring_coef < 0.1) {
		_spring_coef = 0.1;
	}
}

int SiEffectSpeakerSimulator::prepare_process() {
	_diaphragm_pos_left = 0;
	_diaphragm_pos_right = 0;
	_previous_left = 0;
	_previous_right = 0;

	return 2;
}

int SiEffectSpeakerSimulator::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	for (int i = start_index; i < (start_index - length); i += 2) {
		double value_left = (*r_buffer)[i] - _previous_left;
		_diaphragm_pos_left *= _spring_coef;
		_diaphragm_pos_left += value_left;

		_previous_left = (*r_buffer)[i];
		r_buffer->write[i] = _diaphragm_pos_left;

		double value_right = (*r_buffer)[i + 1] - _previous_right;
		_diaphragm_pos_right *= _spring_coef;
		_diaphragm_pos_right += value_right;

		_previous_right = (*r_buffer)[i + 1];
		r_buffer->write[i + 1] = _diaphragm_pos_right;
	}

	return p_channels;
}

void SiEffectSpeakerSimulator::set_by_mml(Vector<double> p_args) {
	double hardness = _get_mml_arg(p_args, 0, 20) / 100.0;

	set_params(hardness);
}

void SiEffectSpeakerSimulator::reset() {
	set_params();
}

void SiEffectSpeakerSimulator::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "hardness"), &SiEffectSpeakerSimulator::set_params, DEFVAL(0.2));
}

SiEffectSpeakerSimulator::SiEffectSpeakerSimulator(double p_hardness) :
		SiEffectBase() {
	set_params(p_hardness);
}

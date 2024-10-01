/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_autopan.h"

void SiEffectAutopan::set_params(double p_frequency, double p_stereo_width) {
	_lfo_step = (int)(172.265625 / (p_frequency * 0.5)); // 44100/256
	if (_lfo_step <= 4) {
		_lfo_step = 4;
	}

	double width = p_stereo_width;
	if (width == 0) {
		width = 1;
		_stereo = true;
	}
	width *= 0.01227184630308513; // PI / 256

	// Volume table.
	for (int i = -128; i < 128; i++) {
		const double value = Math::sin(1.5707963267948965 + i * width);

		_p_left->get()->value = value;
		_p_left->next();
		_p_right->get()->value = value;
		_p_right->next();
	}

	// The right buffer is shifted by half the size.
	for (int i = 0; i < BUFFER_SIZE >> 1; i++) {
		_p_right->next();
	}
}

int SiEffectAutopan::prepare_process() {
	return _stereo ? 2 : 1;
}

void SiEffectAutopan::_process_lfo_mono(Vector<double> *r_buffer, int p_start_index, int p_length) {
	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		double value = (*r_buffer)[i];

		r_buffer->write[i] = value * _p_left->get()->value;
		r_buffer->write[i + 1] = value * _p_right->get()->value;
	}

	_p_left->next();
	_p_right->next();
}

void SiEffectAutopan::_process_lfo_stereo(Vector<double> *r_buffer, int p_start_index, int p_length) {
	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		double value_left = (*r_buffer)[i];
		double value_right = (*r_buffer)[i + 1];

		r_buffer->write[i] = value_left * _p_left->get()->value - value_right * _p_right->get()->value;
		r_buffer->write[i + 1] = value_left * _p_right->get()->value + value_right * _p_left->get()->value;
	}

	_p_left->next();
	_p_right->next();
}

int SiEffectAutopan::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	int step = _lfo_residue_step;
	int max = start_index + length;
	int i = start_index;
	while (i < (max - step)) {
		if (_stereo) {
			_process_lfo_stereo(r_buffer, i, step);
		} else {
			_process_lfo_mono(r_buffer, i, step);
		}

		i += step;
		step = _lfo_step << 1;
	}

	if (_stereo) {
		_process_lfo_stereo(r_buffer, i, max - i);
	} else {
		_process_lfo_mono(r_buffer, i, max - i);
	}

	_lfo_residue_step = step - (max - i);

	return 2;
}

void SiEffectAutopan::set_by_mml(Vector<double> p_args) {
	double frequency = _get_mml_arg(p_args, 0, 1);
	double stereo_width = _get_mml_arg(p_args, 1, 100) / 100.0;

	set_params(frequency, stereo_width);
}

void SiEffectAutopan::reset() {
	_lfo_residue_step = 0;
	set_params();
}

void SiEffectAutopan::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "frequency", "stereo_width"), &SiEffectAutopan::set_params, DEFVAL(1), DEFVAL(1));
}

SiEffectAutopan::SiEffectAutopan(double p_frequency, double p_stereo_width) :
		SiEffectBase() {
	_p_left = memnew(SinglyLinkedList<double>(BUFFER_SIZE, 0.0, true));
	_p_right = memnew(SinglyLinkedList<double>(BUFFER_SIZE, 0.0, true));

	set_params(p_frequency, p_stereo_width);
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_wave_shaper.h"

void SiEffectWaveShaper::set_params(double p_distortion, double p_output_level) {
	double distortion = p_distortion;
	if (distortion >= 1) {
		distortion = 0.9999847412109375; //65535/65536
	}

	_coefficient = 2 * distortion/(1 - distortion);
	_output_level = p_output_level;
}

int SiEffectWaveShaper::prepare_process() {
	return 2;
}

int SiEffectWaveShaper::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	double coef = (1 + _coefficient) * _output_level;

	if (p_channels == 2) {
		for (int i = start_index; i < (start_index + length); i++) {
			double value = (*r_buffer)[i];
			value = coef * value / (1 + _coefficient * ABS(value));

			r_buffer->write[i] = value;
		}
	} else {
		for (int i = start_index; i < (start_index + length); i += 2) {
			double value = (*r_buffer)[i];
			value = coef * value / (1 + _coefficient * ABS(value));

			r_buffer->write[i] = value;
			r_buffer->write[i + 1] = value;

		}
	}

	return p_channels;
}

void SiEffectWaveShaper::set_by_mml(Vector<double> p_args) {
	double distortion   = _get_mml_arg(p_args, 0, 50) / 100.0;
	double output_level = _get_mml_arg(p_args, 1, 100) / 100.0;

	set_params(distortion, output_level);
}

void SiEffectWaveShaper::reset() {
	set_params();
}

SiEffectWaveShaper::SiEffectWaveShaper(double p_distortion, double p_output_level) :
		SiEffectBase() {
	set_params(p_distortion, p_output_level);
}

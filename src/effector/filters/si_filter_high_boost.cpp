/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_filter_high_boost.h"

void SiFilterHighBoost::set_params(double p_frequency, double p_slope, double p_gain) {
	// TODO: Pick better names for these variables.
	double slope = MAX(p_slope, 1);

	double A   = Math::pow(10, p_gain * 0.025);
	double omg = p_frequency * 0.00014247585730565955; // 2*pi/44100
	double cos = Math::cos(omg);
	double sin = Math::sin(omg);

	double alp    = sin * 0.5 * Math::sqrt((A + 1.0 / A) * (1.0 / slope - 1) + 2);
	double alpsA2 = alp * Math::sqrt(A) * 2;
	double ia0    = 1.0 / ((A + 1) - (A - 1) * cos + alpsA2);

	_a1 =  2 * ((A - 1) - (A + 1) * cos)          * ia0;
	_a2 =      ((A + 1) - (A - 1) * cos - alpsA2) * ia0;
	_b0 =      ((A + 1) + (A - 1) * cos + alpsA2) * A * ia0;
	_b1 = -2 * ((A - 1) + (A + 1) * cos)          * A * ia0;
	_b2 =      ((A + 1) + (A - 1) * cos - alpsA2) * A * ia0;
}

void SiFilterHighBoost::set_by_mml(Vector<double> p_args) {
	double frequency = _get_mml_arg(p_args, 0, 5500);
	double slope = _get_mml_arg(p_args, 1, 1);
	double gain = _get_mml_arg(p_args, 2, 6);

	set_params(frequency, slope, gain);
}

void SiFilterHighBoost::reset() {
	set_params();
}

SiFilterHighBoost::SiFilterHighBoost(double p_frequency, double p_slope, double p_gain) :
		SiFilterBase() {
	set_params(p_frequency, p_slope, p_gain);
}

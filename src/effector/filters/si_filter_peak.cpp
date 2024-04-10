/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_filter_peak.h"

void SiFilterPeak::set_params(double p_frequency, double p_band, double p_gain) {
	// TODO: Pick better names for these variables.

	double A   = Math::pow(10, p_gain * 0.025);
	double omg = p_frequency * 0.00014247585730565955; // 2*pi/44100
	double cos = Math::cos(omg);
	double sin = Math::sin(omg);

	double ang   = 0.34657359027997264 * p_band * omg / sin;
	double alp   = sin * Math::sinh(ang); // log(2)*0.5
	double alpA  = alp * A;
	double alpiA = alp / A;
	double ia0   = 1.0 / (1.0 + alpiA);

	_a1 = -2 * cos * ia0;
	_b1 = _a1;
	_a2 = (1 - alpiA) * ia0;
	_b0 = (1 + alpA) * ia0;
	_b2 = (1 - alpA) * ia0;
}

void SiFilterPeak::set_by_mml(Vector<double> p_args) {
	double frequency = _get_mml_arg(p_args, 0, 3000);
	double band      = _get_mml_arg(p_args, 1, 1);
	double gain      = _get_mml_arg(p_args, 2, 6);

	set_params(frequency, band, gain);
}

void SiFilterPeak::reset() {
	set_params();
}

void SiFilterPeak::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "frequency", "band", "gain"), &SiFilterPeak::set_params, DEFVAL(3000), DEFVAL(1), DEFVAL(6));
}

SiFilterPeak::SiFilterPeak(double p_frequency, double p_band, double p_gain) :
		SiFilterBase() {
	set_params(p_frequency, p_band, p_gain);
}

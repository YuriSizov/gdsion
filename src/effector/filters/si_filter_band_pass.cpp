/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_filter_band_pass.h"

void SiFilterBandPass::set_params(double p_frequency, double p_band) {
	// TODO: Pick better names for these variables.

	double omg = p_frequency * 0.00014247585730565955; // 2*pi/44100
	double cos = Math::cos(omg);
	double sin = Math::sin(omg);

	double ang = 0.34657359027997264 * p_band * omg / sin;
	double alp = sin * Math::sinh(ang); // log(2)*0.5
	double ia0 = 1.0 / (1.0 + alp);

	_a1 = -2 * cos * ia0;
	_a2 = (1 - alp) * ia0;
	_b1 = 0;
	_b0 = alp * ia0;
	_b2 = -_b0;
}

void SiFilterBandPass::set_by_mml(Vector<double> p_args) {
	double frequency = _get_mml_arg(p_args, 0, 3000);
	double band      = _get_mml_arg(p_args, 1, 1);

	set_params(frequency, band);
}

void SiFilterBandPass::reset() {
	set_params();
}

void SiFilterBandPass::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "frequency", "band"), &SiFilterBandPass::set_params, DEFVAL(3000), DEFVAL(1));
}

SiFilterBandPass::SiFilterBandPass(double p_frequency, double p_band) :
		SiFilterBase() {
	set_params(p_frequency, p_band);
}

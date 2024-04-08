/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_CONTROLLABLE_FILTER_LOW_PASS_H
#define SI_CONTROLLABLE_FILTER_LOW_PASS_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/filters/si_controllable_filter_base.h"

using namespace godot;

class SiControllableFilterLowPass : public SiControllableFilterBase {

	virtual void _process_lfo(Vector<double> *r_buffer, int p_start_index, int p_length) override;

public:
	SiControllableFilterLowPass(double p_cutoff = 1, double p_resonance = 0);
	~SiControllableFilterLowPass() {}
};

#endif // SI_CONTROLLABLE_FILTER_LOW_PASS_H

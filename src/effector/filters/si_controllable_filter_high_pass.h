/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_CONTROLLABLE_FILTER_HIGH_PASS_H
#define SI_CONTROLLABLE_FILTER_HIGH_PASS_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/filters/si_controllable_filter_base.h"

using namespace godot;

class SiControllableFilterHighPass : public SiControllableFilterBase {
	GDCLASS(SiControllableFilterHighPass, SiControllableFilterBase)

	virtual void _process_lfo(Vector<double> *r_buffer, int p_start_index, int p_length) override;

protected:
	static void _bind_methods() {}

public:
	SiControllableFilterHighPass(double p_cutoff = 1, double p_resonance = 0);
	~SiControllableFilterHighPass() {}
};

#endif // SI_CONTROLLABLE_FILTER_HIGH_PASS_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_LOW_PASS_H
#define SI_FILTER_LOW_PASS_H

#include "effector/filters/si_filter_base.h"

class SiFilterLowPass : public SiFilterBase {
	GDCLASS(SiFilterLowPass, SiFilterBase)

protected:
	static void _bind_methods() {}

public:
	void set_params(double p_frequency = 800, double p_band = 1);

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiFilterLowPass(double p_frequency = 800, double p_band = 1);
	~SiFilterLowPass() {}
};

#endif // SI_FILTER_LOW_PASS_H

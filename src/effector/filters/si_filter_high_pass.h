/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_HIGH_PASS_H
#define SI_FILTER_HIGH_PASS_H

#include "effector/filters/si_filter_base.h"

class SiFilterHighPass : public SiFilterBase {
	GDCLASS(SiFilterHighPass, SiFilterBase)

protected:
	static void _bind_methods();

public:
	void set_params(double p_frequency = 5500, double p_band = 1);

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiFilterHighPass(double p_frequency = 5500, double p_band = 1);
	~SiFilterHighPass() {}
};

#endif // SI_FILTER_HIGH_PASS_H

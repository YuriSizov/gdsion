/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_LOW_BOOST_H
#define SI_FILTER_LOW_BOOST_H

#include "effector/filters/si_filter_base.h"

class SiFilterLowBoost : public SiFilterBase {
	GDCLASS(SiFilterLowBoost, SiFilterBase)

protected:
	static void _bind_methods() {}

public:
	void set_params(double p_frequency = 3000, double p_slope = 1, double p_gain = 6);

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiFilterLowBoost(double p_frequency = 3000, double p_slope = 1, double p_gain = 6);
	~SiFilterLowBoost() {}
};

#endif // SI_FILTER_LOW_BOOST_H

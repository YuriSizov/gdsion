/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_PEAK_H
#define SI_FILTER_PEAK_H

#include "effector/filters/si_filter_base.h"

class SiFilterPeak : public SiFilterBase {
	GDCLASS(SiFilterPeak, SiFilterBase)

protected:
	static void _bind_methods() {}

public:
	void set_params(double p_frequency = 3000, double p_band = 1, double p_gain = 6);

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiFilterPeak(double p_frequency = 3000, double p_band = 1, double p_gain = 6);
	~SiFilterPeak() {}
};

#endif // SI_FILTER_PEAK_H

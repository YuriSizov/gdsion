/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_NOTCH_H
#define SI_FILTER_NOTCH_H

#include "effector/filters/si_filter_base.h"

class SiFilterNotch : public SiFilterBase {
	GDCLASS(SiFilterNotch, SiFilterBase)

protected:
	static void _bind_methods();

public:
	void set_params(double p_frequency = 3000, double p_band = 1);

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiFilterNotch(double p_frequency = 3000, double p_band = 1);
	~SiFilterNotch() {}
};

#endif // SI_FILTER_NOTCH_H

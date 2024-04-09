/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_WAVE_SHAPER_H
#define SI_EFFECT_WAVE_SHAPER_H

#include "effector/si_effect_base.h"

class SiEffectWaveShaper : public SiEffectBase {
	GDCLASS(SiEffectWaveShaper, SiEffectBase)

	int _coefficient = 0;
	double _output_level = 0;

protected:
	static void _bind_methods() {}

public:
	void set_params(double p_distortion = 0.5, double p_output_level = 1.0);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectWaveShaper(double p_distortion = 0.5, double p_output_level = 1.0);
	~SiEffectWaveShaper() {}
};

#endif // SI_EFFECT_WAVE_SHAPER_H

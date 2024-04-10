/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_SPEAKER_SIMULATOR_H
#define SI_EFFECT_SPEAKER_SIMULATOR_H

#include "effector/si_effect_base.h"

class SiEffectSpeakerSimulator : public SiEffectBase {
	GDCLASS(SiEffectSpeakerSimulator, SiEffectBase)

	double _spring_coef = 0.96;

	double _diaphragm_pos_left = 0;
	double _diaphragm_pos_right = 0;
	double _previous_left = 0;
	double _previous_right = 0;

protected:
	static void _bind_methods();

public:
	void set_params(double p_hardness = 0.2);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectSpeakerSimulator(double p_hardness = 0.2);
	~SiEffectSpeakerSimulator() {}
};

#endif // SI_EFFECT_SPEAKER_SIMULATOR_H

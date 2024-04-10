/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_STEREO_EXPANDER_H
#define SI_EFFECT_STEREO_EXPANDER_H

#include "effector/si_effect_base.h"

class SiEffectStereoExpander : public SiEffectBase {
	GDCLASS(SiEffectStereoExpander, SiEffectBase)

	double _left_to_left = 0;
	double _right_to_left = 0;
	double _left_to_right = 0;
	double _right_to_right = 0;

	bool _monoralize = false;

protected:
	static void _bind_methods();

public:
	void set_params(double p_stereo_width = 1.4, double p_rotation = 0, bool p_phase_invert = false);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectStereoExpander(double p_stereo_width = 1.4, double p_rotation = 0, bool p_phase_invert = false);
	~SiEffectStereoExpander() {}
};

#endif // SI_EFFECT_STEREO_EXPANDER_H

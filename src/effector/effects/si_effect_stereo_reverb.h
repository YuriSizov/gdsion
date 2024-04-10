/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_STEREO_REVERB_H
#define SI_EFFECT_STEREO_REVERB_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/si_effect_base.h"

using namespace godot;

class SiEffectStereoReverb : public SiEffectBase {
	GDCLASS(SiEffectStereoReverb, SiEffectBase)

	static const int DELAY_BUFFER_BITS = 13;
	static const int DELAY_BUFFER_FILTER = (1 << DELAY_BUFFER_BITS) - 1;

	Vector<double> _delay_buffer_left;
	Vector<double> _delay_buffer_right;

	int _pointer_read0 = 0;
	int _pointer_read1 = 0;
	int _pointer_read2 = 0;
	int _pointer_write = 0;
	double _feedback0 = 0;
	double _feedback1 = 0;
	double _feedback2 = 0;
	double _wet = 0;

	void _process_channel(Vector<double> *r_buffer, int p_buffer_index, Vector<double> *r_delay_buffer);

protected:
	static void _bind_methods();

public:
	void set_params(double p_delay1 = 0.7, double p_delay2 = 0.4, double p_feedback = 0.8, double p_wet = 0.3);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectStereoReverb(double p_delay1 = 0.7, double p_delay2 = 0.4, double p_feedback = 0.8, double p_wet = 0.3);
	~SiEffectStereoReverb() {}
};

#endif // SI_EFFECT_STEREO_REVERB_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_STEREO_DELAY_H
#define SI_EFFECT_STEREO_DELAY_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/si_effect_base.h"

using namespace godot;

class SiEffectStereoDelay : public SiEffectBase {
	GDCLASS(SiEffectStereoDelay, SiEffectBase)

	static const int DELAY_BUFFER_BITS = 16;
	static const int DELAY_BUFFER_FILTER = (1 << DELAY_BUFFER_BITS) - 1;

	Vector<double> _delay_buffer_left;
	Vector<double> _delay_buffer_right;

	int _pointer_read = 0;
	int _pointer_write = 0;
	double _feedback = 0;
	double _wet = 0;
	bool _cross = false;

	void _process_channel(Vector<double> *r_buffer, int p_buffer_index, Vector<double> p_read_buffer, Vector<double> *r_write_buffer);

protected:
	static void _bind_methods() {}

public:
	void set_params(double p_delay_time = 250, double p_feedback = 0.25, bool p_cross = false, double p_wet = 0.25);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectStereoDelay(double p_delay_time = 250, double p_feedback = 0.25, bool p_cross = false, double p_wet = 0.25);
	~SiEffectStereoDelay() {}
};

#endif // SI_EFFECT_STEREO_DELAY_H

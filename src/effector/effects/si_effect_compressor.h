/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_COMPRESSOR_H
#define SI_EFFECT_COMPRESSOR_H

#include "effector/si_effect_base.h"
#include "templates/singly_linked_list.h"

class SiEffectCompressor : public SiEffectBase {
	GDCLASS(SiEffectCompressor, SiEffectBase)

	SinglyLinkedList<double> *_window_rms_list = nullptr;
	int _window_samples = 0;
	double _window_rms_total = 0;
	double _window_rms_averaging = 0;

	double _threshold_squared = 0;
	double _attack_rate = 0;  // Per sample decay.
	double _release_rate = 0; // Per sample decay.

	double _max_gain = 0;
	double _mixing_level = 0;
	double _gain = 0;

protected:
	static void _bind_methods();

public:
	void set_params(double p_threshold = 0.7, double p_window_time = 50, double p_attack_time = 20, double p_release_time = 20, double p_max_gain = -6, double p_mixing_level = 0.5);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectCompressor(double p_threshold = 0.7, double p_window_time = 50, double p_attack_time = 20, double p_release_time = 20, double p_max_gain = -6, double p_mixing_level = 0.5);
	~SiEffectCompressor() {}
};

#endif // SI_EFFECT_COMPRESSOR_H

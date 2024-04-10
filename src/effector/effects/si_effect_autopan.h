/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_AUTOPAN_H
#define SI_EFFECT_AUTOPAN_H

#include "effector/si_effect_base.h"
#include "templates/singly_linked_list.h"

class SiEffectAutopan : public SiEffectBase {
	GDCLASS(SiEffectAutopan, SiEffectBase)

	bool _stereo = false;
	int _lfo_step = 0;
	int _lfo_residue_step = 0;
	SinglyLinkedList<double> *_p_left;
	SinglyLinkedList<double> *_p_right;

	void _process_lfo_mono(Vector<double> *r_buffer, int p_start_index, int p_length);
	void _process_lfo_stereo(Vector<double> *r_buffer, int p_start_index, int p_length);

protected:
	static void _bind_methods();

public:
	void set_params(double p_frequency = 1, double p_stereo_width = 1);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectAutopan(double p_frequency = 1, double p_stereo_width = 1);
	~SiEffectAutopan() {}
};

#endif // SI_EFFECT_AUTOPAN_H

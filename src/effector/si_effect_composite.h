/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_COMPOSITE_H
#define SI_EFFECT_COMPOSITE_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/si_effect_base.h"

using namespace godot;

class SiEffectComposite : public SiEffectBase {

	struct SlottedEffect {
		Vector<SiEffectBase *> effects;
		Vector<double> buffer;
		double send_level = 1;
		double mix_level = 1;
	};

	SlottedEffect _slots[8];

public:
	void set_slot_effects(int p_slot, Vector<SiEffectBase *> p_effects);
	void set_slot_levels(int p_slot, double p_send_level, double p_mix_level);

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void initialize() override;

	SiEffectComposite() {}
	~SiEffectComposite() {}
};

#endif // SI_EFFECT_COMPOSITE_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_CONTROLLABLE_FILTER_BASE_H
#define SI_CONTROLLABLE_FILTER_BASE_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/si_effect_base.h"
#include "templates/singly_linked_list.h"

class SiControllableFilterBase : public SiEffectBase {

	static SinglyLinkedList<int> *_increment_envelope_table;
	static SinglyLinkedList<int> *_decrement_envelope_table;

	SinglyLinkedList<int> *_cutoff_ptr = nullptr;
	SinglyLinkedList<int> *_resonance_ptr = nullptr;

	int _lfo_step = 0;
	int _lfo_residue_step = 0;

	virtual void _process_lfo(Vector<double> *r_buffer, int p_start_index, int p_length) {}

protected:
	double _p0_right = 0;
	double _p1_right = 0;
	double _p0_left = 0;
	double _p1_left = 0;

	int _cutoff_index = 0;
	double _resonance = 0;

public:
	void set_params(int p_cutoff = 255, int p_resonance = 255, double p_fps = 20);
	void set_params_manually(double p_cutoff, double p_resonance);

	double get_cutoff() const;
	void set_cutoff(double p_value);

	double get_resonance() const { return _resonance; }
	void set_resonance(double p_value);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;

	virtual void initialize() override;

	SiControllableFilterBase();
	~SiControllableFilterBase() {}
};

#endif // SI_CONTROLLABLE_FILTER_BASE_H

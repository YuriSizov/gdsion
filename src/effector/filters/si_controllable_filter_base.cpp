/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_controllable_filter_base.h"

#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_ref_table.h"

SinglyLinkedList<int> *SiControllableFilterBase::_increment_envelope_table = nullptr;
SinglyLinkedList<int> *SiControllableFilterBase::_decrement_envelope_table = nullptr;

void SiControllableFilterBase::set_params(int p_cutoff, int p_resonance, double p_fps) {
	_cutoff_ptr = nullptr;
	if (p_cutoff >= 0 && p_cutoff < 255) {
		SiMMLEnvelopeTable *table = SiMMLRefTable::get_instance()->get_envelope_table(p_cutoff);
		if (table) {
			_cutoff_ptr = table->head;
		}
	}

	_resonance_ptr = nullptr;
	if (p_resonance >= 0 && p_resonance < 255) {
		SiMMLEnvelopeTable *table = SiMMLRefTable::get_instance()->get_envelope_table(p_resonance);
		if (table) {
			_resonance_ptr = table->head;
		}
	}

	_cutoff_index = (_cutoff_ptr ? _cutoff_ptr->value : 128);
	_resonance = (_resonance_ptr ? _resonance_ptr->value * 0.007751937984496124 : 0); // 0.007751937984496124 = 1/129

	_lfo_step = (int)(44100 / p_fps);
	if (_lfo_step <= 44) {
		_lfo_step = 44;
	}

	_lfo_residue_step = _lfo_step << 1;
}

void SiControllableFilterBase::set_params_manually(double p_cutoff, double p_resonance) {
	_lfo_step = 2048;
	_lfo_residue_step = 4096;

	set_cutoff(p_cutoff);
	set_resonance(p_resonance);
}

double SiControllableFilterBase::get_cutoff() const {
	return _cutoff_index * 0.0078125;
}

void SiControllableFilterBase::set_cutoff(double p_value) {
	_cutoff_index = CLAMP(p_value * 128, 0, 128);
}

void SiControllableFilterBase::set_resonance(double p_value) {
	_resonance = CLAMP(p_value, 0, 1);
}

//

int SiControllableFilterBase::prepare_process() {
	_lfo_residue_step = 0;
	_p0_left = 0;
	_p1_left = 0;
	_p0_right = 0;
	_p1_right = 0;

	return 2;
}

int SiControllableFilterBase::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	int step = _lfo_residue_step;
	int max = start_index + length;
	int i = start_index;
	while (i < (max - step)) {
		_process_lfo(r_buffer, i, step);

		if (_cutoff_ptr) {
			_cutoff_ptr = _cutoff_ptr->next;
			_cutoff_index = (_cutoff_ptr ? _cutoff_ptr->value : 128);
		}

		if (_resonance_ptr) {
			_resonance_ptr = _resonance_ptr->next;
			_resonance = (_resonance_ptr ? _resonance_ptr->value * 0.007751937984496124 : 0);
		}

		i += step;
		step = _lfo_step << 1;
	}

	_process_lfo(r_buffer, i, max - i);
	_lfo_residue_step = step - (max - i);

	return p_channels;
}

void SiControllableFilterBase::set_by_mml(Vector<double> p_args) {
	int cutoff    = _get_mml_arg(p_args, 0, 255);
	int resonance = _get_mml_arg(p_args, 1, 255);
	double fps    = _get_mml_arg(p_args, 2, 20);

	set_params(cutoff, resonance, fps);
}

void SiControllableFilterBase::reset() {
	set_params();
}

void SiControllableFilterBase::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_params", "cutoff", "resonance", "fps"), &SiControllableFilterBase::set_params, DEFVAL(255), DEFVAL(255), DEFVAL(20));
	ClassDB::bind_method(D_METHOD("set_params_manually", "cutoff", "resonance"), &SiControllableFilterBase::set_params_manually);
}

SiControllableFilterBase::SiControllableFilterBase() :
		SiEffectBase() {
	if (_increment_envelope_table == nullptr) {
		_increment_envelope_table = SinglyLinkedList<int>::alloc_list(129);
		_decrement_envelope_table = SinglyLinkedList<int>::alloc_list(129);

		SinglyLinkedList<int> *inc_ptr = _increment_envelope_table;
		SinglyLinkedList<int> *dec_ptr = _decrement_envelope_table;

		for (int i = 0; i < 129; i++) {
			inc_ptr->value = i;
			dec_ptr->value = 128 - i;

			inc_ptr = inc_ptr->next;
			dec_ptr = dec_ptr->next;
		}
	}
}

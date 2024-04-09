/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_composite.h"

void SiEffectComposite::set_slot_effects(int p_slot, Vector<Ref<SiEffectBase>> p_effects) {
	ERR_FAIL_INDEX(p_slot, 8);

	_slots[p_slot].effects = p_effects;
}

void SiEffectComposite::set_slot_levels(int p_slot, double p_send_level, double p_mix_level) {
	ERR_FAIL_INDEX(p_slot, 8);

	_slots[p_slot].send_level = p_send_level;
	_slots[p_slot].mix_level = p_mix_level;
}

int SiEffectComposite::prepare_process() {
	for (int i = 0; i < 8; i++) {
		for (Ref<SiEffectBase> effect : _slots[i].effects) {
			effect->prepare_process();
		}
	}

	return 2;
}

int SiEffectComposite::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	for (int i = 0; i < 8; i++) {
		if (_slots[i].effects.is_empty()) {
			continue;
		}

		Vector<double> slot_buffer = _slots[i].buffer;
		if (slot_buffer.size() < r_buffer->size()) {
			slot_buffer.resize_zeroed(r_buffer->size());
		}

		for (int j = p_start_index; j < (p_start_index + p_length); j++) {
			slot_buffer.write[j] = (*r_buffer)[j] * _slots[i].send_level;
		}
	}

	for (int j = p_start_index; j < (p_start_index + p_length); j++) {
		r_buffer->write[j] *= _slots[0].send_level;
	}
	for (int i = 1; i < 8; i++) {
		if (_slots[i].effects.is_empty()) {
			continue;
		}

		int channel_num = p_channels;
		for (Ref<SiEffectBase> effect : _slots[i].effects) {
			channel_num = effect->process(channel_num, &_slots[i].buffer, p_start_index, p_length);
		}

		for (int j = p_start_index; j < (p_start_index + p_length); j++) {
			r_buffer->write[j] += _slots[i].buffer[j] * _slots[i].mix_level;
		}
	}

	int out_channels = p_channels;
	if (!_slots[0].effects.is_empty()) {
		for (Ref<SiEffectBase> effect : _slots[0].effects) {
			out_channels = effect->process(out_channels, r_buffer, p_start_index, p_length);
		}

		if (_slots[0].mix_level != 1) {
			for (int j = p_start_index; j < (p_start_index + p_length); j++) {
				r_buffer->write[j] *= _slots[0].mix_level;
			}
		}
	}

	return out_channels;
}

void SiEffectComposite::reset() {
	for (int i = 0; i < 8; i++) {
		_slots[i].effects.clear();
		_slots[i].buffer.clear();
		_slots[i].send_level = 1;
		_slots[i].mix_level = 1;
	}
}

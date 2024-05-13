/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effector.h"

#include <godot_cpp/core/memory.hpp>
#include "chip/siopm_sound_chip.h"
#include "chip/siopm_stream.h"
#include "effector/si_effect_stream.h"
#include "templates/type_constraints.h"

#include "effector/effects/si_effect_autopan.h"
#include "effector/effects/si_effect_compressor.h"
#include "effector/effects/si_effect_distortion.h"
#include "effector/effects/si_effect_downsampler.h"
#include "effector/effects/si_effect_equalizer.h"
#include "effector/effects/si_effect_speaker_simulator.h"
#include "effector/effects/si_effect_stereo_chorus.h"
#include "effector/effects/si_effect_stereo_delay.h"
#include "effector/effects/si_effect_stereo_expander.h"
#include "effector/effects/si_effect_stereo_reverb.h"
#include "effector/effects/si_effect_wave_shaper.h"
#include "effector/filters/si_controllable_filter_high_pass.h"
#include "effector/filters/si_controllable_filter_low_pass.h"
#include "effector/filters/si_filter_all_pass.h"
#include "effector/filters/si_filter_band_pass.h"
#include "effector/filters/si_filter_high_boost.h"
#include "effector/filters/si_filter_high_pass.h"
#include "effector/filters/si_filter_low_boost.h"
#include "effector/filters/si_filter_low_pass.h"
#include "effector/filters/si_filter_notch.h"
#include "effector/filters/si_filter_peak.h"
#include "effector/filters/si_filter_vowel.h"

HashMap<String, List<Ref<SiEffectBase>>> SiEffector::_effect_instances;

// Effects.

template <class T>
void SiEffector::register_effect(const String &p_name) {
	derived_from<T, SiEffectBase>(); // Compile-time check: Only accept SiEffectBase derivatives.

	// We don't actually use the type here, as that would make it incompatible with the collection.
	_effect_instances[p_name] = List<Ref<SiEffectBase>>();
}

Ref<SiEffectBase> SiEffector::get_effect_instance(const String &p_name) {
	ERR_FAIL_COND_V_MSG(!_effect_instances.has(p_name), Ref<SiEffectBase>(), vformat("SiEffector: Effect called '%s' does not exist.", p_name));

	List<Ref<SiEffectBase>> instances = _effect_instances[p_name];

	// Check if we have free instances to reuse first.
	for (int i = 0; i < instances.size(); i++) {
		if (instances[i]->is_free()) {
			Ref<SiEffectBase> effect = instances[i];

			effect->set_free(false);
			effect->reset();
			return effect;
		}
	}

	// Failing above we allocate a new one.
	// FIXME: Maybe there is a better way to do it, but templates, the most obvious choice, didn't prove to be helpful.

#define CREATE_EFFECT(m_type, m_name)                                 \
	if (p_name == m_name) {                                           \
		Ref<SiEffectBase> effect = create_effect_instance<m_type>();  \
		instances.push_back(effect);                                  \
		return effect;                                                \
	}

	CREATE_EFFECT(SiEffectAutopan, "autopan");
	CREATE_EFFECT(SiEffectCompressor, "comp");
	CREATE_EFFECT(SiEffectDistortion, "dist");
	CREATE_EFFECT(SiEffectDownsampler, "ds");
	CREATE_EFFECT(SiEffectEqualizer, "eq");
	CREATE_EFFECT(SiEffectSpeakerSimulator, "speaker");
	CREATE_EFFECT(SiEffectStereoChorus, "chorus");
	CREATE_EFFECT(SiEffectStereoDelay, "delay");
	CREATE_EFFECT(SiEffectStereoExpander, "stereo");
	CREATE_EFFECT(SiEffectStereoReverb, "reverb");
	CREATE_EFFECT(SiEffectWaveShaper, "ws");

	CREATE_EFFECT(SiFilterAllPass, "af");
	CREATE_EFFECT(SiFilterBandPass, "bf");
	CREATE_EFFECT(SiFilterHighBoost, "hb");
	CREATE_EFFECT(SiFilterHighPass, "hf");
	CREATE_EFFECT(SiFilterLowBoost, "lb");
	CREATE_EFFECT(SiFilterLowPass, "lf");
	CREATE_EFFECT(SiFilterNotch, "nf");
	CREATE_EFFECT(SiFilterPeak, "pf");
	CREATE_EFFECT(SiFilterVowel, "vowel");

	CREATE_EFFECT(SiControllableFilterHighPass, "nhf");
	CREATE_EFFECT(SiControllableFilterLowPass, "nlf");

#undef CREATE_EFFECT

	// This should only be possible if custom effects are registered outside of the class.
	return Ref<SiEffectBase>();
}

template <class T>
Ref<T> SiEffector::create_effect_instance() {
	Ref<T> effect;
	effect.instantiate();

	effect->set_free(false);
	effect->reset();
	return effect;
}

// Slots and connections.

SiEffectStream *SiEffector::_get_global_stream(int p_slot) {
	if (_global_effects[p_slot] == nullptr) {
		SiEffectStream *effect = _alloc_stream(0);
		_global_effects.write[p_slot] = effect;
		_sound_chip->set_stream_slot(p_slot, effect->get_stream());
		_global_effect_count++;
	}

	return _global_effects[p_slot];
}

SiEffectStream *SiEffector::_alloc_stream(int p_depth) {
	SiEffectStream *effect = nullptr;
	if (!_free_effect_streams.is_empty()) {
		effect = _free_effect_streams.front()->get();
		_free_effect_streams.pop_front();
	} else {
		effect = memnew(SiEffectStream(_sound_chip));
	}

	effect->initialize(p_depth);
	return effect;
}

List<Ref<SiEffectBase>> SiEffector::get_slot_effect_list(int p_slot) const {
	if (!_global_effects[p_slot]) {
		return List<Ref<SiEffectBase>>();
	}
	return _global_effects[p_slot]->get_chain();
}

void SiEffector::set_slot_effect_list(int p_slot, List<Ref<SiEffectBase>> p_effects) {
	SiEffectStream *effect = _get_global_stream(p_slot);
	effect->set_chain(p_effects);
	effect->prepare_process();
}

void SiEffector::clear_slot(int p_slot) {
	if (p_slot == 0) {
		_master_effect->initialize(0);
	} else {
		if (_global_effects[p_slot] != nullptr) {
			_global_effects[p_slot]->free();
			_free_effect_streams.push_back(_global_effects[p_slot]);
			_global_effects.write[p_slot] = nullptr;
		}
	}
}

void SiEffector::connect_effect(int p_slot, const Ref<SiEffectBase> &p_effect) {
	SiEffectStream *effect = _get_global_stream(p_slot);
	effect->get_chain().push_back(p_effect);
	p_effect->prepare_process();
}

SiEffectStream *SiEffector::create_local_effect(int p_depth, List<Ref<SiEffectBase>> p_effects) {
	SiEffectStream *effect = _alloc_stream(p_depth);
	effect->set_chain(p_effects);
	effect->prepare_process();

	if (p_depth == 0) {
		_local_effects.push_back(effect);
		return effect;
	}

	for (int i = _local_effects.size() - 1; i >= 0; i--) {
		if (_local_effects[i]->get_depth() >= p_depth) {
			_local_effects.insert(i, effect);
			return effect;
		}
	}

	_local_effects.insert(0, effect);
	return effect;
}

void SiEffector::delete_local_effect(SiEffectStream *p_effect) {
	_local_effects.erase(p_effect);
	p_effect->free();
	_free_effect_streams.push_back(p_effect);
}

void SiEffector::parse_global_effect_mml(int p_slot, String p_mml, String p_postfix) {
	SiEffectStream *effect = _get_global_stream(p_slot);
	effect->parse_mml(p_slot, p_mml, p_postfix);
}

// Processing.

void SiEffector::prepare_process() {
	// Do nothing with local effects.

	_global_effect_count = 0;
	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		_sound_chip->set_stream_slot(i, nullptr); // Reset sound chip's stream slot.

		if (_global_effects[i] != nullptr) {
			int channel_count = _global_effects[i]->prepare_process();
			if (channel_count > 0) {
				_sound_chip->set_stream_slot(i, _global_effects[i]->get_stream());
				_global_effect_count++;
			}
		}
	}

	_master_effect->prepare_process();
}

void SiEffector::begin_process() {
	// Do nothing with the master effect.

	for (SiEffectStream *effect : _local_effects) {
		effect->get_stream()->clear();
	}

	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			_global_effects[i]->get_stream()->clear();
		}
	}
}

void SiEffector::end_process() {
	for (SiEffectStream *effect : _local_effects) {
		effect->process(0, _sound_chip->get_buffer_length());
	}

	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			SiEffectStream *effect = _global_effects[i];

			if (effect->is_outputting_directly()) {
				effect->process(0, _sound_chip->get_buffer_length(), false);

				// TODO: Is this the best way to do this — to copy the buffer over?
				Vector<double> buffer = effect->get_stream()->get_buffer();
				Vector<double> output = _sound_chip->get_output_buffer();
				for (int j = 0; j < output.size(); j++) {
					output.write[j] += buffer[j];
				}
				_sound_chip->set_output_buffer(output);
			} else {
				effect->process(0, _sound_chip->get_buffer_length(), true);
			}
		}
	}

	_master_effect->process(0, _sound_chip->get_buffer_length(), false);
}

void SiEffector::reset() {
	for (SiEffectStream *effect : _local_effects) {
		effect->reset();
	}

	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			_global_effects[i]->reset();
		}
	}

	_master_effect->reset();
	_global_effects.write[0] = _master_effect;
}

//

// This function is called from SiONDriver::play() when its 2nd argument is set to true.
// If you want to connect effects by code, you have to call this first, then call connect_effect(),
// and then SiONDriver::play() with the 2nd argument set to false.
void SiEffector::initialize() {
	for (SiEffectStream *effect : _local_effects) {
		effect->free();
		_free_effect_streams.push_back(effect);
	}
	_local_effects.clear();

	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			_global_effects[i]->free();
			_free_effect_streams.push_back(_global_effects[i]);
			_global_effects.write[i] = nullptr;
		}
	}
	_global_effect_count = 0;

	_master_effect->initialize(0);
	_global_effects.write[0] = _master_effect;
}

SiEffector::SiEffector(SiOPMSoundChip *p_chip) {
	_sound_chip = p_chip;

	_master_effect = memnew(SiEffectStream(_sound_chip, _sound_chip->get_output_stream()));
	_global_effects.resize_zeroed(SiOPMSoundChip::STREAM_SEND_SIZE);
	_global_effects.write[0] = _master_effect;

	// Register default effect instances.
	// FIXME: Implement all effects and filters.

	register_effect<SiEffectAutopan>("autopan");
	register_effect<SiEffectCompressor>("comp");
	register_effect<SiEffectDistortion>("dist");
	register_effect<SiEffectDownsampler>("ds");
	register_effect<SiEffectEqualizer>("eq");
	register_effect<SiEffectSpeakerSimulator>("speaker");
	register_effect<SiEffectStereoChorus>("chorus");
	register_effect<SiEffectStereoDelay>("delay");
	register_effect<SiEffectStereoExpander>("stereo");
	register_effect<SiEffectStereoReverb>("reverb");
	register_effect<SiEffectWaveShaper>("ws");

	register_effect<SiFilterAllPass>("af");
	register_effect<SiFilterBandPass>("bf");
	register_effect<SiFilterHighBoost>("hb");
	register_effect<SiFilterHighPass>("hf");
	register_effect<SiFilterLowBoost>("lb");
	register_effect<SiFilterLowPass>("lf");
	register_effect<SiFilterNotch>("nf");
	register_effect<SiFilterPeak>("pf");
	register_effect<SiFilterVowel>("vowel");

	register_effect<SiControllableFilterHighPass>("nhf");
	register_effect<SiControllableFilterLowPass>("nlf");
}

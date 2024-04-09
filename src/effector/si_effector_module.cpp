/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effector_module.h"

#include <godot_cpp/core/memory.hpp>
#include "effector/si_effect_stream.h"
#include "processor/siopm_module.h"
#include "processor/siopm_stream.h"
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

HashMap<String, List<Ref<SiEffectBase>>> SiEffectorModule::_effect_instances;

// Effects.

template <class T>
void SiEffectorModule::register_effector(const String &p_name) {
	derived_from<T, SiEffectBase>(); // Compile-time check: Only accept SiEffectBase derivatives.

	// We don't actually use the type here, as that would make it incompatible with the collection.
	_effect_instances[p_name] = List<Ref<SiEffectBase>>();
}

Ref<SiEffectBase> SiEffectorModule::get_effector_instance(const String &p_name) {
	ERR_FAIL_COND_V_MSG(!_effect_instances.has(p_name), Ref<SiEffectBase>(), vformat("SiEffectorModule: Effect called '%s' does not exist.", p_name));

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

#define CREATE_EFFECTOR(m_type, m_name)                                 \
	if (p_name == m_name) {                                             \
		Ref<SiEffectBase> effect = create_effector_instance<m_type>();  \
		instances.push_back(effect);                                    \
		return effect;                                                  \
	}

	CREATE_EFFECTOR(SiEffectAutopan, "autopan");
	CREATE_EFFECTOR(SiEffectCompressor, "comp");
	CREATE_EFFECTOR(SiEffectDistortion, "dist");
	CREATE_EFFECTOR(SiEffectDownsampler, "ds");
	CREATE_EFFECTOR(SiEffectEqualizer, "eq");
	CREATE_EFFECTOR(SiEffectSpeakerSimulator, "speaker");
	CREATE_EFFECTOR(SiEffectStereoChorus, "chorus");
	CREATE_EFFECTOR(SiEffectStereoDelay, "delay");
	CREATE_EFFECTOR(SiEffectStereoExpander, "stereo");
	CREATE_EFFECTOR(SiEffectStereoReverb, "reverb");
	CREATE_EFFECTOR(SiEffectWaveShaper, "ws");

	// CREATE_EFFECTOR(SiFilterAllPass, "af");
	// CREATE_EFFECTOR(SiFilterBandPass, "bf");
	// CREATE_EFFECTOR(SiFilterHighBoost, "hb");
	// CREATE_EFFECTOR(SiFilterHighPass, "hf");
	// CREATE_EFFECTOR(SiFilterLowBoost, "lb");
	// CREATE_EFFECTOR(SiFilterLowPass, "lf");
	// CREATE_EFFECTOR(SiFilterNotch, "nf");
	// CREATE_EFFECTOR(SiFilterPeak, "pf");
	// CREATE_EFFECTOR(SiFilterVowel, "vowel");

	CREATE_EFFECTOR(SiControllableFilterHighPass, "nhf");
	CREATE_EFFECTOR(SiControllableFilterLowPass, "nlf");

#undef CREATE_EFFECTOR

	// This should only be possible if custom effectors are registered outside of the class.
	return Ref<SiEffectBase>();
}

template <class T>
Ref<T> SiEffectorModule::create_effector_instance() {
	Ref<T> effect;
	effect.instantiate();

	effect->set_free(false);
	effect->reset();
	return effect;
}

// Slots and connections.

SiEffectStream *SiEffectorModule::_get_global_effector(int p_slot) {
	if (_global_effects[p_slot] == nullptr) {
		SiEffectStream *effect = _alloc_stream(0);
		_global_effects.write[p_slot] = effect;
		_module->set_stream_slot(p_slot, effect->get_stream());
		_global_effect_count++;
	}

	return _global_effects[p_slot];
}

SiEffectStream *SiEffectorModule::_alloc_stream(int p_depth) {
	SiEffectStream *effect = nullptr;
	if (!_free_effect_streams.is_empty()) {
		effect = _free_effect_streams.front()->get();
		_free_effect_streams.pop_front();
	} else {
		effect = memnew(SiEffectStream(_module));
	}

	effect->initialize(p_depth);
	return effect;
}

List<Ref<SiEffectBase>> SiEffectorModule::get_slot_effect_list(int p_slot) const {
	if (!_global_effects[p_slot]) {
		return List<Ref<SiEffectBase>>();
	}
	return _global_effects[p_slot]->get_chain();
}

void SiEffectorModule::set_slot_effect_list(int p_slot, List<Ref<SiEffectBase>> p_effects) {
	SiEffectStream *effect = _get_global_effector(p_slot);
	effect->set_chain(p_effects);
	effect->prepare_process();
}

void SiEffectorModule::clear_slot(int p_slot) {
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

void SiEffectorModule::connect_effector(int p_slot, const Ref<SiEffectBase> &p_effector) {
	SiEffectStream *effect = _get_global_effector(p_slot);
	effect->get_chain().push_back(p_effector);
	p_effector->prepare_process();
}

SiEffectStream *SiEffectorModule::create_local_effect(int p_depth, List<Ref<SiEffectBase>> p_effects) {
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

void SiEffectorModule::delete_local_effect(SiEffectStream *p_effect) {
	_local_effects.erase(p_effect);
	p_effect->free();
	_free_effect_streams.push_back(p_effect);
}

void SiEffectorModule::parse_global_effect_mml(int p_slot, String p_mml, String p_postfix) {
	SiEffectStream *effect = _get_global_effector(p_slot);
	effect->parse_mml(p_slot, p_mml, p_postfix);
}

// Processing.

void SiEffectorModule::prepare_process() {
	// Do nothing with local effects.

	_global_effect_count = 0;
	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		_module->set_stream_slot(i, nullptr); // Reset module's stream slot.

		if (_global_effects[i] != nullptr) {
			int channel_count = _global_effects[i]->prepare_process();
			if (channel_count > 0) {
				_module->set_stream_slot(i, _global_effects[i]->get_stream());
				_global_effect_count++;
			}
		}
	}

	_master_effect->prepare_process();
}

void SiEffectorModule::begin_process() {
	// Do nothing with the master effect.

	for (SiEffectStream *effect : _local_effects) {
		effect->get_stream()->clear();
	}

	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			_global_effects[i]->get_stream()->clear();
		}
	}
}

void SiEffectorModule::end_process() {
	for (SiEffectStream *effect : _local_effects) {
		effect->process(0, _module->get_buffer_length());
	}

	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			SiEffectStream *effect = _global_effects[i];

			if (effect->is_outputting_directly()) {
				effect->process(0, _module->get_buffer_length(), false);

				// TODO: Is this the best way to do this — to copy the buffer over?
				Vector<double> buffer = effect->get_stream()->get_buffer();
				Vector<double> output = _module->get_output_buffer();
				for (int j = 0; j < output.size(); j++) {
					output.write[j] += buffer[j];
				}
				_module->set_output_buffer(output);
			} else {
				effect->process(0, _module->get_buffer_length(), true);
			}
		}
	}

	_master_effect->process(0, _module->get_buffer_length(), false);
}

void SiEffectorModule::reset() {
	for (SiEffectStream *effect : _local_effects) {
		effect->reset();
	}

	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		if (_global_effects[i] != nullptr) {
			_global_effects[i]->reset();
		}
	}

	_master_effect->reset();
	_global_effects.write[0] = _master_effect;
}

//

// This function is called from SiONDriver::play() when its 2nd argument is set to true.
// If you want to connect effectors by code, you have to call this first, then call connect(),
// and then SiONDriver::play() with the 2nd argument set to false.
void SiEffectorModule::initialize() {
	for (SiEffectStream *effect : _local_effects) {
		effect->free();
		_free_effect_streams.push_back(effect);
	}
	_local_effects.clear();

	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
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

SiEffectorModule::SiEffectorModule(SiOPMModule *p_module) {
	_module = p_module;

	_master_effect = memnew(SiEffectStream(_module, _module->get_output_stream()));
	_global_effects.resize_zeroed(SiOPMModule::STREAM_SEND_SIZE);
	_global_effects.write[0] = _master_effect;

	// Register default effect instances.
	// FIXME: Implement all effects and filters.

	register_effector<SiEffectAutopan>("autopan");
	register_effector<SiEffectCompressor>("comp");
	register_effector<SiEffectDistortion>("dist");
	register_effector<SiEffectDownsampler>("ds");
	register_effector<SiEffectEqualizer>("eq");
	register_effector<SiEffectSpeakerSimulator>("speaker");
	register_effector<SiEffectStereoChorus>("chorus");
	register_effector<SiEffectStereoDelay>("delay");
	register_effector<SiEffectStereoExpander>("stereo");
	register_effector<SiEffectStereoReverb>("reverb");
	register_effector<SiEffectWaveShaper>("ws");

	// register_effector<SiFilterAllPass>("af");
	// register_effector<SiFilterBandPass>("bf");
	// register_effector<SiFilterHighBoost>("hb");
	// register_effector<SiFilterHighPass>("hf");
	// register_effector<SiFilterLowBoost>("lb");
	// register_effector<SiFilterLowPass>("lf");
	// register_effector<SiFilterNotch>("nf");
	// register_effector<SiFilterPeak>("pf");
	// register_effector<SiFilterVowel>("vowel");

	register_effector<SiControllableFilterHighPass>("nhf");
	register_effector<SiControllableFilterLowPass>("nlf");
}

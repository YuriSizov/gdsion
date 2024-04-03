/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effector_module.h"

#include <godot_cpp/core/memory.hpp>
#include "effector/si_effect_base.h"
#include "effector/si_effect_stream.h"
#include "processor/siopm_module.h"
#include "processor/siopm_stream.h"
#include "templates/type_constraints.h"

HashMap<String, SiEffectorModule::EffectCollection<SiEffectBase>> SiEffectorModule::_effect_instances;

template <class T>
T *SiEffectorModule::EffectCollection<T>::get_instance() {
	SiEffectBase *effect = nullptr;

	// Check if we have free instances to reuse first.
	for (int i = 0; i < _instances.size(); i++) {
		if (_instances[i]->is_free()) {
			effect = _instances[i];

			effect->set_free(false);
			effect->initialize();
			return effect;
		}
	}

	// Failing above we allocate a new one.
	effect = memnew(T);
	_instances.push_back(effect);

	effect->set_free(false);
	effect->initialize();
	return effect;
}

template <class T>
SiEffectorModule::EffectCollection<T>::EffectCollection() {
	derived_from<T, SiEffectBase>(); // Compile-time check: Only accept SiEffectBase derivatives.
}

template <class T>
SiEffectorModule::EffectCollection<T>::~EffectCollection() {
	while (!_instances.is_empty()) {
		T *effect = _instances.back()->get();
		_instances.pop_back();
		memdelete(effect);
	}
}

// Effects.

template <class T>
void SiEffectorModule::register_effector(String p_name) {
	_effect_instances[p_name] = memnew(EffectCollection<T>);
}

SiEffectBase *SiEffectorModule::get_effector_instance(String p_name) {
	ERR_FAIL_COND_V_MSG(!_effect_instances.has(p_name), nullptr, vformat("SiEffectorModule: Effect called '%s' does not exist.", p_name));

	return _effect_instances[p_name].get_instance();
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

List<SiEffectBase *> SiEffectorModule::get_slot_effect_list(int p_slot) const {
	if (!_global_effects[p_slot]) {
		return List<SiEffectBase *>();
	}
	return _global_effects[p_slot]->get_chain();
}

void SiEffectorModule::set_slot_effect_list(int p_slot, List<SiEffectBase *> p_effects) {
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

void SiEffectorModule::connect_effector(int p_slot, SiEffectBase *p_effector) {
	SiEffectStream *effect = _get_global_effector(p_slot);
	effect->get_chain().push_back(p_effector);
	p_effector->prepare_process();
}

SiEffectStream *SiEffectorModule::create_local_effect(int p_depth, List<SiEffectBase *> p_effects) {
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

// This function is called from SiONDriver::play() with the 2nd argument set to true.
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

	// Register default effectors.
	// FIXME: Implement all effects and filters.

	// register_effector<SiEffectWaveShaper>("ws");
	// register_effector<SiEffectEqualiser>("eq");
	// register_effector<SiEffectStereoDelay>("delay");
	// register_effector<SiEffectStereoReverb>("reverb");
	// register_effector<SiEffectStereoChorus>("chorus");
	// register_effector<SiEffectAutoPan>("autopan");
	// register_effector<SiEffectDownSampler>("ds");
	// register_effector<SiEffectSpeakerSimulator>("speaker");
	// register_effector<SiEffectCompressor>("comp");
	// register_effector<SiEffectDistortion>("dist");
	// register_effector<SiEffectStereoExpander>("stereo");
	// register_effector<SiFilterVowel>("vowel");

	// register_effector<SiFilterLowPass>("lf");
	// register_effector<SiFilterHighPass>("hf");
	// register_effector<SiFilterBandPass>("bf");
	// register_effector<SiFilterNotch>("nf");
	// register_effector<SiFilterPeak>("pf");
	// register_effector<SiFilterAllPass>("af");
	// register_effector<SiFilterLowBoost>("lb");
	// register_effector<SiFilterHighBoost>("hb");

	// register_effector<SiCtrlFilterLowPass>("nlf");
	// register_effector<SiCtrlFilterHighPass>("nhf");
}

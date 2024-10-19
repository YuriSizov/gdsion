/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "sion_data.h"
#include "sion_driver.h"
#include "sion_voice.h"

#include "chip/channels/siopm_channel_base.h"
#include "chip/channels/siopm_channel_fm.h"
#include "chip/channels/siopm_channel_ks.h"
#include "chip/channels/siopm_channel_pcm.h"
#include "chip/channels/siopm_channel_sampler.h"
#include "chip/channels/siopm_operator.h"
#include "chip/siopm_channel_params.h"
#include "chip/siopm_operator_params.h"
#include "chip/siopm_ref_table.h"
#include "chip/siopm_sound_chip.h"
#include "chip/wave/siopm_wave_base.h"
#include "chip/wave/siopm_wave_pcm_data.h"
#include "chip/wave/siopm_wave_pcm_table.h"
#include "chip/wave/siopm_wave_sampler_data.h"
#include "chip/wave/siopm_wave_sampler_table.h"
#include "chip/wave/siopm_wave_table.h"
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
#include "effector/filters/si_controllable_filter_base.h"
#include "effector/filters/si_controllable_filter_high_pass.h"
#include "effector/filters/si_controllable_filter_low_pass.h"
#include "effector/filters/si_filter_all_pass.h"
#include "effector/filters/si_filter_band_pass.h"
#include "effector/filters/si_filter_base.h"
#include "effector/filters/si_filter_high_boost.h"
#include "effector/filters/si_filter_high_pass.h"
#include "effector/filters/si_filter_low_boost.h"
#include "effector/filters/si_filter_low_pass.h"
#include "effector/filters/si_filter_notch.h"
#include "effector/filters/si_filter_peak.h"
#include "effector/filters/si_filter_vowel.h"
#include "effector/si_effect_base.h"
#include "effector/si_effect_composite.h"
#include "effector/si_effector.h"
#include "events/sion_event.h"
#include "events/sion_track_event.h"
#include "sequencer/base/beats_per_minute.h"
#include "sequencer/base/mml_data.h"
#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_parser.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/base/mml_sequence_group.h"
#include "sequencer/base/mml_sequencer.h"
#include "sequencer/base/mml_system_command.h"
#include "sequencer/simml_data.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_sequencer.h"
#include "sequencer/simml_track.h"
#include "sequencer/simml_voice.h"
#include "utils/sion_voice_preset_util.h"

#include "templates/singly_linked_list.h"

using namespace godot;

void initialize_sion_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Additional SiON API classes and prerequisites.

	// NOTE: Not every class registered below is designed to be used publicly,
	// but all classes extending Godot objects need to be registered.
	// Also, all of these classes must have a constructor that has no required
	// arguments. Even if this is not a supported case for the class itself.

	{
		// Chip.

		ClassDB::register_abstract_class<SiOPMChannelBase>();
		ClassDB::register_abstract_class<SiOPMChannelFM>();
		ClassDB::register_abstract_class<SiOPMChannelKS>();
		ClassDB::register_abstract_class<SiOPMChannelPCM>();
		ClassDB::register_abstract_class<SiOPMChannelSampler>();
		ClassDB::register_abstract_class<SiOPMChannelParams>();
		ClassDB::register_abstract_class<SiOPMOperator>();
		ClassDB::register_abstract_class<SiOPMOperatorParams>();
		ClassDB::register_abstract_class<SiOPMSoundChip>();
		ClassDB::register_abstract_class<SiOPMWaveBase>();
		ClassDB::register_abstract_class<SiOPMWavePCMData>();
		ClassDB::register_abstract_class<SiOPMWavePCMTable>();
		ClassDB::register_abstract_class<SiOPMWaveSamplerData>();
		ClassDB::register_abstract_class<SiOPMWaveSamplerTable>();
		ClassDB::register_abstract_class<SiOPMWaveTable>();

		// Effector.

		ClassDB::register_abstract_class<SiEffector>();

		ClassDB::register_abstract_class<SiEffectBase>();
		ClassDB::register_abstract_class<SiControllableFilterBase>();
		ClassDB::register_abstract_class<SiFilterBase>();

		ClassDB::register_class<SiControllableFilterHighPass>();
		ClassDB::register_class<SiControllableFilterLowPass>();
		ClassDB::register_class<SiEffectAutopan>();
		ClassDB::register_class<SiEffectComposite>();
		ClassDB::register_class<SiEffectCompressor>();
		ClassDB::register_class<SiEffectDistortion>();
		ClassDB::register_class<SiEffectDownsampler>();
		ClassDB::register_class<SiEffectEqualizer>();
		ClassDB::register_class<SiEffectSpeakerSimulator>();
		ClassDB::register_class<SiEffectStereoChorus>();
		ClassDB::register_class<SiEffectStereoDelay>();
		ClassDB::register_class<SiEffectStereoExpander>();
		ClassDB::register_class<SiEffectStereoReverb>();
		ClassDB::register_class<SiEffectWaveShaper>();
		ClassDB::register_class<SiFilterAllPass>();
		ClassDB::register_class<SiFilterBandPass>();
		ClassDB::register_class<SiFilterHighBoost>();
		ClassDB::register_class<SiFilterHighPass>();
		ClassDB::register_class<SiFilterLowBoost>();
		ClassDB::register_class<SiFilterLowPass>();
		ClassDB::register_class<SiFilterNotch>();
		ClassDB::register_class<SiFilterPeak>();
		ClassDB::register_class<SiFilterVowel>();

		// Events.

		ClassDB::register_abstract_class<SiONEvent>();
		ClassDB::register_abstract_class<SiONTrackEvent>();

		// Sequencer.

		ClassDB::register_class<BeatsPerMinute>();
		ClassDB::register_abstract_class<MMLData>();
		ClassDB::register_class<MMLEvent>();
		ClassDB::register_class<MMLSequence>();
		ClassDB::register_class<MMLSequenceGroup>();
		ClassDB::register_abstract_class<MMLSequencer>();
		ClassDB::register_abstract_class<MMLSystemCommand>();
		ClassDB::register_abstract_class<SiMMLData>();
		ClassDB::register_abstract_class<SiMMLEnvelopeTable>();
		ClassDB::register_abstract_class<SiMMLSequencer>();
		ClassDB::register_abstract_class<SiMMLTrack>();
		ClassDB::register_abstract_class<SiMMLVoice>();

		// Utils.

		ClassDB::register_class<SiONVoicePresetUtil>();

		// Main SiON API classes.

		ClassDB::register_class<SiONData>();
		ClassDB::register_class<SiONDriver>();
		ClassDB::register_class<SiONVoice>();
	}

	// Initialization.

	// SUS: This is a bit ugly, but I don't have a better idea yet.
	SinglyLinkedList<int>::initialize_pool();
	SinglyLinkedList<double>::initialize_pool();

	// Initialize singletons and static members before the execution.
	MMLParser::initialize();
	MMLSequencer::initialize();
	SiOPMRefTable::initialize();
	SiMMLRefTable::initialize();
	SiMMLTrack::initialize();
}

void uninitialize_sion_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Finalization.

	// SUS: This is a bit ugly, but I don't have a better idea yet.
	SinglyLinkedList<int>::finalize_pool();
	SinglyLinkedList<double>::finalize_pool();

	// Finalize singletons and static members after the execution.
	SiOPMChannelFM::finalize_pool();
	SiMMLTrack::finalize();
	SiMMLRefTable::finalize();
	SiOPMRefTable::finalize();
	MMLSequencer::finalize();
	MMLParser::finalize();
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT gdsion_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_sion_module);
	init_obj.register_terminator(uninitialize_sion_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}

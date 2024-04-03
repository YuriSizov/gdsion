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

#include "events/sion_event.h"
#include "events/sion_track_event.h"
#include "processor/channels/siopm_channel_base.h"
#include "processor/channels/siopm_channel_fm.h"
#include "processor/siopm_channel_params.h"
#include "processor/siopm_table.h"
#include "processor/wave/siopm_wave_base.h"
#include "processor/wave/siopm_wave_pcm_data.h"
#include "processor/wave/siopm_wave_pcm_table.h"
#include "processor/wave/siopm_wave_sampler_data.h"
#include "processor/wave/siopm_wave_sampler_table.h"
#include "processor/wave/siopm_wave_table.h"
#include "sequencer/base/mml_data.h"
#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_parser.h"
#include "sequencer/base/mml_sequencer.h"
#include "sequencer/base/mml_system_command.h"
#include "sequencer/simml_data.h"
#include "sequencer/simml_sequencer.h"
#include "sequencer/simml_table.h"
#include "sequencer/simml_track.h"
#include "sequencer/simml_voice.h"
#include "utils/sion_voice_preset_util.h"

using namespace godot;

void initialize_sion_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Additional SiON API classes and prerequisites.

	// NOTE: Not every class registered below is designed to be used publicly,
	// but all classes extending Godot objects needs to be registered.
	// Also, all of these classes must have a constructor that has no required
	// arguments. Even if this is not a supported case for the class itself.

	// Events.
	ClassDB::register_abstract_class<SiONEvent>();
	ClassDB::register_abstract_class<SiONTrackEvent>();

	// Processor.
	ClassDB::register_abstract_class<SiOPMChannelBase>();
	ClassDB::register_abstract_class<SiOPMChannelFM>();
	ClassDB::register_abstract_class<SiOPMChannelParams>();
	ClassDB::register_abstract_class<SiOPMWaveBase>();
	ClassDB::register_abstract_class<SiOPMWavePCMData>();
	ClassDB::register_abstract_class<SiOPMWavePCMTable>();
	ClassDB::register_abstract_class<SiOPMWaveSamplerData>();
	ClassDB::register_abstract_class<SiOPMWaveSamplerTable>();
	ClassDB::register_abstract_class<SiOPMWaveTable>();

	// Sequencer.
	ClassDB::register_abstract_class<MMLData>();
	ClassDB::register_abstract_class<MMLEvent>();
	ClassDB::register_abstract_class<MMLSequencer>();
	ClassDB::register_abstract_class<MMLSystemCommand>();
	ClassDB::register_abstract_class<SiMMLData>();
	ClassDB::register_abstract_class<SiMMLSequencer>();
	ClassDB::register_abstract_class<SiMMLTrack>();
	ClassDB::register_abstract_class<SiMMLVoice>();

	// Utils.
	ClassDB::register_class<SiONVoicePresetUtil>();

	// Main SiON API classes.

	ClassDB::register_class<SiONData>();
	ClassDB::register_class<SiONDriver>();
	ClassDB::register_class<SiONVoice>();

	// Initialize singletons before the execution.

	MMLParser::initialize();
	MMLSequencer::initialize();
	SiOPMTable::initialize();
	SiMMLTable::initialize();
}

void uninitialize_sion_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
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

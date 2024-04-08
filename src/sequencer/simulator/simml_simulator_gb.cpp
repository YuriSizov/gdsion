/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_gb.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorGB::SiMMLSimulatorGB() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_GB, 4, memnew(SiMMLSimulatorVoiceSet(11))) {
	// Default voice set.

	for (int i = 0; i < 8; i++) {
		_default_voice_set->set_voice(i, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_PULSE + i * 2, SiOPMRefTable::PT_PSG)));
	}
	_default_voice_set->set_voice(8, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_CUSTOM, SiOPMRefTable::PT_PSG)));
	_default_voice_set->set_voice(9, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_NOISE_PULSE, SiOPMRefTable::PT_GB_NOISE)));
	_default_voice_set->set_voice(10, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_NOISE_GB_SHORT, SiOPMRefTable::PT_GB_NOISE)));
	_default_voice_set->set_init_voice_index(1);

	// Multi-channel voice sets.

	SiMMLSimulatorVoiceSet *tone_voice_set = nullptr;

	// Channels 1 and 2.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(8));
	for (int i = 0; i < 8; i++) {
		tone_voice_set->set_voice(i, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_PULSE + i * 2, SiOPMRefTable::PT_PSG)));
	}
	tone_voice_set->set_init_voice_index(4);

	_channel_voice_set.write[0] = tone_voice_set;
	_channel_voice_set.write[1] = tone_voice_set;

	// Channel 3.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(1));
	tone_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_CUSTOM, SiOPMRefTable::PT_PSG)));
	tone_voice_set->set_init_voice_index(0);

	_channel_voice_set.write[2] = tone_voice_set;

	// Channel 4.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(2));
	tone_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_NOISE_PULSE, SiOPMRefTable::PT_GB_NOISE)));
	tone_voice_set->set_voice(1, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_NOISE_GB_SHORT, SiOPMRefTable::PT_GB_NOISE)));
	tone_voice_set->set_init_voice_index(0);

	_channel_voice_set.write[3] = tone_voice_set;
}

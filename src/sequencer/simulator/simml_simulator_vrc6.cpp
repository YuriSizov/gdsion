/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_vrc6.h"

#include "sion_enums.h"

SiMMLSimulatorVRC6::SiMMLSimulatorVRC6() :
		SiMMLSimulatorBase(MT_VRC6, 4, memnew(SiMMLSimulatorVoiceSet(9))) {

	// Default voice set.

	for (int i = 0; i < 8; i++) {
		_default_voice_set->set_voice(i, memnew(SiMMLSimulatorVoice(PG_PULSE + i, PT_PSG)));
	}
	_default_voice_set->set_voice(8, memnew(SiMMLSimulatorVoice(PG_SAW_VC6, PT_PSG)));
	_default_voice_set->set_init_voice_index(7);

	// Multi-channel voice sets.

	SiMMLSimulatorVoiceSet *tone_voice_set = nullptr;

	// Channels 1 and 2.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(8));
	for (int i = 0; i < 8; i++) {
		tone_voice_set->set_voice(i, memnew(SiMMLSimulatorVoice(PG_PULSE + i, PT_PSG)));
	}
	tone_voice_set->set_init_voice_index(4);

	_channel_voice_set.write[0] = tone_voice_set;
	_channel_voice_set.write[1] = tone_voice_set;

	// Channel 3.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(1));
	tone_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(PG_SAW_VC6, PT_PSG)));
	tone_voice_set->set_init_voice_index(0);

	_channel_voice_set.write[2] = tone_voice_set;
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_sid.h"

#include "sion_enums.h"

SiMMLSimulatorSID::SiMMLSimulatorSID() :
		SiMMLSimulatorBase(MT_SID, 3, memnew(SiMMLSimulatorVoiceSet(11))) {

	// Note that the original code configures this simulator to 3 channels, but sets 4.
	// The code seems to be copied and pasted from APU/GB, which are almost identical,
	// but differ in specific type values.
	// Initial thought was that this should be 4 channels, just like APU/GB. But upon a closer
	// examination it became apparent that the 4th channel is set to the exact values copied
	// from the GB sim. This is very suspicious, so we assume that the intent is to have
	// 3 channels and the extra one was just a copy-paste error.
	// Also note that the 3rd channel is set to the same values as the GB sim, but that
	// breaks the general pattern for how the default and the auxiliary voice sets are
	// configured in these three classes. Might be a mistake as well.

	// Default voice set.

	for (int i = 0; i < 8; i++) {
		_default_voice_set->set_voice(i, memnew(SiMMLSimulatorVoice(PG_PULSE + i, PT_PSG)));
	}
	_default_voice_set->set_voice(8, memnew(SiMMLSimulatorVoice(PG_TRIANGLE, PT_PSG)));
	_default_voice_set->set_voice(9, memnew(SiMMLSimulatorVoice(PG_SAW_UP, PT_PSG)));
	_default_voice_set->set_voice(10, memnew(SiMMLSimulatorVoice(PG_NOISE_PULSE, PT_OPM_NOISE)));
	_default_voice_set->set_init_voice_index(1);

	// Multi-channel voice sets.

	SiMMLSimulatorVoiceSet *tone_voice_set = nullptr;

	// Channels 1 and 2.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(8));
	for (int i = 0; i < 8; i++) {
		tone_voice_set->set_voice(i, memnew(SiMMLSimulatorVoice(PG_PULSE + i * 2, PT_PSG)));
	}
	tone_voice_set->set_init_voice_index(4);

	_channel_voice_set.write[0] = tone_voice_set;
	_channel_voice_set.write[1] = tone_voice_set;

	// Channel 3.

	tone_voice_set = memnew(SiMMLSimulatorVoiceSet(1));
	tone_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(PG_CUSTOM, PT_PSG)));
	tone_voice_set->set_init_voice_index(0);

	_channel_voice_set.write[2] = tone_voice_set;
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_base_fm.h"

#include "chip/channels/siopm_channel_base.h"
#include "chip/siopm_channel_params.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/simml_track.h"
#include "sequencer/simml_voice.h"

// NOTE: There is some code duplication with SiMMLChannelSettings. Might be worth to refactor?

MMLSequence *SiMMLSimulatorBaseFM::_select_fm_tone(SiMMLTrack *p_track, int p_voice_index) {
	if (p_voice_index == -1) {
		return nullptr;
	}

	int voice_index = p_voice_index;
	if (voice_index < 0 || voice_index >= SiMMLRefTable::VOICE_MAX) {
		voice_index = 0;
	}

	Ref<SiMMLVoice> voice = SiMMLRefTable::get_instance()->get_voice(voice_index);
	if (voice.is_null()) {
		return nullptr;
	}

	if (voice->should_update_track_parameters()) {
		voice->update_track_voice(p_track);
		return nullptr;
	}

	// This module only changes channel params, but not track params.
	p_track->get_channel()->set_channel_params(voice->get_channel_params(), false, false);
	p_track->reset_volume_offset();

	MMLSequence *init_sequence = voice->get_channel_params()->get_init_sequence();
	if (init_sequence->is_empty()) {
		return nullptr;
	}

	return init_sequence;
}

MMLSequence *SiMMLSimulatorBaseFM::select_tone(SiMMLTrack *p_track, int p_voice_index) {
	return _select_fm_tone(p_track, p_voice_index);
}

SiMMLSimulatorBaseFM::SiMMLSimulatorBaseFM(SiONModuleType p_type, int p_channel_num) :
		SiMMLSimulatorBase(p_type, p_channel_num, memnew(SiMMLSimulatorVoiceSet(512, PG_SINE)), false) {
}

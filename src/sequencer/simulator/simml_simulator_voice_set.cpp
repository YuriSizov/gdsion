/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_voice_set.h"

#include "chip/siopm_ref_table.h"
#include "chip/wave/siopm_wave_table.h"
#include "sequencer/simml_channel_settings.h"

int SiMMLSimulatorVoiceSet::get_voices_amount() const {
	return _voices.size();
}

SiMMLSimulatorVoice *SiMMLSimulatorVoiceSet::get_voice(int p_index) {
	ERR_FAIL_INDEX_V(p_index, _voices.size(), nullptr);
	return _voices[p_index];
}

void SiMMLSimulatorVoiceSet::set_voice(int p_index, SiMMLSimulatorVoice *p_voice) {
	ERR_FAIL_INDEX(p_index, _voices.size());
	_voices.write[p_index] = p_voice;
}

SiMMLSimulatorVoice *SiMMLSimulatorVoiceSet::get_init_voice() const {
	ERR_FAIL_INDEX_V(_init_voice_index, _voices.size(), nullptr);
	return _voices[_init_voice_index];
}

SiMMLSimulatorVoiceSet::SiMMLSimulatorVoiceSet(int p_length, int p_offset, int p_channel_type) {
	_voices.resize_zeroed(p_length);

	if (p_offset == -1) {
		return;
	}

	int channel_type = p_channel_type;
	if (channel_type == -1) {
		channel_type = SiMMLChannelSettings::SELECT_TONE_FM;
	}

	for (int i = 0; i < p_length; i++) {
		SiONPitchTableType pt_type = SiOPMRefTable::get_instance()->get_wave_table(i + p_offset)->get_default_pitch_table_type();
		_voices.write[i] = memnew(SiMMLSimulatorVoice(i + p_offset, pt_type, channel_type));
	}
}

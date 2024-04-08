/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_voice.h"

#include "sequencer/simml_channel_settings.h"

SiMMLSimulatorVoice::SiMMLSimulatorVoice(int p_pg_type, SiONPitchTableType p_pt_type, int p_channel_type) {
	_pg_type = p_pg_type;
	_pt_type = p_pt_type;
	_channel_type = (p_channel_type == -1 ? SiMMLChannelSettings::SELECT_TONE_FM : p_channel_type);
}

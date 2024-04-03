/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_pcm.h"

#include "processor/channels/siopm_channel_manager.h"
#include "processor/siopm_table.h"
#include "sequencer/simml_table.h"

SiMMLSimulatorPCM::SiMMLSimulatorPCM() :
		SiMMLSimulatorBase(SiMMLTable::MT_PCM, 1, memnew(SiMMLSimulatorVoiceSet(SiOPMChannelManager::CT_CHANNEL_PCM, 1, SiOPMTable::PG_PCM)), false) {
	// Empty.
}

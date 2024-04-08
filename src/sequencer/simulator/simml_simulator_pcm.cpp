/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_pcm.h"

#include "processor/channels/siopm_channel_manager.h"
#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorPCM::SiMMLSimulatorPCM() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_PCM, 1, memnew(SiMMLSimulatorVoiceSet(SiOPMChannelManager::CT_CHANNEL_PCM, 1, SiOPMRefTable::PG_PCM)), false) {
	// Empty.
}

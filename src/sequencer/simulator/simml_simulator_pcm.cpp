/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_pcm.h"

#include "sion_enums.h"
#include "processor/channels/siopm_channel_manager.h"

SiMMLSimulatorPCM::SiMMLSimulatorPCM() :
		SiMMLSimulatorBase(MT_PCM, 1, memnew(SiMMLSimulatorVoiceSet(SiOPMChannelManager::CT_CHANNEL_PCM, 1, PG_PCM)), false) {
	// Empty.
}

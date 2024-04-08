/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_sampler.h"

#include "sion_enums.h"
#include "processor/channels/siopm_channel_manager.h"

SiMMLSimulatorSampler::SiMMLSimulatorSampler() :
		SiMMLSimulatorBase(MT_SAMPLE, 1, memnew(SiMMLSimulatorVoiceSet(SiOPMChannelManager::CT_CHANNEL_SAMPLER, 1, 0)), false) {
	// Empty.
}

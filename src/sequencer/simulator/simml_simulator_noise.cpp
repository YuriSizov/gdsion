/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_noise.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorNoise::SiMMLSimulatorNoise() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_NOISE, 1, memnew(SiMMLSimulatorVoiceSet(16, SiOPMRefTable::PG_NOISE_WHITE))) {
	// Empty.
}

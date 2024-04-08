/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_noise.h"

#include "sion_enums.h"

SiMMLSimulatorNoise::SiMMLSimulatorNoise() :
		SiMMLSimulatorBase(MT_NOISE, 1, memnew(SiMMLSimulatorVoiceSet(16, PG_NOISE_WHITE))) {
	// Empty.
}

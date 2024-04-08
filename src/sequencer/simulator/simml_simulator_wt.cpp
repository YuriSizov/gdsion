/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_wt.h"

#include "sion_enums.h"

SiMMLSimulatorWT::SiMMLSimulatorWT() :
		SiMMLSimulatorBase(MT_CUSTOM, 1, memnew(SiMMLSimulatorVoiceSet(256, PG_CUSTOM))) {
	// Empty.
}

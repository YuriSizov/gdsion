/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ramp.h"

#include "sion_enums.h"

SiMMLSimulatorRamp::SiMMLSimulatorRamp() :
		SiMMLSimulatorBase(MT_RAMP, 1, memnew(SiMMLSimulatorVoiceSet(128, PG_RAMP))) {
	// Empty.
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_pulse.h"

#include "sion_enums.h"

SiMMLSimulatorPulse::SiMMLSimulatorPulse() :
		SiMMLSimulatorBase(MT_PULSE, 1, memnew(SiMMLSimulatorVoiceSet(32, PG_PULSE))) {
	// Empty.
}

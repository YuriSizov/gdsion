/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_siopm.h"

#include "sion_enums.h"

SiMMLSimulatorSiOPM::SiMMLSimulatorSiOPM() :
		SiMMLSimulatorBase(MT_ALL, 1, memnew(SiMMLSimulatorVoiceSet(512, PG_SINE))) {
	// Empty.
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ramp.h"

#include "processor/siopm_table.h"
#include "sequencer/simml_table.h"

SiMMLSimulatorRamp::SiMMLSimulatorRamp() :
		SiMMLSimulatorBase(SiMMLTable::MT_RAMP, 1, memnew(SiMMLSimulatorVoiceSet(128, SiOPMTable::PG_RAMP))) {
	// Empty.
}

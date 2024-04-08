/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ramp.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorRamp::SiMMLSimulatorRamp() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_RAMP, 1, memnew(SiMMLSimulatorVoiceSet(128, SiOPMRefTable::PG_RAMP))) {
	// Empty.
}

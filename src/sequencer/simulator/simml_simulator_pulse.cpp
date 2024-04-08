/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_pulse.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorPulse::SiMMLSimulatorPulse() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_PULSE, 1, memnew(SiMMLSimulatorVoiceSet(32, SiOPMRefTable::PG_PULSE))) {
	// Empty.
}

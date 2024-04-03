/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_pulse.h"

#include "processor/siopm_table.h"
#include "sequencer/simml_table.h"

SiMMLSimulatorPulse::SiMMLSimulatorPulse() :
		SiMMLSimulatorBase(SiMMLTable::MT_PULSE, 1, memnew(SiMMLSimulatorVoiceSet(32, SiOPMTable::PG_PULSE))) {
	// Empty.
}

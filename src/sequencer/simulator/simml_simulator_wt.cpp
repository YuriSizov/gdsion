/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_wt.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorWT::SiMMLSimulatorWT() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_CUSTOM, 1, memnew(SiMMLSimulatorVoiceSet(256, SiOPMRefTable::PG_CUSTOM))) {
	// Empty.
}

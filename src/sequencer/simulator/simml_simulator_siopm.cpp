/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_siopm.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorSiOPM::SiMMLSimulatorSiOPM() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_ALL, 1, memnew(SiMMLSimulatorVoiceSet(512, SiOPMRefTable::PG_SINE))) {
	// Empty.
}

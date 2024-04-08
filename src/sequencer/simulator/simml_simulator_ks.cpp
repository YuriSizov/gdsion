/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ks.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorKS::SiMMLSimulatorKS() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_KS, 1, memnew(SiMMLSimulatorVoiceSet(512, SiOPMRefTable::PG_SINE)), false) {
	// Empty.
}

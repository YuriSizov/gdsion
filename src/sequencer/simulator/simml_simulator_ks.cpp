/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ks.h"

#include "processor/siopm_table.h"
#include "sequencer/simml_table.h"

SiMMLSimulatorKS::SiMMLSimulatorKS() :
		SiMMLSimulatorBase(SiMMLTable::MT_KS, 1, memnew(SiMMLSimulatorVoiceSet(512, SiOPMTable::PG_SINE)), false) {
	// Empty.
}

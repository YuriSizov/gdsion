/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opll.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMOPLL::SiMMLSimulatorFMOPLL() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM_OPLL, 1) {
	// Empty.
}

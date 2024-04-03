/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opl3.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMOPL3::SiMMLSimulatorFMOPL3() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM_OPL3, 1) {
	// Empty.
}

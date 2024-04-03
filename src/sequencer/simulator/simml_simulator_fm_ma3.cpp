/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_ma3.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMMA3::SiMMLSimulatorFMMA3() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM_MA3, 1) {
	// Empty.
}

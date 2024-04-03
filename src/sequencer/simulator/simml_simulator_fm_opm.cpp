/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opm.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMOPM::SiMMLSimulatorFMOPM() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM_OPM, 1) {
	// Empty.
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_siopm.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMSiOPM::SiMMLSimulatorFMSiOPM() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM, 1) {
	// Empty.
}

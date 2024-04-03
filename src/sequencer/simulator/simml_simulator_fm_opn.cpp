/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opn.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMOPN::SiMMLSimulatorFMOPN() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM_OPN, 1) {
	// Empty.
}

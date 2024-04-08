/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opn.h"

#include "sequencer/simml_ref_table.h"

SiMMLSimulatorFMOPN::SiMMLSimulatorFMOPN() :
		SiMMLSimulatorBaseFM(SiMMLRefTable::MT_FM_OPN, 1) {
	// Empty.
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opna.h"

#include "sequencer/simml_table.h"

SiMMLSimulatorFMOPNA::SiMMLSimulatorFMOPNA() :
		SiMMLSimulatorBaseFM(SiMMLTable::MT_FM_OPNA, 1) {
	// Empty.
}

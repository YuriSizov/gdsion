/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opna.h"

#include "sequencer/simml_ref_table.h"

SiMMLSimulatorFMOPNA::SiMMLSimulatorFMOPNA() :
		SiMMLSimulatorBaseFM(SiMMLRefTable::MT_FM_OPNA, 1) {
	// Empty.
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_opm.h"

#include "sequencer/simml_ref_table.h"

SiMMLSimulatorFMOPM::SiMMLSimulatorFMOPM() :
		SiMMLSimulatorBaseFM(SiMMLRefTable::MT_FM_OPM, 1) {
	// Empty.
}

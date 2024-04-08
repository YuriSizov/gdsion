/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_fm_siopm.h"

#include "sequencer/simml_ref_table.h"

SiMMLSimulatorFMSiOPM::SiMMLSimulatorFMSiOPM() :
		SiMMLSimulatorBaseFM(SiMMLRefTable::MT_FM, 1) {
	// Empty.
}

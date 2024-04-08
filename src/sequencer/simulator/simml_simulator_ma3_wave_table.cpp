/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ma3_wave_table.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorMA3WaveTable::SiMMLSimulatorMA3WaveTable() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_MA3, 1, memnew(SiMMLSimulatorVoiceSet(32, SiOPMRefTable::PG_MA3_WAVE))) {
	// Empty.
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ma3_wave_table.h"

#include "processor/siopm_table.h"
#include "sequencer/simml_table.h"

SiMMLSimulatorMA3WaveTable::SiMMLSimulatorMA3WaveTable() :
		SiMMLSimulatorBase(SiMMLTable::MT_MA3, 1, memnew(SiMMLSimulatorVoiceSet(32, SiOPMTable::PG_MA3_WAVE))) {
	// Empty.
}

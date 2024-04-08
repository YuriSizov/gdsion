/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ma3_wave_table.h"

#include "sion_enums.h"

SiMMLSimulatorMA3WaveTable::SiMMLSimulatorMA3WaveTable() :
		SiMMLSimulatorBase(MT_MA3, 1, memnew(SiMMLSimulatorVoiceSet(32, PG_MA3_WAVE))) {
	// Empty.
}

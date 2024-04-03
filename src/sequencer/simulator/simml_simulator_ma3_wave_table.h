/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_MA3_WAVE_TABLE_H
#define SIMML_SIMULATOR_MA3_WAVE_TABLE_H

#include "sequencer/simulator/simml_simulator_base.h"

// Simulator of a single-operator sound generator of YAMAHA MA3 waveforms
class SiMMLSimulatorMA3WaveTable : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorMA3WaveTable();
	~SiMMLSimulatorMA3WaveTable() {}
};

#endif // SIMML_SIMULATOR_MA3_WAVE_TABLE_H

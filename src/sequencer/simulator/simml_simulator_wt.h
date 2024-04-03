/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_WT_H
#define SIMML_SIMULATOR_WT_H

#include "sequencer/simulator/simml_simulator_base.h"

// Simulator of custom waveform single-operator sound generator.
class SiMMLSimulatorWT : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorWT();
	~SiMMLSimulatorWT() {}
};

#endif // SIMML_SIMULATOR_WT_H

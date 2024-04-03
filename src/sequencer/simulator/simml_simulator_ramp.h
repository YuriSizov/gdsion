/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_RAMP_H
#define SIMML_SIMULATOR_RAMP_H

#include "sequencer/simulator/simml_simulator_base.h"

// Simulator of ramp waveform single-operator sound generator.
class SiMMLSimulatorRamp : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorRamp();
	~SiMMLSimulatorRamp() {}
};

#endif // SIMML_SIMULATOR_RAMP_H

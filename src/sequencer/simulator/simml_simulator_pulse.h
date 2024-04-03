/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_PULSE_H
#define SIMML_SIMULATOR_PULSE_H

#include "sequencer/simulator/simml_simulator_base.h"

// Simulator of pulse waveform single-operator sound generator.
class SiMMLSimulatorPulse : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorPulse();
	~SiMMLSimulatorPulse() {}
};

#endif // SIMML_SIMULATOR_PULSE_H

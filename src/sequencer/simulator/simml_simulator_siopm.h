/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_SIOPM_H
#define SIMML_SIMULATOR_SIOPM_H

#include "sequencer/simulator/simml_simulator_base.h"

// Simulator of all SiOPM waveform single-operator sound generator.
class SiMMLSimulatorSiOPM : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorSiOPM();
	~SiMMLSimulatorSiOPM() {}
};

#endif // SIMML_SIMULATOR_SIOPM_H

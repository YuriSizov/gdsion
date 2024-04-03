/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_NOISE_H
#define SIMML_SIMULATOR_NOISE_H

#include "sequencer/simulator/simml_simulator_base.h"

// Noise generator simulator
class SiMMLSimulatorNoise : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorNoise();
	~SiMMLSimulatorNoise() {}
};

#endif // SIMML_SIMULATOR_NOISE_H

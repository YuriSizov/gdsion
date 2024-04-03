/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_SAMPLER_H
#define SIMML_SIMULATOR_SAMPLER_H

#include "sequencer/simulator/simml_simulator_base.h"

// Simple sampler simulator.
class SiMMLSimulatorSampler : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorSampler();
	~SiMMLSimulatorSampler() {}
};

#endif // SIMML_SIMULATOR_SAMPLER_H

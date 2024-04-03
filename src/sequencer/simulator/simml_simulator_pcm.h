/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_PCM_H
#define SIMML_SIMULATOR_PCM_H

#include "sequencer/simulator/simml_simulator_base.h"

// PCM sound module simulator.
class SiMMLSimulatorPCM : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorPCM();
	~SiMMLSimulatorPCM() {}
};

#endif // SIMML_SIMULATOR_PCM_H

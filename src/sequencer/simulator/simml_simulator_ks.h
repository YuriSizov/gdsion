/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_KS_H
#define SIMML_SIMULATOR_KS_H

#include "sequencer/simulator/simml_simulator_base.h"

// Physical modeling guitar simulator.
class SiMMLSimulatorKS : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorKS();
	~SiMMLSimulatorKS() {}
};

#endif // SIMML_SIMULATOR_KS_H

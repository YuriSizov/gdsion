/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_APU_H
#define SIMML_SIMULATOR_APU_H

#include "sequencer/simulator/simml_simulator_base.h"

// Nintendo Entertainment System Simulator
class SiMMLSimulatorAPU : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorAPU();
	~SiMMLSimulatorAPU() {}
};

#endif // SIMML_SIMULATOR_APU_H

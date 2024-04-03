/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_GB_H
#define SIMML_SIMULATOR_GB_H

#include "sequencer/simulator/simml_simulator_base.h"

// Nintendo Gameboy simulator.
class SiMMLSimulatorGB : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorGB();
	~SiMMLSimulatorGB() {}
};

#endif // SIMML_SIMULATOR_GB_H

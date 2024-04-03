/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_PSG_H
#define SIMML_SIMULATOR_PSG_H

#include "sequencer/simulator/simml_simulator_base.h"

// Texas Instrments AY-3-8910 simulator.
class SiMMLSimulatorPSG : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorPSG();
	~SiMMLSimulatorPSG() {}
};

#endif // SIMML_SIMULATOR_PSG_H

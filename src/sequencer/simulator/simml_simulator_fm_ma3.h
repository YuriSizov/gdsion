/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_FM_MA3_H
#define SIMML_SIMULATOR_FM_MA3_H

#include "sequencer/simulator/simml_simulator_base_fm.h"

// YAMAHA YMU762 simulator.
class SiMMLSimulatorFMMA3 : public SiMMLSimulatorBaseFM {

public:
	SiMMLSimulatorFMMA3();
	~SiMMLSimulatorFMMA3() {}
};

#endif // SIMML_SIMULATOR_FM_MA3_H

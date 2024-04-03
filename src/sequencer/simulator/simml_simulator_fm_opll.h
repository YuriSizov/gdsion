/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_FM_OPLL_H
#define SIMML_SIMULATOR_FM_OPLL_H

#include "sequencer/simulator/simml_simulator_base_fm.h"

// YAMAHA YM2413 simulator.
class SiMMLSimulatorFMOPLL : public SiMMLSimulatorBaseFM {

public:
	SiMMLSimulatorFMOPLL();
	~SiMMLSimulatorFMOPLL() {}
};

#endif // SIMML_SIMULATOR_FM_OPLL_H

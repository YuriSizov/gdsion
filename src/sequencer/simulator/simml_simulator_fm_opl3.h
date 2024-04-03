/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_FM_OPL3_H
#define SIMML_SIMULATOR_FM_OPL3_H

#include "sequencer/simulator/simml_simulator_base_fm.h"

// YAMAHA YM3812 simulator.
class SiMMLSimulatorFMOPL3 : public SiMMLSimulatorBaseFM {

public:
	SiMMLSimulatorFMOPL3();
	~SiMMLSimulatorFMOPL3() {}
};

#endif // SIMML_SIMULATOR_FM_OPL3_H

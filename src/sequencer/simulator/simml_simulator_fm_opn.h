/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_FM_OPN_H
#define SIMML_SIMULATOR_FM_OPN_H

#include "sequencer/simulator/simml_simulator_base_fm.h"

// YAMAHA YM2203 simulator.
class SiMMLSimulatorFMOPN : public SiMMLSimulatorBaseFM {

public:
	SiMMLSimulatorFMOPN();
	~SiMMLSimulatorFMOPN() {}
};

#endif // SIMML_SIMULATOR_FM_OPN_H

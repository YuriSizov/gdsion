/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_FM_OPM_H
#define SIMML_SIMULATOR_FM_OPM_H

#include "sequencer/simulator/simml_simulator_base_fm.h"

// YAMAHA YM2151 simulator.
class SiMMLSimulatorFMOPM : public SiMMLSimulatorBaseFM {

public:
	SiMMLSimulatorFMOPM();
	~SiMMLSimulatorFMOPM() {}
};

#endif // SIMML_SIMULATOR_FM_OPM_H

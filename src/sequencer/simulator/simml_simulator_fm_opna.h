/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_FM_OPNA_H
#define SIMML_SIMULATOR_FM_OPNA_H

#include "sequencer/simulator/simml_simulator_base_fm.h"

// YAMAHA YM2608 simulator.
class SiMMLSimulatorFMOPNA : public SiMMLSimulatorBaseFM {

public:
	SiMMLSimulatorFMOPNA();
	~SiMMLSimulatorFMOPNA() {}
};

#endif // SIMML_SIMULATOR_FM_OPNA_H

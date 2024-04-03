/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_SID_H
#define SIMML_SIMULATOR_SID_H

#include "sequencer/simulator/simml_simulator_base.h"

// MOS Tech 8580 SID chip simulator.
class SiMMLSimulatorSID : public SiMMLSimulatorBase {

public:
	SiMMLSimulatorSID();
	~SiMMLSimulatorSID() {}
};

#endif // SIMML_SIMULATOR_SID_H

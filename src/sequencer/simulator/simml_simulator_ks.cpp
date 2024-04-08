/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_ks.h"

#include "sion_enums.h"

SiMMLSimulatorKS::SiMMLSimulatorKS() :
		SiMMLSimulatorBase(MT_KS, 1, memnew(SiMMLSimulatorVoiceSet(512, PG_SINE)), false) {
	// Empty.
}

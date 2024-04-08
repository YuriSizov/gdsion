/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_psg.h"

#include "sion_enums.h"

SiMMLSimulatorPSG::SiMMLSimulatorPSG() :
		SiMMLSimulatorBase(MT_PSG, 3, memnew(SiMMLSimulatorVoiceSet(2))) {
	_default_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(PG_SQUARE,      PT_PSG)));
	_default_voice_set->set_voice(1, memnew(SiMMLSimulatorVoice(PG_NOISE_PULSE, PT_PSG_NOISE)));
}

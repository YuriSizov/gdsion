/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_psg.h"

#include "processor/siopm_table.h"
#include "sequencer/simml_table.h"

SiMMLSimulatorPSG::SiMMLSimulatorPSG() :
		SiMMLSimulatorBase(SiMMLTable::MT_PSG, 3, memnew(SiMMLSimulatorVoiceSet(2))) {
	_default_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(SiOPMTable::PG_SQUARE,      SiOPMTable::PT_PSG)));
	_default_voice_set->set_voice(1, memnew(SiMMLSimulatorVoice(SiOPMTable::PG_NOISE_PULSE, SiOPMTable::PT_PSG_NOISE)));
}

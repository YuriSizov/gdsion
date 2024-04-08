/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_psg.h"

#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"

SiMMLSimulatorPSG::SiMMLSimulatorPSG() :
		SiMMLSimulatorBase(SiMMLRefTable::MT_PSG, 3, memnew(SiMMLSimulatorVoiceSet(2))) {
	_default_voice_set->set_voice(0, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_SQUARE,      SiOPMRefTable::PT_PSG)));
	_default_voice_set->set_voice(1, memnew(SiMMLSimulatorVoice(SiOPMRefTable::PG_NOISE_PULSE, SiOPMRefTable::PT_PSG_NOISE)));
}

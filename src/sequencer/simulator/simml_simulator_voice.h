/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_VOICE_H
#define SIMML_SIMULATOR_VOICE_H

#include "sion_enums.h"

class SiMMLSimulatorVoice {

	int _pg_type = SiONPulseGeneratorType::PG_SINE;
	SiONPitchTableType _pt_type = SiONPitchTableType::PT_OPM;
	int _channel_type = 0;

public:
	int get_pg_type() const { return _pg_type; }
	SiONPitchTableType get_pt_type() const { return _pt_type; }
	int get_channel_type() const { return _channel_type; }

	SiMMLSimulatorVoice(int p_pg_type, SiONPitchTableType p_pt_type, int p_channel_type = -1);
	~SiMMLSimulatorVoice() {}
};

#endif // SIMML_SIMULATOR_VOICE_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_VOICE_SET_H
#define SIMML_SIMULATOR_VOICE_SET_H

#include <godot_cpp/templates/vector.hpp>
#include "sequencer/simulator/simml_simulator_voice.h"

using namespace godot;

class SiMMLSimulatorVoiceSet {

	Vector<SiMMLSimulatorVoice *> _voices;
	int _init_voice_index = 0;

public:
	int get_voices_amount() const;
	SiMMLSimulatorVoice *get_voice(int p_index);
	void set_voice(int p_index, SiMMLSimulatorVoice *p_voice);

	int get_init_voice_index() const { return _init_voice_index; }
	void set_init_voice_index(int p_index) { _init_voice_index = p_index; }
	SiMMLSimulatorVoice *get_init_voice() const;

	// Offset > -1 sets all voice instances.
	SiMMLSimulatorVoiceSet(int p_length, int p_offset = -1, int p_channel_type = -1);
	~SiMMLSimulatorVoiceSet() {}
};

#endif // SIMML_SIMULATOR_VOICE_SET_H

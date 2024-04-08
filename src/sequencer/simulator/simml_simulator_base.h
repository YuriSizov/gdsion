/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_BASE_H
#define SIMML_SIMULATOR_BASE_H

#include <godot_cpp/templates/vector.hpp>
#include "sequencer/simulator/simml_simulator_voice_set.h"

using namespace godot;

class MMLSequence;
class SiMMLTrack;
enum SiONModuleType : int;

// Base class of all module simulators which control SiMMLTrack (not SiOPMChannel) to simulate various modules.
class SiMMLSimulatorBase {

protected:
	SiONModuleType _type = (SiONModuleType)5;

	// Tables converting from MML voice number to SiOPM pgType for each channel, if the table is different for each channel.
	Vector<SiMMLSimulatorVoiceSet *> _channel_voice_set;
	// Default table converting from MML voice number to SiOPM pulse generator type
	SiMMLSimulatorVoiceSet *_default_voice_set = nullptr;

	bool _is_suitable_for_fm_voice = true;
	int _default_operator_count = 1;

	MMLSequence *_select_single_wave_tone(SiMMLTrack *p_track, int p_voice_index);
	void _update_channel_instance(SiMMLTrack *p_track, int p_buffer_index, SiMMLSimulatorVoiceSet *p_voice_set);

public:
	virtual int initialize_tone(SiMMLTrack *p_track, int p_channel_num, int p_buffer_index);
	virtual MMLSequence *select_tone(SiMMLTrack *p_track, int p_voice_index);

	SiMMLSimulatorBase(SiONModuleType p_type, int p_channel_num, SiMMLSimulatorVoiceSet *p_default_voice_set, bool p_suitable_for_fm_voice = true);
};

#endif // SIMML_SIMULATOR_BASE_H

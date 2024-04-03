/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_simulator_base.h"

#include "processor/channels/siopm_channel_base.h"
#include "processor/channels/siopm_channel_manager.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/simml_track.h"
#include "sequencer/simulator/simml_simulator_voice.h"

// NOTE: There is some code duplication with SiMMLChannelSettings. Might be worth to refactor?

MMLSequence *SiMMLSimulatorBase::_select_single_wave_tone(SiMMLTrack *p_track, int p_voice_index) {
	if (p_voice_index == -1) {
		return nullptr;
	}

	int channel_num = p_track->get_channel_number();
	SiMMLSimulatorVoiceSet *voice_set = _default_voice_set;
	if (channel_num >= 0 && channel_num < _channel_voice_set.size() && _channel_voice_set[channel_num]) {
		voice_set = _channel_voice_set[channel_num];
	}

	int voice_index = p_voice_index;
	if (voice_index < 0 || voice_index >= voice_set->get_voices_amount()) {
		voice_index = voice_set->get_init_voice_index();
	}

	SiMMLSimulatorVoice *voice = voice_set->get_voice(voice_index);
	p_track->get_channel()->set_types(voice->get_pg_type(), voice->get_pt_type());

	// Different implementations may return something.
	return nullptr;
}

void SiMMLSimulatorBase::_update_channel_instance(SiMMLTrack *p_track, int p_buffer_index, SiMMLSimulatorVoiceSet *p_voice_set) {
	SiMMLSimulatorVoice *default_voice = p_voice_set->get_init_voice();
	SiOPMChannelManager::ChannelType default_channel_type = (SiOPMChannelManager::ChannelType)default_voice->get_channel_type();

	if (!p_track->get_channel()) {
		// Create a new channel.
		SiOPMChannelBase *channel = SiOPMChannelManager::create_channel(default_channel_type, nullptr, p_buffer_index);
		p_track->set_channel(channel);

	} else if (p_track->get_channel()->get_channel_type() != default_channel_type) {
		// Update the channel type.
		SiOPMChannelBase *old_channel = p_track->get_channel();
		SiOPMChannelBase *channel = SiOPMChannelManager::create_channel(default_channel_type, old_channel, p_buffer_index);
		p_track->set_channel(channel);

		SiOPMChannelManager::delete_channel(old_channel);

	} else {
		// Just initialize the channel.
		p_track->get_channel()->initialize(p_track->get_channel(), p_buffer_index);
		p_track->reset_volume_offset();
	}
}

int SiMMLSimulatorBase::initialize_tone(SiMMLTrack *p_track, int p_channel_num, int p_buffer_index) {
	SiMMLSimulatorVoiceSet *voice_set = _default_voice_set;
	int channel_num_restricted = p_channel_num;

	if (p_channel_num >= 0 && p_channel_num < _channel_voice_set.size() && _channel_voice_set[p_channel_num]) {
		voice_set = _channel_voice_set[p_channel_num];
	} else {
		channel_num_restricted = 0;
	}

	_update_channel_instance(p_track, p_buffer_index, voice_set);

	p_track->set_channel_number(p_channel_num < 0 ? -1 : p_channel_num);
	// Channel requires restricted number.
	p_track->get_channel()->set_channel_number(channel_num_restricted);
	p_track->get_channel()->set_algorithm(_default_operator_count, 0);

	select_tone(p_track, voice_set->get_init_voice_index());

	return (p_channel_num == -1 ? -1 : voice_set->get_init_voice_index());
}

MMLSequence *SiMMLSimulatorBase::select_tone(SiMMLTrack *p_track, int p_voice_index) {
	return _select_single_wave_tone(p_track, p_voice_index);
}

SiMMLSimulatorBase::SiMMLSimulatorBase(SiMMLTable::ModuleType p_type, int p_channel_num, SiMMLSimulatorVoiceSet *p_default_voice_set, bool p_suitable_for_fm_voice) {
	_type = p_type;
	_is_suitable_for_fm_voice = p_suitable_for_fm_voice;
	_channel_voice_set.resize_zeroed(p_channel_num);
	_default_voice_set = p_default_voice_set;
}

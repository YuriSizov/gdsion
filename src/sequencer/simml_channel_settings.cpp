/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_channel_settings.h"

#include "processor/channels/siopm_channel_base.h"
#include "processor/siopm_channel_params.h"
#include "processor/siopm_ref_table.h"
#include "processor/wave/siopm_wave_table.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_track.h"
#include "sequencer/simml_voice.h"

int SiMMLChannelSettings::initialize_tone(SiMMLTrack *p_track, int p_channel_num, int p_buffer_index) {
	// Prepare the track.

	if (!p_track->get_channel()) {
		// Create a new channel.
		SiOPMChannelBase *channel = SiOPMChannelManager::create_channel(_channel_type, nullptr, p_buffer_index);
		p_track->set_channel(channel);

	} else if (p_track->get_channel()->get_channel_type() != _channel_type) {
		// Update the channel type.
		SiOPMChannelBase *old_channel = p_track->get_channel();
		SiOPMChannelBase *channel = SiOPMChannelManager::create_channel(_channel_type, old_channel, p_buffer_index);
		p_track->set_channel(channel);

		SiOPMChannelManager::delete_channel(old_channel);

	} else {
		// Just initialize the channel.
		p_track->get_channel()->initialize(p_track->get_channel(), p_buffer_index);
		p_track->reset_volume_offset();
	}

	// Initialize the tone.

	// Voice index is the same as channel number, except for PSG, APU, and analog.
	int voice_index = _initial_voice_index;
	int channel_num_restricted = p_channel_num;

	if (p_channel_num >= 0 && p_channel_num < _voice_index_table.size()) {
		voice_index = _voice_index_table[p_channel_num];
	} else {
		channel_num_restricted = 0;
	}

	p_track->set_channel_number(p_channel_num < 0 ? -1 : p_channel_num);
	// Channel requires restricted number.
	p_track->get_channel()->set_channel_number(channel_num_restricted);
	p_track->get_channel()->set_algorithm(_default_operator_count, 0);

	select_tone(p_track, voice_index);

	return (p_channel_num == -1 ? -1 : voice_index);
}

MMLSequence *SiMMLChannelSettings::select_tone(SiMMLTrack *p_track, int p_voice_index) {
	if (p_voice_index == -1) {
		return nullptr;
	}

	switch (_select_tone_type) {
		case SELECT_TONE_NORMAL: {
			int voice_index = p_voice_index;
			if (voice_index < 0 || voice_index >= _pg_type_list.size()) {
				voice_index = _initial_voice_index;
			}

			p_track->get_channel()->set_types(_pg_type_list[voice_index], _pt_type_list[voice_index]);
		} break;

		case SELECT_TONE_FM: {
			int voice_index = p_voice_index;
			if (voice_index < 0 || voice_index >= SiMMLRefTable::VOICE_MAX) {
				voice_index = 0;
			}

			SiMMLVoice *voice = SiMMLRefTable::get_instance()->get_voice(voice_index);
			if (!voice) {
				break;
			}

			if (voice->should_update_track_parameters()) {
				voice->update_track_voice(p_track);
				return nullptr;
			}

			// This module only changes channel params, but not track params.
			p_track->get_channel()->set_channel_params(voice->get_channel_params(), false, false);
			p_track->reset_volume_offset();

			MMLSequence *init_sequence = voice->get_channel_params()->get_init_sequence();
			if (init_sequence->is_empty()) {
				return nullptr;
			}

			return init_sequence;
		} break;

		default: break; // Silences enum warnings.
	}

	return nullptr;
}

//

int SiMMLChannelSettings::get_pg_type(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _pg_type_list.size(), -1);
	return _pg_type_list[p_index];
}

void SiMMLChannelSettings::set_pg_type(int p_index, int p_value) {
	ERR_FAIL_INDEX(p_index, _pg_type_list.size());
	_pg_type_list.write[p_index] = p_value;
}

int SiMMLChannelSettings::get_pt_type(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _pt_type_list.size(), -1);
	return _pt_type_list[p_index];
}

void SiMMLChannelSettings::set_pt_type(int p_index, int p_value) {
	ERR_FAIL_INDEX(p_index, _pt_type_list.size());
	_pt_type_list.write[p_index] = p_value;
}

int SiMMLChannelSettings::get_voice_index(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _voice_index_table.size(), -1);
	return _voice_index_table[p_index];
}

void SiMMLChannelSettings::set_voice_index(int p_index, int p_value) {
	ERR_FAIL_INDEX(p_index, _voice_index_table.size());
	_voice_index_table.write[p_index] = p_value;
}

//

SiMMLChannelSettings::SiMMLChannelSettings(int p_module_type, int p_pg_type, int p_length, int p_step, int p_channel_count) {
	_table = SiOPMRefTable::get_instance();
	_type = p_module_type;

	_pg_type_list.resize_zeroed(p_length);
	_pt_type_list.resize_zeroed(p_length);

	int idx = p_pg_type;
	for (int i = 0; i < p_length; i++) {
		_pg_type_list.write[i] = idx;
		_pt_type_list.write[i] = _table->get_wave_table(idx)->get_default_pitch_table_type();
		idx += p_step;
	}

	_voice_index_table.resize_zeroed(p_channel_count);
	for (int i = 0; i < p_channel_count; i++) {
		_voice_index_table.write[i] = i;
	}
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_track.h"

#include <godot_cpp/core/class_db.hpp>
#include "sion_enums.h"
#include "chip/channels/siopm_channel_base.h"
#include "chip/siopm_ref_table.h"
#include "chip/wave/siopm_wave_sampler_table.h"
#include "chip/wave/siopm_wave_table.h"
#include "sequencer/base/mml_executor.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/simml_channel_settings.h"
#include "sequencer/simml_data.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_voice.h"

SinglyLinkedList<int> *SiMMLTrack::_envelope_zero_table = nullptr;

// Properties and data.

MMLSequence *SiMMLTrack::set_channel_parameters(Vector<int> p_params) {
	MMLSequence *sequence = nullptr;

	if (p_params[0] != INT32_MIN) {
		sequence = _channel_settings->select_tone(this, p_params[0]);
		_voice_index = p_params[0];
	}

	_channel->set_parameters(p_params);
	return sequence;
}

int SiMMLTrack::get_track_id() const {
	return _internal_track_id & TRACK_ID_FILTER;
}

int SiMMLTrack::get_track_type_id() const {
	return _internal_track_id & TRACK_TYPE_FILTER;
}

Ref<BeatsPerMinute> SiMMLTrack::get_bpm_settings() const {
	if (_mml_data.is_valid() && (_internal_track_id & TRACK_TYPE_FILTER) != MML_TRACK) {
		return _mml_data->get_bpm_settings();
	}
	return nullptr;
}

int SiMMLTrack::get_priority() const {
	// Non-disposable and currently playing tracks always have top priority.
	if (!_is_disposable || is_playing_sequence()) {
		return 0;
	}

	return _priority;
}

bool SiMMLTrack::is_active() const {
	return !_is_disposable || _executor->get_pointer() || !_channel->is_idling();
}

bool SiMMLTrack::is_playing_sequence() const {
	return ((_internal_track_id & TRACK_TYPE_FILTER) != DRIVER_NOTE && _executor->get_pointer());
}

bool SiMMLTrack::is_finished() const {
	return (!_executor->get_pointer() && _channel->is_idling());
}

void SiMMLTrack::set_velocity_mode(int p_mode) {
	_velocity_mode = (p_mode >= 0 && p_mode < SiOPMRefTable::VM_MAX) ? p_mode : SiOPMRefTable::VM_LINEAR;

	int (&velocity_table)[SiOPMRefTable::TL_TABLE_SIZE] = SiOPMRefTable::get_instance()->eg_total_level_tables[_velocity_mode];
	int (&expression_table)[SiOPMRefTable::TL_TABLE_SIZE] = SiOPMRefTable::get_instance()->eg_total_level_tables[_expression_mode];
	_channel->set_volume_tables(velocity_table, expression_table);
}

void SiMMLTrack::set_expression_mode(int p_mode) {
	_expression_mode = (p_mode >= 0 && p_mode < SiOPMRefTable::VM_MAX) ? p_mode : SiOPMRefTable::VM_LINEAR;

	int (&velocity_table)[SiOPMRefTable::TL_TABLE_SIZE] = SiOPMRefTable::get_instance()->eg_total_level_tables[_velocity_mode];
	int (&expression_table)[SiOPMRefTable::TL_TABLE_SIZE] = SiOPMRefTable::get_instance()->eg_total_level_tables[_expression_mode];
	_channel->set_volume_tables(velocity_table, expression_table);
}

void SiMMLTrack::set_velocity(int p_value) {
	_velocity = CLAMP(p_value, 0, 512);
	_channel->offset_volume(_expression, _velocity);
}

void SiMMLTrack::set_expression(int p_value) {
	_expression = CLAMP(p_value, 0, 128);
	_channel->offset_volume(_expression, _velocity);
}

double SiMMLTrack::get_output_level() const {
	int volume = _channel->get_master_volume();
	if (volume == 0) {
		return _velocity * _expression * 0.0000152587890625; // 0.5 / (128 * 256);
	}

	return volume * _velocity * _expression * 2.384185791015625e-7; // 1 / (128 * 128 * 256)
}

void SiMMLTrack::set_pitch_bend(int p_value) {
	_pitch_bend = p_value;
	_channel->set_pitch(_pitch_index + _pitch_bend);
}

void SiMMLTrack::start_pitch_bending(int p_note_from, int p_tick_length) {
	_executor->pitch_bend_from(p_note_from, p_tick_length);
}

void SiMMLTrack::set_note_immediately(int p_note, int p_sample_length, bool p_slur) {
	// Play with key off when quantize_ratio == 0 or p_sample_length != 0.
	if (!p_slur && (_quantize_ratio == 0 || p_sample_length > 0)) {
		_key_on_length = (int)(p_sample_length * _quantize_ratio) - _quantize_count - _key_on_delay;
		if (_key_on_length < 1) {
			_key_on_length = 1;
		}
	} else {
		_key_on_length = 0;
	}

	_mml_key_on(p_note);
	_flag_no_key_on = p_slur;
}

// Channel properties.

void SiMMLTrack::set_channel_module_type(SiONModuleType p_type, int p_channel_num, int p_tone_num) {
	_channel_settings = _table->channel_settings_map[p_type];

	_voice_index = _channel_settings->initialize_tone(this, p_channel_num, _channel->get_buffer_index());
	if (p_tone_num >= 0) {
		_voice_index = p_tone_num;
		_channel_settings->select_tone(this, p_tone_num);
	}
}

void SiMMLTrack::reset_volume_offset() {
	_channel->offset_volume(_expression, _velocity);
}

int SiMMLTrack::get_master_volume() const {
	return _channel->get_master_volume();
}

void SiMMLTrack::set_master_volume(int p_value) {
	_channel->set_master_volume(p_value);
}

int SiMMLTrack::get_effect_send1() const {
	return _channel->get_stream_send(1);
}

int SiMMLTrack::get_effect_send2() const {
	return _channel->get_stream_send(2);
}

int SiMMLTrack::get_effect_send3() const {
	return _channel->get_stream_send(3);
}

int SiMMLTrack::get_effect_send4() const {
	return _channel->get_stream_send(4);
}

#define STREAM_SEND_SAN(m_value) (m_value < 0 ? 0 : (m_value > 128 ? 1 : m_value * 0.0078125))

void SiMMLTrack::set_effect_send1(int p_value) {
	_channel->set_stream_send(1, STREAM_SEND_SAN(p_value));
}

void SiMMLTrack::set_effect_send2(int p_value) {
	_channel->set_stream_send(2, STREAM_SEND_SAN(p_value));
}

void SiMMLTrack::set_effect_send3(int p_value) {
	_channel->set_stream_send(3, STREAM_SEND_SAN(p_value));
}

void SiMMLTrack::set_effect_send4(int p_value) {
	_channel->set_stream_send(4, STREAM_SEND_SAN(p_value));
}

#undef STREAM_SEND_SAN

bool SiMMLTrack::is_mute() const {
	return _channel->is_mute();
}

void SiMMLTrack::set_mute(bool p_value) {
	_channel->set_mute(p_value);
}

int SiMMLTrack::get_pan() const {
	return _channel->get_pan();
}

void SiMMLTrack::set_pan(int p_value) {
	_channel->set_pan(p_value);
}

// Envelopes.

SinglyLinkedList<int> *SiMMLTrack::_make_modulation_table(int p_depth, int p_end_depth, int p_delay, int p_term) {
	SinglyLinkedList<int> *list = SinglyLinkedList<int>::alloc_list(p_delay + p_term + 1);

	SinglyLinkedList<int> *element = list;
	if (p_delay != 0) {
		for (int i = 0; i < p_delay; i++) {
			element->value = p_depth;
			element = element->next;
		}
	}

	if (p_term != 0) {
		int depth = p_depth << FIXED_BITS;
		int step = ((p_end_depth << FIXED_BITS) - depth) / p_term;

		for (int i = 0; i < p_term; i++) {
			element->value = depth >> FIXED_BITS;
			depth += step;
			element = element->next;
		}
	}

	element->value = p_end_depth;

	return list;
}

void SiMMLTrack::set_portament(int p_frame) {
	_setting_sweep_step[1] = p_frame;

	if (p_frame != 0) {
		_setting_pns_or[1] = true;
		_enable_envelope_mode(1);
	} else {
		_disable_envelope_mode(1);
	}
}

void SiMMLTrack::set_envelope_fps(int p_fps) {
	_envelope_interval = SiOPMRefTable::get_instance()->sampling_rate / p_fps;
}

void SiMMLTrack::set_release_sweep(int p_sweep) {
	_setting_sweep_step[0] = p_sweep << FIXED_BITS;
	_setting_sweep_end[0] = p_sweep < 0 ? 0 : SWEEP_MAX;

	if (p_sweep != 0) {
		_setting_pns_or[0] = true;
		_enable_envelope_mode(0);
	} else {
		_disable_envelope_mode(0);
	}
}

void SiMMLTrack::set_modulation_envelope(bool p_is_pitch_mod, int p_depth, int p_end_depth, int p_delay, int p_term) {
	Vector<SinglyLinkedList<int> *> *table = (p_is_pitch_mod ? &_table_envelope_mod_pitch : &_table_envelope_mod_amp);

	// Free previous table.
	if (table->get(1)) {
		SinglyLinkedList<int>::free_list(table->get(1));
	}

	if ((p_depth >= 0 && p_depth < p_end_depth) || (p_depth < 0 && p_depth > p_end_depth)) {
		table->set(1, _make_modulation_table(p_depth, p_end_depth, p_delay, p_term));
		_enable_envelope_mode(1);
	} else {
		table->set(1, nullptr);

		if (p_is_pitch_mod) {
			_channel->set_pitch_modulation(p_depth);
		} else {
			_channel->set_amplitude_modulation(p_depth);
		}

		_disable_envelope_mode(1);
	}
}

void SiMMLTrack::set_tone_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step) {
	if (!p_table || p_step == 0) {
		_setting_envelope_voice.write[p_note_on] = nullptr;
		_disable_envelope_mode(p_note_on);
	} else {
		_setting_envelope_voice.write[p_note_on] = p_table->head;
		_setting_counter_voice[p_note_on] = p_step;
		_enable_envelope_mode(p_note_on);
	}
}

void SiMMLTrack::set_amplitude_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step, bool p_offset) {
	if (!p_table || p_step == 0) {
		_setting_envelope_exp.write[p_note_on] = nullptr;
		_disable_envelope_mode(p_note_on);
	} else {
		_setting_envelope_exp.write[p_note_on] = p_table->head;
		_setting_counter_exp[p_note_on] = p_step;
		_setting_exp_offset[p_note_on] = p_offset;
		_enable_envelope_mode(p_note_on);
	}
}

void SiMMLTrack::set_filter_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step) {
	if (!p_table || p_step == 0) {
		_setting_envelope_filter.write[p_note_on] = nullptr;
		_disable_envelope_mode(p_note_on);
	} else {
		_setting_envelope_filter.write[p_note_on] = p_table->head;
		_setting_counter_filter[p_note_on] = p_step;
		_enable_envelope_mode(p_note_on);
	}
}

void SiMMLTrack::set_pitch_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step) {
	if (!p_table || p_step == 0) {
		_setting_envelope_pitch.write[p_note_on] = _envelope_zero_table;
		_disable_envelope_mode(p_note_on);
	} else {
		_setting_envelope_pitch.write[p_note_on] = p_table->head;
		_setting_counter_pitch[p_note_on] = p_step;
		_setting_pns_or[p_note_on] = true;
		_enable_envelope_mode(p_note_on);
	}
}

void SiMMLTrack::set_note_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step) {
	if (!p_table || p_step == 0) {
		_setting_envelope_note.write[p_note_on] = _envelope_zero_table;
		_disable_envelope_mode(p_note_on);
	} else {
		_setting_envelope_note.write[p_note_on] = p_table->head;
		_setting_counter_note[p_note_on] = p_step;
		_setting_pns_or[p_note_on] = true;
		_enable_envelope_mode(p_note_on);
	}
}

// Events.

void SiMMLTrack::_default_update_register(int p_address, int p_data) {
	_channel->set_register(p_address, p_data);
}

void SiMMLTrack::set_update_register_callback(const Callable &p_func) {
	if (p_func.is_valid()) {
		_callback_update_register = p_func;
	} else {
		_callback_update_register = Callable(this, "_default_update_register");
	}
}

void SiMMLTrack::call_update_register(int p_address, int p_data) {
	if (_callback_update_register.is_valid()) {
		_callback_update_register.call(p_address, p_data);
	}
}

void SiMMLTrack::set_note_on_callback(const Callable &p_func) {
	_callback_before_note_on  = p_func;
}

void SiMMLTrack::set_note_off_callback(const Callable &p_func) {
	_callback_before_note_off = p_func;
}

void SiMMLTrack::set_event_trigger_callbacks(int p_id, EventTriggerType p_note_on_type, EventTriggerType p_note_off_type) {
	_event_trigger_id = p_id;
	_event_trigger_type_on  = p_note_on_type;
	_event_trigger_type_off = p_note_off_type;

	_callback_before_note_on = (_event_trigger_type_on != NO_EVENTS ? _event_trigger_on : Callable());
	_callback_before_note_off = (_event_trigger_type_off != NO_EVENTS ? _event_trigger_off : Callable());
}

void SiMMLTrack::trigger_note_callback(bool p_note_on) {
	if (p_note_on && _callback_before_note_on.is_valid()) {
		_callback_before_note_on.call(this);
	} else if (!p_note_on && _callback_before_note_off.is_valid()) {
		_callback_before_note_off.call(this);
	}
}

void SiMMLTrack::trigger_note_on_event(int p_id, EventTriggerType p_trigger_type) {
	if (p_trigger_type == NO_EVENTS) {
		return;
	}

	// Remember current settings.
	int current_id = _event_trigger_id;
	EventTriggerType current_type = _event_trigger_type_on;

	// Temporarily change them and run the callback.
	_event_trigger_id = p_id;
	_event_trigger_type_on = p_trigger_type;
	_event_trigger_on.call(this);

	// Set everything back.
	_event_trigger_id = current_id;
	_event_trigger_type_on = current_type;
}

void SiMMLTrack::handle_rest_event() {
	_flag_no_key_on = false;
}

void SiMMLTrack::handle_note_event(int p_note, int p_length) {
	_key_on_length = (int)(p_length * _quantize_ratio) - _quantize_count - _key_on_delay;

	if (_key_on_length < 1) {
		_key_on_length = 1;
	}

	_mml_key_on(p_note);
}

void SiMMLTrack::handle_slur() {
	_flag_no_key_on = true;
	_key_on_counter = 0;
}

void SiMMLTrack::handle_slur_weak() {
	_key_on_counter = 0;
}

void SiMMLTrack::handle_pitch_bend(int p_next_note, int p_term) {
	int start_pitch = _channel->get_pitch();

	int end_pitch_base = (p_next_note + _note_shift) << 6;
	if (end_pitch_base == 0) {
		end_pitch_base = start_pitch & 63;
	}
	int end_pitch = end_pitch_base + _pitch_shift;

	handle_slur();
	if (start_pitch == end_pitch) {
		return;
	}

	_sweep_step = ((end_pitch - start_pitch) << FIXED_BITS) * _envelope_interval / p_term;
	_sweep_end  = end_pitch << FIXED_BITS;
	_sweep_pitch = start_pitch << FIXED_BITS;
	_envelope_pitch_active = true;
	_envelope_note  = _setting_envelope_note[1];
	_envelope_pitch = _setting_envelope_pitch[1];

	_process_mode = ProcessMode::ENVELOPE;
}

void SiMMLTrack::handle_velocity(int p_value) {
	_velocity = p_value << _velocity_shift;
}

void SiMMLTrack::handle_velocity_shift(int p_value) {
	_velocity += p_value << _velocity_shift;
}

// Playback.

void SiMMLTrack::_enable_envelope_mode(int p_note_on) {
	_setting_process_mode[p_note_on] = ProcessMode::ENVELOPE;
}

void SiMMLTrack::_disable_envelope_mode(int p_note_on) {
	// Update pitch, note, sweep.
	if (_setting_sweep_step[p_note_on] == 0 &&
			_setting_envelope_pitch[p_note_on] == _envelope_zero_table &&
			_setting_envelope_note[p_note_on] == _envelope_zero_table) {
		_setting_pns_or[p_note_on] = false;
	}

	// When all envelopes are off, update the process mode.
	if (!_setting_pns_or[p_note_on] &&
			!_table_envelope_mod_amp[p_note_on] &&
			!_table_envelope_mod_pitch[p_note_on] &&
			!_setting_envelope_exp[p_note_on] &&
			!_setting_envelope_filter[p_note_on] &&
			!_setting_envelope_voice[p_note_on]) {
		_setting_process_mode[p_note_on] = ProcessMode::NORMAL;
	}
}

int SiMMLTrack::prepare_buffer(int p_buffer_length) {
	if (_mml_data.is_valid()) {
		_mml_data->register_all();
	} else {
		SiOPMRefTable::get_instance()->sampler_tables[0]->set_stencil(nullptr);

		SiOPMRefTable::get_instance()->set_stencil_custom_wave_tables(Vector<SiOPMWaveTable *>());
		SiOPMRefTable::get_instance()->set_stencil_pcm_voices(Vector<Ref<SiMMLVoice>>());

		_table->set_stencil_envelopes(Vector<SiMMLEnvelopeTable *>());
		_table->set_stencil_voices(Vector<Ref<SiMMLVoice>>());
	}

	// No delay.
	if (_track_start_delay == 0) {
		return p_buffer_length;
	}

	// Wait for the starting sound.
	if (p_buffer_length <= _track_start_delay) {
		_track_start_delay -= p_buffer_length;
		return 0;
	}

	// Start sound in this frame.
	int length = p_buffer_length - _track_start_delay;
	_channel->buffer_no_process(_track_start_delay);
	_track_start_delay = 0;
	_priority += 1;

	return length;
}

int SiMMLTrack::_buffer_envelope(int p_length, int p_step) {
	int remaining_length = p_length;
	int current_step = p_step;

	while (remaining_length >= current_step) {
		// Process buffer.
		if (current_step > 0) {
			_channel->buffer(current_step);
		}

		// Update expression.
		if (_envelope_exp && _counter_exp == 1) {
			_counter_exp--;

			int expression = CLAMP(_envelope_exp_offset + _envelope_exp->value, 0, 128);
			_channel->offset_volume(expression, _velocity);

			_envelope_exp = _envelope_exp->next;
			_counter_exp = _max_counter_exp;
		}

		// Update pitch/note.
		if (_envelope_pitch_active) {
			_channel->set_pitch(_envelope_pitch->value + (_envelope_note->value << 6) + (_sweep_pitch >> FIXED_BITS));

			if (_counter_pitch == 1) {
				_counter_pitch--;

				_envelope_pitch = _envelope_pitch->next;
				_counter_pitch = _max_counter_pitch;
			}

			if (_counter_note == 1) {
				_counter_note--;

				_envelope_note = _envelope_note->next;
				_counter_note = _max_counter_note;
			}

			_sweep_pitch += _sweep_step;
			if (_sweep_step > 0 && _sweep_pitch > _sweep_end) {
				_sweep_pitch = _sweep_end;
				_sweep_step = 0;
			} else if (_sweep_step <= 0 && _sweep_pitch < _sweep_end) {
				_sweep_pitch = _sweep_end;
				_sweep_step = 0;
			}
		}

		// Update filter.
		if (_envelope_filter && _counter_filter == 1) {
			_counter_filter--;

			_channel->offset_filter(_envelope_filter->value);

			_envelope_filter = _envelope_filter->next;
			_counter_filter = _max_counter_filter;
		}

		// Update tone.
		if (_envelope_voice && _counter_voice == 1) {
			_counter_voice--;

			_channel_settings->select_tone(this, _envelope_voice->value);

			_envelope_voice = _envelope_voice->next;
			_counter_voice = _max_counter_voice;
		}

		// Update modulations.
		if (_envelope_mod_amp) {
			_channel->set_amplitude_modulation(_envelope_mod_amp->value);
			_envelope_mod_amp =_envelope_mod_amp->next;
		}
		if (_envelope_mod_pitch) {
			_channel->set_pitch_modulation(_envelope_mod_pitch->value);
			_envelope_mod_pitch = _envelope_mod_pitch->next;
		}

		remaining_length -= current_step;
		current_step = _envelope_interval;
	}

	// Process the remainder.
	if (remaining_length > 0) {
		_channel->buffer(remaining_length);
	}

	return _envelope_interval - remaining_length;
}

void SiMMLTrack::_process_buffer(int p_length) {
	switch (_process_mode) {
		case ProcessMode::NORMAL: {
			_channel->buffer(p_length);
		} break;

		case ProcessMode::ENVELOPE: {
			_residue = _buffer_envelope(p_length, _residue);
		} break;
	}
}

void SiMMLTrack::buffer(int p_length) {
	int length = p_length;

	// Check if the track is stopping.

	bool track_stopped = false;
	int track_stop_resume = 0;

	if (_track_stop_delay > 0) {
		if (_track_stop_delay > length) {
			_track_stop_delay -= length;
		} else {
			track_stop_resume = length - _track_stop_delay;
			track_stopped = true;

			length = _track_stop_delay;
			_track_stop_delay = 0;
		}
	}

	// Buffering.
	if (_key_on_counter == 0) {
		// No status change.
		_process_buffer(length);
	} else if (_key_on_counter > length) {
		// Decrement the counter.
		_process_buffer(length);
		_key_on_counter -= length;
	} else {
		// Process -> Toggle key -> Process again.
		length -= _key_on_counter;
		_process_buffer(_key_on_counter);
		_toggle_key();
		if (length > 0) {
			_process_buffer(length);
		}
	}

	if (track_stopped) {
		if (_executor->get_pointer()) {
			_executor->stop();

			if (_stop_with_reset) {
				_key_off();
				_note = -1;
				_channel->reset();
			}
		} else if (_channel->is_note_on()) {
			_key_off();
			_note = -1;

			if (_stop_with_reset) {
				_channel->reset();
			}
		}

		if (track_stop_resume > 0) {
			_process_buffer(track_stop_resume);
		}
	}
}

void SiMMLTrack::_toggle_key() {
	if (_channel->is_note_on()) {
		_key_off();
	} else {
		_key_on();
	}
}

void SiMMLTrack::_key_on() {
	if (_callback_before_note_on.is_valid()) {
		_callback_before_note_on.call(this);
	}

	// Update pitch.
	int old_pitch = _channel->get_pitch();
	_pitch_index = ((_note + _note_shift) << 6) + _pitch_shift;
	_channel->set_pitch(_pitch_index + _pitch_bend);

	if (_flag_no_key_on) {
		// Portament.
		if (_setting_sweep_step[1] > 0) {
			_channel->set_pitch(old_pitch);
			_sweep_step = ((_pitch_index - old_pitch) << FIXED_BITS) / _setting_sweep_step[1];
			_sweep_end = _pitch_index << FIXED_BITS;
			_sweep_pitch = old_pitch << FIXED_BITS;
		} else {
			_sweep_pitch = _channel->get_pitch() << FIXED_BITS;
		}

		// Try to set envelope off.
		_disable_envelope_mode(1);
	} else {
		// Reset previous envelope.
		if (_process_mode == ProcessMode::ENVELOPE) {
			_channel->offset_volume(_expression, _velocity);
			_channel_settings->select_tone(this, _voice_index);
			_channel->offset_filter(128);
		}

		// Previous note off.
		if (_channel->is_note_on()) {
			if (_callback_before_note_off.is_valid()) {
				_callback_before_note_off.call(this);
			}
			_channel->note_off();
		}

		_update_process(1);
		_channel->note_on();
	}

	_flag_no_key_on = false;
	_key_on_counter = _key_on_length;
}

void SiMMLTrack::_key_off() {
	if (_callback_before_note_off.is_valid()) {
		_callback_before_note_off.call(this);
	}

	_channel->note_off();
	_key_on_counter = 0;
	_update_process(0);

	// Lower the priority.
	_priority += 32;
}

void SiMMLTrack::_update_process(int p_key_on) {
	_process_mode = _setting_process_mode[p_key_on];
	if (_process_mode != ProcessMode::ENVELOPE) {
		return;
	}

	// Set envelope tables.
	_envelope_exp    = _setting_envelope_exp[p_key_on];
	_envelope_voice  = _setting_envelope_voice[p_key_on];
	_envelope_note   = _setting_envelope_note[p_key_on];
	_envelope_pitch  = _setting_envelope_pitch[p_key_on];
	_envelope_filter = _setting_envelope_filter[p_key_on];

	// Set envelope counters.
	_max_counter_exp    = _setting_counter_exp[p_key_on];
	_max_counter_voice  = _setting_counter_voice[p_key_on];
	_max_counter_note   = _setting_counter_note[p_key_on];
	_max_counter_pitch  = _setting_counter_pitch[p_key_on];
	_max_counter_filter = _setting_counter_filter[p_key_on];

	_counter_exp    = 1;
	_counter_voice  = 1;
	_counter_note   = 1;
	_counter_pitch  = 1;
	_counter_filter = 1;

	// Set modulation envelopes.
	_envelope_mod_amp   = _table_envelope_mod_amp[p_key_on];
	_envelope_mod_pitch = _table_envelope_mod_pitch[p_key_on];

	// Set sweep.
	_sweep_step = (p_key_on == 1 ? 0 : _setting_sweep_step[p_key_on]);
	_sweep_end  = (p_key_on == 1 ? 0 : _setting_sweep_end[p_key_on]);
	_sweep_pitch = _channel->get_pitch() << FIXED_BITS;

	// Set pitch values.
	_envelope_exp_offset   = (_setting_exp_offset[p_key_on] ? _expression : 0);
	_envelope_pitch_active = _setting_pns_or[p_key_on];

	// Activate filter.
	if (!_channel->is_filter_active()) {
		_channel->activate_filter(_envelope_filter != nullptr);
	}

	// Reset index.
	_residue = 0;
}

void SiMMLTrack::_mml_key_on(int p_note) {
	_note = p_note;
	_track_start_delay = 0;

	if (_key_on_delay != 0) {
		_key_off();
		_key_on_counter = _key_on_delay;
	} else {
		_key_on();
	}
}

void SiMMLTrack::key_on(int p_note, int p_tick_length, int p_sample_delay) {
	_track_start_delay = p_sample_delay;
	_executor->execute_single_note(p_note, p_tick_length);
}

void SiMMLTrack::key_off(int p_sample_delay, bool p_with_reset) {
	_stop_with_reset = p_with_reset;

	if (p_sample_delay != 0) {
		_track_stop_delay = p_sample_delay;
	} else {
		_key_off();
		_note = -1;

		if (_stop_with_reset) {
			_channel->reset();
		}
	}
}

void SiMMLTrack::sequence_on(MMLSequence *p_sequence, int p_sample_length, int p_sample_delay) {
	_track_start_delay = p_sample_delay;
	_track_stop_delay = p_sample_length;

	_mml_data = Ref<SiMMLData>();
	if (p_sequence) {
		_mml_data = p_sequence->get_owner();
	}

	_executor->initialize(p_sequence);
}

void SiMMLTrack::sequence_off(int p_sample_delay, bool p_with_reset) {
	_stop_with_reset = p_with_reset;

	if (p_sample_delay != 0) {
		_track_stop_delay = p_sample_delay;
	} else {
		_executor->clear();

		if (_stop_with_reset) {
			_channel->reset();
		}
	}
}

void SiMMLTrack::limit_key_length(int p_stop_delay) {
	int length = p_stop_delay - _track_start_delay;

	if (length < _key_on_length) {
		_key_on_length = length;
		_key_on_counter = _key_on_length;
	}
}

void SiMMLTrack::change_note_length(int p_length) {
	_key_on_counter = (int)(p_length * _quantize_ratio) - _quantize_count - _key_on_delay;

	if (_key_on_counter < 1) {
		_key_on_counter = 1;
	}
}

//

void SiMMLTrack::reset(int p_buffer_index) {
	// Channel module settings.
	_channel_settings = _table->channel_settings_map[MT_PSG];
	_simulator = _table->channel_simulator_map[MT_PSG];
	_channel_number = 0;

	// Initialize channel.

	if (_mml_data.is_valid()) {
		_velocity_shift = _mml_data->get_default_velocity_shift();
		_velocity_mode = _mml_data->get_default_velocity_mode();
		_expression_mode = _mml_data->get_default_expression_mode();
	} else {
		_velocity_shift = 4;
		_velocity_mode = SiOPMRefTable::VM_LINEAR;
		_expression_mode = SiOPMRefTable::VM_LINEAR;
	}

	_velocity = 256;
	_expression = 128;
	_pitch_bend = 0;
	_note = -1;

	_channel = nullptr;
	_voice_index = _channel_settings->initialize_tone(this, INT32_MIN, p_buffer_index); // This sets the channel.

	int (&velocity_table)[SiOPMRefTable::TL_TABLE_SIZE] = SiOPMRefTable::get_instance()->eg_total_level_tables[_velocity_mode];
	int (&expression_table)[SiOPMRefTable::TL_TABLE_SIZE] = SiOPMRefTable::get_instance()->eg_total_level_tables[_expression_mode];
	_channel->set_volume_tables(velocity_table, expression_table);

	// Initialize parameters.

	_note_shift = 0;
	_pitch_shift = 0;
	_quantize_ratio = 1;
	_quantize_count = 0;

	_event_mask = NO_MASK;

	_key_on_counter = 0;
	_key_on_length = 0;
	_key_on_delay = 0;
	_flag_no_key_on = false;

	_process_mode = NORMAL;
	_track_start_delay = 0;
	_track_stop_delay = 0;
	_stop_with_reset = false;
	_priority = 0;

	_pitch_index = 0;
	_sweep_pitch = 0;

	_envelope_pitch_active = false;
	_envelope_exp_offset = 0;
	set_envelope_fps(_default_fps);

	_callback_before_note_on = Callable();
	_callback_before_note_off = Callable();
	_callback_update_register = Callable(this, "_default_update_register");

	_residue = 0;

	_envelope_exp = nullptr;
	_envelope_voice = nullptr;
	_envelope_note = _envelope_zero_table;
	_envelope_pitch = _envelope_zero_table;
	_envelope_filter = nullptr;
	_envelope_mod_amp = nullptr;
	_envelope_mod_pitch = nullptr;

	// Reset envelope tables.
	for (int i = 0; i < 2; i++) {
		_setting_process_mode[i] = NORMAL;

		_setting_envelope_exp.write[i]    = nullptr;
		_setting_envelope_voice.write[i]  = nullptr;
		_setting_envelope_note.write[i]   = _envelope_zero_table;
		_setting_envelope_pitch.write[i]  = _envelope_zero_table;
		_setting_envelope_filter.write[i] = nullptr;

		_setting_pns_or[i]     = false;
		_setting_exp_offset[i] = false;

		_setting_counter_exp[i]    = 1;
		_setting_counter_voice[i]  = 1;
		_setting_counter_note[i]   = 1;
		_setting_counter_pitch[i]  = 1;
		_setting_counter_filter[i] = 1;

		_setting_sweep_step[i] = 0;
		_setting_sweep_end[i]  = 0;

		_table_envelope_mod_amp.write[i] = nullptr;
		_table_envelope_mod_pitch.write[i] = nullptr;
	}

	// Reset executor.
	_executor->reset_pointer();
}

void SiMMLTrack::initialize(MMLSequence *p_sequence, int p_fps, int p_internal_track_id, const Callable &p_event_trigger_on, const Callable &p_event_trigger_off, bool p_disposable) {
	_default_fps = p_fps;
	_internal_track_id = p_internal_track_id;
	_is_disposable = p_disposable;

	_event_trigger_on = p_event_trigger_on;
	_event_trigger_off = p_event_trigger_off;
	_event_trigger_id = -1;
	_event_trigger_type_on = EventTriggerType::NO_EVENTS;
	_event_trigger_type_off = EventTriggerType::NO_EVENTS;

	_mml_data = Ref<SiMMLData>();
	if (p_sequence) {
		_mml_data = p_sequence->get_owner();
	}

	_executor->initialize(p_sequence);
}

void SiMMLTrack::_bind_methods() {
	// To be used as callables.
	ClassDB::bind_method(D_METHOD("_default_update_register", "address", "data"), &SiMMLTrack::_default_update_register);
}

SiMMLTrack::SiMMLTrack() {
	// Initialize static members once.
	if (!_envelope_zero_table) {
		_envelope_zero_table = SinglyLinkedList<int>::alloc_ring(1);
	}

	_table = SiMMLRefTable::get_instance();
	_executor = memnew(MMLExecutor);

	_setting_envelope_exp.resize_zeroed(2);
	_setting_envelope_voice.resize_zeroed(2);
	_setting_envelope_note.resize_zeroed(2);
	_setting_envelope_pitch.resize_zeroed(2);
	_setting_envelope_filter.resize_zeroed(2);

	_table_envelope_mod_amp.resize_zeroed(2);
	_table_envelope_mod_pitch.resize_zeroed(2);
}

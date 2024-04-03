/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_base.h"

#include <godot_cpp/core/class_db.hpp>
#include "processor/siopm_module.h"
#include "processor/siopm_stream.h"
#include "utils/godot_util.h"

#define COPY_TL_TABLE(m_target, m_source)                     \
	for (int _i = 0; _i < SiOPMTable::TL_TABLE_SIZE; _i++) {  \
		m_target[_i] = m_source[_i];                          \
	}

int SiOPMChannelBase::get_master_volume() const {
	return _volumes[0] * 128;
}

void SiOPMChannelBase::set_master_volume(int p_value) {
	int value = CLAMP(p_value, 0, 128);
	_volumes.write[0] = value * 0.0078125; // 0.0078125 = 1/128
}

// External value is in the -64 to 64 range (from all the way to the left,
// to all the way to the right). We store it as 0-128.
// [left volume]  = cos((pan + 64) / 128 * PI * 0.5) * volume;
// [right volume] = sin((pan + 64) / 128 * PI * 0.5) * volume;

int SiOPMChannelBase::get_pan() const {
	return _pan - 64;
}

void SiOPMChannelBase::set_pan(int p_value) {
	_pan = CLAMP(p_value, -64, 64) + 64;
}

//

void SiOPMChannelBase::set_filter_type(int p_type) {
	_filter_type = (p_type < 0 || p_type > 2) ? 0 : p_type;
}

// Volume control.

void SiOPMChannelBase::set_all_stream_send_levels(Vector<int> p_levels) {
	for (int i = 0; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		int value = p_levels[i];
		_volumes.write[i] = value != INT32_MIN ? value * 0.0078125 : 0;
	}

	_has_effect_send = false;
	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		if (_volumes[i] > 0) {
			_has_effect_send = true;
		}
	}
}

void SiOPMChannelBase::set_stream_buffer(int p_stream_num, SiOPMStream *p_stream) {
	_streams.write[p_stream_num] = p_stream;
}

void SiOPMChannelBase::set_stream_send(int p_stream_num, double p_volume) {
	_volumes.write[p_stream_num] = p_volume;
	if (p_stream_num == 0) {
		return;
	}

	if (p_volume > 0) {
		_has_effect_send = true;
	} else {
		_has_effect_send = false;
		for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
			if (_volumes[i] > 0) {
				_has_effect_send = true;
			}
		}
	}
}

double SiOPMChannelBase::get_stream_send(int p_stream_num) {
	return _volumes[p_stream_num];
}

// LFO control.

void SiOPMChannelBase::initialize_lfo(int p_waveform, Vector<int> p_custom_wave_table) {
	if (p_waveform == -1 && p_custom_wave_table.size() == SiOPMTable::LFO_TABLE_SIZE) {
		_lfo_wave_shape = -1;
		_lfo_wave_table = p_custom_wave_table;
	} else {
		_lfo_wave_shape = (p_waveform >= 0 && p_waveform <= SiOPMTable::LFO_WAVE_MAX) ? p_waveform : SiOPMTable::LFO_WAVE_TRIANGLE;
		_lfo_wave_table = make_vector<int>(_table->lfo_wave_tables[_lfo_wave_shape]);
	}

	_lfo_timer = 1;
	_lfo_timer_step = 0;
	_lfo_timer_step_buffer = 0;
	_lfo_phase = 0;
}

void SiOPMChannelBase::set_lfo_cycle_time(double p_ms) {
	_lfo_timer = 0;
	// 0.17294117647058824 = 44100/(1000*255)
	_lfo_timer_step = ((int)(SiOPMTable::LFO_TIMER_INITIAL/(p_ms * 0.17294117647058824))) << _table->sample_rate_pitch_shift;
	_lfo_timer_step_buffer = _lfo_timer_step;
}

// Filter control.

void SiOPMChannelBase::set_sv_filter(int p_cutoff, int p_resonance, int p_attack_rate, int p_decay_rate1, int p_decay_rate2, int p_release_rate, int p_decay_cutoff1, int p_decay_cutoff2, int p_sustain_cutoff, int p_release_cutoff) {
	_filter_eg_cutoff[EG_ATTACK]  = CLAMP(p_cutoff, 0, 128);
	_filter_eg_cutoff[EG_DECAY1]  = CLAMP(p_decay_cutoff1, 0, 128);
	_filter_eg_cutoff[EG_DECAY2]  = CLAMP(p_decay_cutoff2, 0, 128);
	_filter_eg_cutoff[EG_SUSTAIN] = CLAMP(p_sustain_cutoff, 0, 128);
	_filter_eg_cutoff[EG_RELEASE] = 0;
	_filter_eg_cutoff[EG_OFF]     = CLAMP(p_release_cutoff, 0, 128);

	_filter_eg_time[EG_ATTACK]  = _table->filter_eg_rate[p_attack_rate & 63];
	_filter_eg_time[EG_DECAY1]  = _table->filter_eg_rate[p_decay_rate1 & 63];
	_filter_eg_time[EG_DECAY2]  = _table->filter_eg_rate[p_decay_rate2 & 63];
	_filter_eg_time[EG_SUSTAIN] = INT32_MAX;
	_filter_eg_time[EG_RELEASE] = _table->filter_eg_rate[p_release_rate & 63];
	_filter_eg_time[EG_OFF]     = INT32_MAX;

	_resonance = (1 << (9 - CLAMP(p_resonance, 0, 9))) * 0.001953125; // 0.001953125 = 1/512
	_filter_on = (p_cutoff < 128 || p_resonance > 0 || p_attack_rate > 0 || p_release_rate > 0);
}

void SiOPMChannelBase::offset_filter(int p_offset) {
	_cutoff_offset = p_offset - 128;
}

// Connection control.

void SiOPMChannelBase::set_input(int p_level, int p_pipe_index) {
	if (p_level > 0) {
		_in_pipe = _chip->get_pipe(p_pipe_index & 3, _buffer_index);
		_input_mode = InputMode::INPUT_PIPE;
		_input_level = p_level + 10;
	} else {
		_in_pipe = _chip->get_zero_buffer();
		_input_mode = InputMode::INPUT_ZERO;
		_input_level = 0;
	}
}

void SiOPMChannelBase::set_ring_modulation(int p_level, int p_pipe_index) {
	_ringmod_level = p_level * 4.0 / (1 << SiOPMTable::LOG_VOLUME_BITS);
	_ring_pipe = p_level > 0 ? _chip->get_pipe(p_pipe_index & 3, _buffer_index) : nullptr;
}

void SiOPMChannelBase::set_output(OutputMode p_output_mode, int p_pipe_index) {
	bool flag_add = false;
	int pipe_index = p_pipe_index & 3;

	if (p_output_mode == OutputMode::OUTPUT_STANDARD) {
		pipe_index = 4;   // pipe[4] is used.
		flag_add = false; // Ovewrite mode.
	} else {
		flag_add = (p_output_mode == OutputMode::OUTPUT_ADD);
	}

	_output_mode = p_output_mode;
	_out_pipe = _chip->get_pipe(pipe_index, _buffer_index);
	_base_pipe = flag_add ? _out_pipe : _chip->get_zero_buffer();
}

void SiOPMChannelBase::set_volume_tables(int (&p_velocity_table)[SiOPMTable::TL_TABLE_SIZE], int (&p_expression_table)[SiOPMTable::TL_TABLE_SIZE]) {
	COPY_TL_TABLE(_velocity_table, p_velocity_table);
	COPY_TL_TABLE(_expression_table, p_expression_table);
}

// Processing.

void SiOPMChannelBase::_reset_sv_filter_state() {
	_cutoff_frequency = _filter_eg_cutoff[EG_ATTACK];
}

bool SiOPMChannelBase::_try_shift_sv_filter_state(int p_state) {
	if (_filter_eg_time[p_state] == 0) {
		return false;
	}

	_filter_eg_state = p_state;
	_filter_eg_step  = _filter_eg_time[p_state];
	_filter_eg_next  = _filter_eg_cutoff[p_state + 1];
	_filter_eg_cutoff_inc = (_cutoff_frequency < _filter_eg_next) ? 1 : -1;
	return (_cutoff_frequency != _filter_eg_next);
}

void SiOPMChannelBase::_shift_sv_filter_state(int p_state) {
	int state = p_state;

	switch (state) {
		case EG_ATTACK: {
			if (_try_shift_sv_filter_state(state)) {
				break;
			}
			state++;
		}
			[[fallthrough]];
		case EG_DECAY1: {
			if (_try_shift_sv_filter_state(state)) {
				break;
			}
			state++;
		}
			[[fallthrough]];
		case EG_DECAY2: {
			if (_try_shift_sv_filter_state(state)) {
				break;
			}
			state++;
		}
			[[fallthrough]];
		case EG_SUSTAIN: {
			// Catch all.
			_filter_eg_state = EG_SUSTAIN;
			_filter_eg_step  = INT32_MAX;
			_filter_eg_next  = _cutoff_frequency + 1;
			_filter_eg_cutoff_inc = 0;
		} break;

		case EG_RELEASE: {
			if (_try_shift_sv_filter_state(state)) {
				break;
			}
			state++;
		}
			[[fallthrough]];
		case EG_OFF: {
			// Catch all.
			_filter_eg_state = EG_OFF;
			_filter_eg_step  = INT32_MAX;
			_filter_eg_next  = _cutoff_frequency + 1;
			_filter_eg_cutoff_inc = 0;
		} break;
	}

	_filter_eg_residue = _filter_eg_step;
}

void SiOPMChannelBase::note_on() {
	_lfo_phase = 0; // Reset.
	if (_filter_on) {
		_reset_sv_filter_state();
		_shift_sv_filter_state(EG_ATTACK);
	}
	_is_note_on = true;
}

void SiOPMChannelBase::note_off() {
	if (_filter_on) {
		_shift_sv_filter_state(EG_RELEASE);
	}
	_is_note_on = false;
}

SinglyLinkedList<int> *SiOPMChannelBase::_rotate_pipe(SinglyLinkedList<int> *p_pipe, int p_length) {
	SinglyLinkedList<int> *pipe = p_pipe;
	for (int i = 0; i < p_length; i++) {
		pipe = pipe->next;
	}
	return pipe;
}

void SiOPMChannelBase::_no_process(int p_length) {
	SinglyLinkedList<int> *pipe = nullptr;

	// Rotate the output buffer.
	if (_output_mode == OutputMode::OUTPUT_STANDARD) {
		int pipe_index = (_buffer_index + p_length) & (_chip->get_buffer_length() - 1);
		_out_pipe = _chip->get_pipe(4, pipe_index);
	} else {
		_out_pipe = _rotate_pipe(_out_pipe, p_length);
		_base_pipe = _output_mode == OutputMode::OUTPUT_ADD ? pipe : _chip->get_zero_buffer();
	}

	// Rotate the input buffer when connected by @i.
	if (_input_mode == InputMode::INPUT_PIPE) {
		_in_pipe = _rotate_pipe(_in_pipe, p_length);
	}

	// Rotate the ring buffer.
	if (_ring_pipe) {
		_ring_pipe = _rotate_pipe(_ring_pipe, p_length);
	}
}

void SiOPMChannelBase::reset_channel_buffer_status() {
	_buffer_index = 0;
}

void SiOPMChannelBase::_apply_ring_modulation(SinglyLinkedList<int> *p_target, int p_length) {
	SinglyLinkedList<int> *pipe = _ring_pipe;
	SinglyLinkedList<int> *target = p_target;

	for (int i = 0; i < p_length; i++) {
		target->value *= pipe->value * _ringmod_level;
		pipe = pipe->next;
		target = target->next;
	}

	_ring_pipe = pipe;
}

void SiOPMChannelBase::_apply_sv_filter(SinglyLinkedList<int> *p_target, int p_length, double (&r_variables)[3]) {
	int cutoff = CLAMP(_cutoff_frequency + _cutoff_offset, 0, 128);
	double cutoff_value = _table->filter_cutoff_table[cutoff];
	double feedback_value = _resonance; // * _table->filter_feedback_table[out]; // This is commented out in original code.

	// Previous setting.
	int step = _filter_eg_residue;

	SinglyLinkedList<int> *target = p_target;
	int length = p_length;
	while (length >= step) {
		// Process.
		for (int i = 0; i < step; i++) {
			r_variables[2] = (double)target->value - r_variables[0] - r_variables[1] * feedback_value;
			r_variables[1] += r_variables[2] * cutoff_value;
			r_variables[0] += r_variables[1] * cutoff_value;

			target->value = (int)r_variables[_filter_type];
			target = target->next;
		}
		length -= step;

		// Change cutoff and shift state.

		_cutoff_frequency += _filter_eg_cutoff_inc;
		cutoff = CLAMP(_cutoff_frequency + _cutoff_offset, 0, 128);
		cutoff_value = _table->filter_cutoff_table[cutoff];
		feedback_value = _resonance; // * _table->filter_feedback_table[out]; // This is commented out in original code.

		if (_cutoff_frequency == _filter_eg_next) {
			_shift_sv_filter_state(_filter_eg_state + 1);
		}

		step = _filter_eg_step;
	}

	// Process the remainder.
	for (int i = 0; i < length; i++) {
		r_variables[2] = (double)target->value - r_variables[0] - r_variables[1] * feedback_value;
		r_variables[1] += r_variables[2] * cutoff_value;
		r_variables[0] += r_variables[1] * cutoff_value;

		target->value = (int)r_variables[_filter_type];
		target = target->next;
	}

	// Next setting.
	_filter_eg_residue = _filter_eg_step - length;
}

void SiOPMChannelBase::buffer(int p_length) {
	if (_is_idling) {
		buffer_no_process(p_length);
		return;
	}

	// Preserve the start of the output pipe.
	SinglyLinkedList<int> *mono_out = _out_pipe;
	// Update the output pipe for the provided length.
	if (_process_function.is_valid()) {
		_process_function.call(p_length);
	}

	if (_ring_pipe) {
		_apply_ring_modulation(mono_out, p_length);
	}
	if (_filter_on) {
		_apply_sv_filter(mono_out, p_length, _filter_variables);
	}

	if (_output_mode == OutputMode::OUTPUT_STANDARD && !_mute) {
		if (_has_effect_send) {
			for (int i = 0; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
				if (_volumes[i] > 0) {
					SiOPMStream *stream = _streams[i] ? _streams[i] : _chip->get_stream_slot(i);
					if (stream) {
						stream->write(mono_out, _buffer_index, p_length, _volumes[i], _pan);
					}
				}
			}
		} else {
			SiOPMStream *stream = _streams[0] ? _streams[0] : _chip->get_output_stream();
			stream->write(mono_out, _buffer_index, p_length, _volumes[0], _pan);
		}
	}

	_buffer_index += p_length;
}

void SiOPMChannelBase::buffer_no_process(int p_length) {
	_no_process(p_length);
	_buffer_index += p_length;
}

//

void SiOPMChannelBase::initialize(SiOPMChannelBase *p_prev, int p_buffer_index) {
	// Volume.

	if (p_prev) {
		for (int i = 0; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
			_volumes.write[i] = p_prev->_volumes[i];
			_streams.write[i] = p_prev->_streams[i];
		}

		_pan = p_prev->_pan;
		_has_effect_send = p_prev->_has_effect_send;
		_mute = p_prev->_mute;
		COPY_TL_TABLE(_velocity_table, p_prev->_velocity_table);
		COPY_TL_TABLE(_expression_table, p_prev->_expression_table);
	} else {
		_volumes.write[0] = 0.5;
		_streams.write[0] = nullptr;
		for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
			_volumes.write[i] = 0;
			_streams.write[i] = nullptr;
		}

		_pan = 64;
		_has_effect_send = false;
		_mute = false;
		COPY_TL_TABLE(_velocity_table, _table->eg_total_level_tables[SiOPMTable::VM_LINEAR]);
		COPY_TL_TABLE(_expression_table, _table->eg_total_level_tables[SiOPMTable::VM_LINEAR]);
	}

	// Buffer index.
	_is_note_on = false;
	_is_idling = true;
	_buffer_index = p_buffer_index;

	// LFO.
	initialize_lfo(SiOPMTable::LFO_WAVE_TRIANGLE);
	set_lfo_cycle_time(333);
	set_frequency_ratio(100);

	// Connection.
	set_input(0, 0);
	set_ring_modulation(0, 0);
	set_output(OutputMode::OUTPUT_STANDARD, 0);

	// LP filter.
	_filter_variables[0] = 0;
	_filter_variables[1] = 0;
	_filter_variables[2] = 0;
	_cutoff_offset = 0;
	_filter_type = FILTER_LP;
	set_sv_filter();
	_shift_sv_filter_state(EG_OFF);
}

void SiOPMChannelBase::reset() {
	_is_note_on = false;
	_is_idling = true;
}

void SiOPMChannelBase::_bind_methods() {
	// To be used as callables.
	ClassDB::bind_method(D_METHOD("_no_process", "length"), &SiOPMChannelBase::_no_process);
}

SiOPMChannelBase::SiOPMChannelBase(SiOPMModule *p_chip) {
	_table = SiOPMTable::get_instance();
	_chip = p_chip;
	_process_function = Callable(this, "_no_process");

	_streams.clear();
	_streams.resize_zeroed(SiOPMModule::STREAM_SEND_SIZE);
	_volumes.clear();
	_volumes.resize_zeroed(SiOPMModule::STREAM_SEND_SIZE);
}

#undef COPY_TL_TABLE

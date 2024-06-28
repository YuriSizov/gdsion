/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_sequencer.h"

#include <godot_cpp/core/class_db.hpp>
#include "sequencer/base/beats_per_minute.h"
#include "sequencer/base/mml_data.h"
#include "sequencer/base/mml_executor.h"
#include "sequencer/base/mml_parser.h"
#include "sequencer/base/mml_parser_settings.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/base/mml_sequence_group.h"

using namespace godot;

MMLExecutor *MMLSequencer::_temp_executor = nullptr;

void MMLSequencer::initialize() {
	_temp_executor = memnew(MMLExecutor);
}

void MMLSequencer::finalize() {
	if (_temp_executor) {
		memdelete(_temp_executor);
	}
}

// Properties.

double MMLSequencer::get_default_bpm() const {
	return _parser_settings->default_bpm;
}

void MMLSequencer::set_default_bpm(double p_value) {
	_parser_settings->default_bpm = p_value;
}

double MMLSequencer::get_bpm() const {
	return _adjustible_bpm->get_bpm();
}

void MMLSequencer::set_bpm(double p_value) {
	double old_value = _adjustible_bpm->get_bpm();
	if (_adjustible_bpm->update(p_value, _sample_rate)) {
		_on_tempo_changed(old_value / p_value);
	}
}

// Events.

void MMLSequencer::_set_mml_event_listener(int p_event_id, const Callable &p_handler, bool p_global) {
	_event_handlers.write[p_event_id] = p_handler;
	_event_global_flags.write[p_event_id] = p_global;
}

int MMLSequencer::_create_mml_event_listener(String p_letter, const Callable &p_handler, bool p_global) {
	int event_id = _next_user_defined_event_id;
	_next_user_defined_event_id++;

	_user_defined_event_map[p_letter] = event_id;
	_event_command_letter_map[event_id] = p_letter;
	_set_mml_event_listener(event_id, p_handler, p_global);

	return event_id;
}

int MMLSequencer::get_event_id(String p_mml_command) {
	int event_id = MMLEvent::get_id_from_mml(p_mml_command);
	if (event_id != 0) {
		return event_id;
	}

	if (_user_defined_event_map.has(p_mml_command)) {
		return _user_defined_event_map[p_mml_command];
	}
	return 0;
}

String MMLSequencer::get_event_letter(int p_event_id) {
	ERR_FAIL_COND_V(!_event_command_letter_map.has(p_event_id), "");

	return _event_command_letter_map[p_event_id];
}

// Event handlers.

MMLEvent *MMLSequencer::_no_process(MMLEvent *p_event) {
	return p_event->get_next();
}

MMLEvent *MMLSequencer::_dummy_on_process(MMLEvent *p_event) {
	// Set processing length.
	if (_current_executor->get_residue_sample_count() == 0) {
		int sample_count_fixed = p_event->get_length() * _bpm->get_sample_per_tick() + _current_executor->get_decimal_fraction_sample_count();
		_current_executor->set_residue_sample_count(sample_count_fixed >> FIXED_BITS);
		_current_executor->set_decimal_fraction_sample_count(sample_count_fixed & FIXED_FILTER);
	}

	// Process.
	if (_current_executor->get_residue_sample_count() <= _process_buffer_sample_count) {
		_process_buffer_sample_count -= _current_executor->get_residue_sample_count();
		_current_executor->set_residue_sample_count(0);
		// Go to the next command.
		return p_event->get_jump()->get_next();
	} else {
		_current_executor->adjust_residue_sample_count(-_process_buffer_sample_count);
		_process_buffer_sample_count = 0;
		// Stay on this command.
		return p_event;
	}
}

MMLEvent *MMLSequencer::_dummy_on_process_event(MMLEvent *p_event) {
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *MMLSequencer::_default_on_no_operation(MMLEvent *p_event) {
	_on_process(_process_buffer_sample_count, p_event);
	_current_executor->adjust_residue_sample_count(-_process_buffer_sample_count);
	return p_event;
}

MMLEvent *MMLSequencer::_default_on_global_wait(MMLEvent *p_event) {
	// Set processing length.
	if (_current_executor->get_residue_sample_count() == 0) {
		int sample_count_fixed = p_event->get_length() * _bpm->get_sample_per_tick() + _current_executor->get_decimal_fraction_sample_count();
		_current_executor->set_residue_sample_count(sample_count_fixed >> FIXED_BITS);
		_current_executor->set_decimal_fraction_sample_count(sample_count_fixed & FIXED_FILTER);
	}

	// Wait.
	if (_current_executor->get_residue_sample_count() <= _global_buffer_sample_count) {
		_global_execute_sample_count = _current_executor->get_residue_sample_count();
		_global_buffer_sample_count -= _global_execute_sample_count;
		_current_executor->set_residue_sample_count(0);
		// Go to the next command.
		return p_event->get_next();
	} else {
		_global_execute_sample_count =  _global_buffer_sample_count;
		_current_executor->adjust_residue_sample_count(-_global_execute_sample_count);
		_global_buffer_sample_count  = 0;
		// Stay on this command.
		return p_event;
	}
}

MMLEvent *MMLSequencer::_default_on_process(MMLEvent *p_event) {
	// Set processing length.
	if (_current_executor->get_residue_sample_count() == 0) {
		int sample_count_fixed = p_event->get_length() * _bpm->get_sample_per_tick() + _current_executor->get_decimal_fraction_sample_count();
		_current_executor->set_residue_sample_count(sample_count_fixed >> FIXED_BITS);
		_current_executor->set_decimal_fraction_sample_count(sample_count_fixed & FIXED_FILTER);
	}

	// Process.
	if (_current_executor->get_residue_sample_count() <= _process_buffer_sample_count) {
		_on_process(_current_executor->get_residue_sample_count(), p_event->get_jump());
		_process_buffer_sample_count -= _current_executor->get_residue_sample_count();
		_current_executor->set_residue_sample_count(0);
		// Go to the next command.
		return p_event->get_jump()->get_next();
	} else {
		_on_process(_process_buffer_sample_count, p_event->get_jump());
		_current_executor->adjust_residue_sample_count(-_process_buffer_sample_count);
		_process_buffer_sample_count = 0;
		// Stay on this command.
		return p_event;
	}
}

MMLEvent *MMLSequencer::_default_on_repeat_all(MMLEvent *p_event) {
	return _current_executor->on_repeat_all(p_event);
}

MMLEvent *MMLSequencer::_default_on_repeat_begin(MMLEvent *p_event) {
	return _current_executor->on_repeat_begin(p_event);
}

MMLEvent *MMLSequencer::_default_on_repeat_break(MMLEvent *p_event) {
	return _current_executor->on_repeat_break(p_event);
}

MMLEvent *MMLSequencer::_default_on_repeat_end(MMLEvent *p_event) {
	return _current_executor->on_repeat_end(p_event);
}

MMLEvent *MMLSequencer::_default_on_sequence_tail(MMLEvent *p_event) {
	return _current_executor->on_sequence_tail(p_event);
}

MMLEvent *MMLSequencer::_default_on_tempo(MMLEvent *p_event) {
	double bpm = mml_data.is_valid() ? mml_data->get_bpm_from_tcommand(p_event->get_data()) : p_event->get_data();
	set_bpm(bpm);
	return p_event->get_next();
}

MMLEvent *MMLSequencer::_default_on_timer(MMLEvent *p_event) {
	_on_timer_interruption();
	return p_event->get_next();
}

MMLEvent *MMLSequencer::_default_on_internal_wait(MMLEvent *p_event) {
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *MMLSequencer::_default_on_internal_call(MMLEvent *p_event) {
	List<Callable> callbacks = _current_executor->get_sequence()->get_callbacks_for_internal_call();
	int callback_idx = p_event->get_data();

	MMLEvent *next = nullptr;
	if (callback_idx >= 0 && callback_idx < callbacks.size()) {
		Callable cb = callbacks[callback_idx];
		if (cb.is_valid()) {
			next = Object::cast_to<MMLEvent>(cb.call(p_event->get_length()));
		}
	}

	if (next) {
		return next;
	}
	return p_event->get_next();
}

// Compilation and processing.

struct EventComparator {
	_FORCE_INLINE_ bool operator()(const MMLEvent *e1, const MMLEvent *e2) const {
		return e1->get_length() < e2->get_length();
	}
};

void MMLSequencer::_extract_global_sequence() {
	MMLSequenceGroup *seq_group = mml_data->get_sequence_group();

	List<MMLEvent *> list;

	MMLSequence *sequence = seq_group->get_head_sequence();
	while (sequence) {
		int count = sequence->get_head_event()->get_data();
		if (count == 0) {
			sequence = sequence->get_next_sequence();
			continue;
		}

		_temp_executor->initialize(sequence);
		MMLEvent *prev = sequence->get_head_event();
		MMLEvent *event = prev->get_next();

		int position = 0;
		bool has_no_event = true;

		// Calculate position and pick global events.
		while (event && (count > 0 || has_no_event)) {
			// Global or table event.
			if (_event_global_flags[event->get_id()]) {
				if (event->get_id() == MMLEvent::TABLE_EVENT) {
					// Well, table event then.
					parse_table_event(prev);
				} else {
					// And here it's a global one.
					if (sequence->get_head_event()->get_jump() == event) {
						sequence->get_head_event()->set_jump(prev);
					}

					prev->set_next(event->get_next());
					event->set_next(nullptr);
					event->set_length(position);
					list.push_back(event);
				}

				event = prev->get_next();
				count--;
				continue;
			}

			// Note or rest.
			if (event->get_length() != 0) {
				position += event->get_length();
				if (event->get_id() != MMLEvent::REST) {
					has_no_event = false;
				}
				prev = event;
				event = event->get_next();
				continue;
			}

			// Everything else.
			prev = event;

			switch (event->get_id()) {
				case MMLEvent::REPEAT_BEGIN: {
					event = _temp_executor->on_repeat_begin(event);
				} break;

				case MMLEvent::REPEAT_BREAK: {
					event = _temp_executor->on_repeat_break(event);
					if (prev->get_next() != event) {
						prev = prev->get_jump()->get_jump();
					}
				} break;

				case MMLEvent::REPEAT_END: {
					event = _temp_executor->on_repeat_end(event);
					if (prev->get_next() != event) {
						prev = prev->get_jump();
					}
				} break;

				case MMLEvent::REPEAT_ALL: {
					event = _temp_executor->on_repeat_all(event);
				} break;

				case MMLEvent::SEQUENCE_TAIL: {
					event = nullptr;
				} break;

				default: {
					event = event->get_next();
					has_no_event = true;
				} break;
			}
		}

		// If there is no event (except for rest) in the sequence, skip this sequence.
		if (has_no_event) {
			sequence = sequence->remove_from_chain();
		}
		sequence = sequence->get_next_sequence();
	}

	list.sort_custom<EventComparator>();

	// Create global sequence.
	MMLSequence *glob_sequence = mml_data->get_global_sequence();
	int position = 0;
	int initial_bpm = 0;

	for (MMLEvent *event : list) {
		if (event->get_length() == 0 && event->get_id() == MMLEvent::TEMPO) {
			// First tempo command is default BPM.
			initial_bpm = mml_data->get_bpm_from_tcommand(event->get_data());
		} else {
			int count = event->get_length() - position;
			position = event->get_length();
			event->set_length(0);

			if (count > 0) {
				glob_sequence->append_new_event(MMLEvent::GLOBAL_WAIT, 0, count);
			}
			glob_sequence->push_back(event);
		}
	}

	if (initial_bpm > 0) {
		Ref<BeatsPerMinute> bpm_obj = memnew(BeatsPerMinute(initial_bpm, 44100, _parser_settings->resolution));
		mml_data->set_bpm_settings(bpm_obj);
	}
}

bool MMLSequencer::prepare_compile(const Ref<MMLData> &p_data, String p_mml) {
	mml_data = p_data;
	if (mml_data.is_null()) {
		return false;
	}

	mml_data->clear();

	MMLParser::get_instance()->set_user_defined_event_map(_user_defined_event_map);
	MMLParser::get_instance()->set_global_event_flags(_event_global_flags);

	String mml_string = _on_before_compile(p_mml);
	if (mml_string.is_empty()) {
		mml_data = Ref<MMLData>();
		return false;
	}

	MMLParser::get_instance()->prepare_parse(_parser_settings, mml_string);
	return true;
}

double MMLSequencer::compile(int p_interval) {
	if (mml_data.is_null()) {
		return 1;
	}

	MMLEvent *event = MMLParser::get_instance()->parse(p_interval);
	// If there is no event, then the parsing process is still going.
	// TODO: Use a signal instead?
	if (!event) {
		return MMLParser::get_instance()->get_parse_progress();
	}

	// Create the main sequence group.
	mml_data->get_sequence_group()->alloc(event);
	_extract_global_sequence();
	_on_after_compile(mml_data->get_sequence_group());

	return 1;
}

void MMLSequencer::prepare_process(const Ref<MMLData> &p_data, int p_sample_rate, int p_buffer_length) {
	ERR_FAIL_COND_MSG((p_sample_rate != 22050 && p_sample_rate != 44100), "MMLSequencer: Sampling rate can only be 22050 or 44100.");

	mml_data = p_data;
	_sample_rate = p_sample_rate;
	_buffer_length = p_buffer_length;

	if (mml_data.is_valid() && mml_data->get_bpm() > 0) {
		_adjustible_bpm->update(mml_data->get_bpm(), p_sample_rate);
		_global_executor->initialize(mml_data->get_global_sequence());
	} else {
		_adjustible_bpm->update(_parser_settings->default_bpm, p_sample_rate);
		_global_executor->initialize(nullptr);
	}

	_bpm = _adjustible_bpm;
	_global_buffer_index = 0;
	_global_beat_16th = 0;
}

void MMLSequencer::set_global_sequence(MMLSequence *p_sequence) {
	_global_executor->initialize(p_sequence);
}

void MMLSequencer::start_global_sequence() {
	_global_buffer_sample_count = _buffer_length;
	_global_execute_sample_count = 0;
	_global_buffer_index = 0;
}

int MMLSequencer::execute_global_sequence() {
	_current_executor = _global_executor;

	MMLEvent *event = _current_executor->get_pointer();
	_global_execute_sample_count = 0;

	do {
		if (event == nullptr) {
			_global_execute_sample_count = _global_buffer_sample_count;
			_global_buffer_sample_count = 0;
		} else {
			// Update global execute sample count in some event handlers.
			Callable cb = _event_handlers[event->get_id()];
			if (cb.is_valid()) {
				event = Object::cast_to<MMLEvent>(cb.call(event));
				_current_executor->set_pointer(event);
			} else {
				// This shouldn't happen unless something is very wrong with our callables.
				event = nullptr;
			}
		}
	} while (_global_execute_sample_count == 0);

	return _global_execute_sample_count;
}

bool MMLSequencer::check_global_sequence_end() {
	double prev_beat = _global_beat_16th;
	int floor_prev_beat = (int)prev_beat;

	_global_buffer_index += _global_execute_sample_count;
	_global_beat_16th += _global_execute_sample_count * _bpm->get_beat_16th_per_sample();

	if (prev_beat == 0) {
		_on_beat(0, 0);
	} else {
		int floor_curr_beat = (int)_global_beat_16th;
		while (floor_prev_beat < floor_curr_beat) {
			floor_prev_beat++;

			if ((floor_prev_beat & _on_beat_callback_filter) == 0) {
				_on_beat((floor_prev_beat - prev_beat) * _bpm->get_sample_per_beat_16th(), floor_prev_beat);
			}
		}
	}

	if (_global_buffer_sample_count == 0) {
		_global_buffer_index = 0;
		return true;
	}
	return false;
}

bool MMLSequencer::process_executor(MMLExecutor *p_executor, int p_buffer_sample_count) {
	_current_executor = p_executor;

	MMLEvent *event = _current_executor->get_pointer();
	_process_buffer_sample_count = p_buffer_sample_count;
	while (_process_buffer_sample_count > 0) {
		if (event == nullptr) {
			Callable cb = _event_handlers[MMLEvent::NO_OP];
			if (cb.is_valid()) {
				cb.call(_current_executor->get_nop_event());
			}
			return true;
		}

		// Update process buffer sample count in some event handlers.
		Callable cb = _event_handlers[event->get_id()];
		if (cb.is_valid()) {
			event = Object::cast_to<MMLEvent>(cb.call(event));
			_current_executor->set_pointer(event);
		} else {
			// This shouldn't happen unless something is very wrong with our callables.
			event = nullptr;
		}
	}

	return false;
}

int MMLSequencer::calculate_sample_count(int p_length) {
	return (int)(p_length * _bpm->get_sample_per_tick()) >> FIXED_BITS;
}

double MMLSequencer::calculate_sample_length(double p_beat_16th) {
	return p_beat_16th * _bpm->get_sample_per_beat_16th();
}

double MMLSequencer::calculate_sample_delay(int p_sample_offset, double p_beat_16th_offset, double p_quant) {
	if (p_quant == 0) {
		return p_sample_offset + p_beat_16th_offset * _bpm->get_sample_per_beat_16th();
	}

	int beats = (int)(p_sample_offset * _bpm->get_beat_16th_per_sample() + _global_beat_16th + p_beat_16th_offset + 0.9999847412109375); // = 65535/65536
	if (p_quant != 1) {
		beats = (int)((beats + p_quant - 1) / p_quant) * p_quant;
	}

	return (beats - _global_beat_16th) * _bpm->get_sample_per_beat_16th();
}

int MMLSequencer::get_current_tick_count() {
	return _current_executor->get_current_tick_count() - _current_executor->get_residue_sample_count() * _bpm->get_tick_per_sample();
}

void MMLSequencer::parse_table_event(MMLEvent *p_prev) {
	MMLEvent *table_event = p_prev->get_next();
	_on_table_parse(p_prev, MMLParser::get_instance()->get_system_event_string(table_event));

	p_prev->set_next(table_event->get_next());
	MMLParser::get_instance()->free_event(table_event);
}

//

void MMLSequencer::_bind_methods() {
	// To be used as callables.
	ClassDB::bind_method(D_METHOD("_no_process", "event"),               &MMLSequencer::_no_process);
	ClassDB::bind_method(D_METHOD("_dummy_on_process", "event"),         &MMLSequencer::_dummy_on_process);
	ClassDB::bind_method(D_METHOD("_dummy_on_process_event", "event"),   &MMLSequencer::_dummy_on_process_event);

	ClassDB::bind_method(D_METHOD("_default_on_no_operation", "event"),  &MMLSequencer::_default_on_no_operation);
	ClassDB::bind_method(D_METHOD("_default_on_process", "event"),       &MMLSequencer::_default_on_process);
	ClassDB::bind_method(D_METHOD("_default_on_repeat_all", "event"),    &MMLSequencer::_default_on_repeat_all);
	ClassDB::bind_method(D_METHOD("_default_on_repeat_begin", "event"),  &MMLSequencer::_default_on_repeat_begin);
	ClassDB::bind_method(D_METHOD("_default_on_repeat_break", "event"),  &MMLSequencer::_default_on_repeat_break);
	ClassDB::bind_method(D_METHOD("_default_on_repeat_end", "event"),    &MMLSequencer::_default_on_repeat_end);
	ClassDB::bind_method(D_METHOD("_default_on_sequence_tail", "event"), &MMLSequencer::_default_on_sequence_tail);
	ClassDB::bind_method(D_METHOD("_default_on_global_wait", "event"),   &MMLSequencer::_default_on_global_wait);
	ClassDB::bind_method(D_METHOD("_default_on_tempo", "event"),         &MMLSequencer::_default_on_tempo);
	ClassDB::bind_method(D_METHOD("_default_on_timer", "event"),         &MMLSequencer::_default_on_timer);
	ClassDB::bind_method(D_METHOD("_default_on_internal_wait", "event"), &MMLSequencer::_default_on_internal_wait);
	ClassDB::bind_method(D_METHOD("_default_on_internal_call", "event"), &MMLSequencer::_default_on_internal_call);
}

MMLSequencer::MMLSequencer() {
	_parser_settings = memnew(MMLParserSettings);

	_event_handlers.resize_zeroed(MMLEvent::COMMAND_MAX);
	_event_global_flags.resize_zeroed(MMLEvent::COMMAND_MAX);

	for (int i = 0; i < MMLEvent::COMMAND_MAX; i++) {
		_event_handlers.write[i] = Callable(this, "_no_process");
	}

	_set_mml_event_listener(MMLEvent::NO_OP,         Callable(this, "_default_on_no_operation"),  false);
	_set_mml_event_listener(MMLEvent::PROCESS,       Callable(this, "_default_on_process"),       false);
	_set_mml_event_listener(MMLEvent::REPEAT_ALL,    Callable(this, "_default_on_repeat_all"),    false);
	_set_mml_event_listener(MMLEvent::REPEAT_BEGIN,  Callable(this, "_default_on_repeat_begin"),  false);
	_set_mml_event_listener(MMLEvent::REPEAT_BREAK,  Callable(this, "_default_on_repeat_break"),  false);
	_set_mml_event_listener(MMLEvent::REPEAT_END,    Callable(this, "_default_on_repeat_end"),    false);
	_set_mml_event_listener(MMLEvent::SEQUENCE_TAIL, Callable(this, "_default_on_sequence_tail"), false);
	_set_mml_event_listener(MMLEvent::GLOBAL_WAIT,   Callable(this, "_default_on_global_wait"),   true);
	_set_mml_event_listener(MMLEvent::TEMPO,         Callable(this, "_default_on_tempo"),         true);
	_set_mml_event_listener(MMLEvent::TIMER,         Callable(this, "_default_on_timer"),         true);
	_set_mml_event_listener(MMLEvent::INTERNAL_WAIT, Callable(this, "_default_on_internal_wait"), false);
	_set_mml_event_listener(MMLEvent::INTERNAL_CALL, Callable(this, "_default_on_internal_call"), false);
	_set_mml_event_listener(MMLEvent::TABLE_EVENT,   Callable(this, "_no_process"),               true);

	Ref<BeatsPerMinute> base_bpm = memnew(BeatsPerMinute(120, 44100));
	_adjustible_bpm = base_bpm;
	_bpm = base_bpm;

	_global_executor = memnew(MMLExecutor);
	MMLParser::get_instance()->get_command_letters(&_event_command_letter_map);
}

MMLSequencer::~MMLSequencer() {
	memdelete(_parser_settings);
	memdelete(_global_executor);
}

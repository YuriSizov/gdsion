/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_parser.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_parser_settings.h"
#include "sequencer/base/mml_sequence.h"
#include "utils/godot_util.h"

using namespace godot;

MMLParser *MMLParser::_instance = nullptr;

void MMLParser::initialize() {
	if (_instance) {
		return;
	}

	// Sets the instance internally.
	memnew(MMLParser);
}

// Methods.

#define OP_ERR_FAIL_RANGE(m_value, m_min, m_max, m_cmd)                                                                                                            \
	ERR_FAIL_COND_MSG(m_value < m_min || m_value > m_max, vformat("MMLParser: Command '%s' has argument outside of valid range (%d : %d).", m_cmd, m_min, m_max));

#define OP_ERR_FAIL_RANGE_V(m_value, m_min, m_max, m_cmd, m_return)                                                                                                            \
	ERR_FAIL_COND_V_MSG(m_value < m_min || m_value > m_max, m_return, vformat("MMLParser: Command '%s' has argument outside of valid range (%d : %d).", m_cmd, m_min, m_max));

// Settings.

void MMLParser::_create_mml_regex(bool p_reset) {
	if (p_reset) {
		_mml_regex_last_index = 0;
	}

	// It must be cleared first, otherwise we won't recreate it.
	if (_mml_regex.is_valid()) {
		return;
	}

	// We generate a regular expression string based on some setting information.
	// Specifically, we account for user define event letters.

	PackedStringArray user_defs;
	for (const KeyValue<String, int> &kv : _user_defined_event_map) {
		user_defs.push_back(kv.key);
	}

	// Here's the part where user definitions are converted to a regex-compatible string.
	// If there are no definitions, we set the string to "a" as a hacky solution.
	String user_defs_str = "a";
	if (user_defs.size() > 0) {
		user_defs.sort();
		user_defs.reverse(); // We want descending order.

		user_defs_str = String("|").join(user_defs);
	}

	// Godot's RegEx implementation doesn't support passing global flags, but PCRE2 allows local flags, which we can abuse.
	// (?s) enables single line mode (dot matches newline) for the entire expression.
	String reg_string = "(?s)";
	reg_string += "(\\s+)";                                            // whitespace [1]
	reg_string += "|(#[^;]*)";                                         // system [2]
	reg_string += "|(";                                                // --all-- [3]
		reg_string += "([a-g])([\\-+#]?)";                                 // note [4][5]
		reg_string += "|(" + user_defs_str + ")";                          // module events [6]
		reg_string += "|(@[qvio]?|&&|!@ns|[rlqovt^<>()\\[\\]/|$%&*,;])";   // default events [7]
		reg_string += "|(\\{.*?\\}[0-9]*\\*?[\\-0-9.]*\\+?[\\-0-9.]*)";    // table event [8]
	reg_string += ")\\s*(-?[0-9]*)";                                   // parameter [9]
	reg_string += "\\s*(\\.*)";                                        // periods [10]

	_mml_regex = RegEx::create_from_string(reg_string);
}

void MMLParser::_clear_mml_regex() {
	_mml_regex = Ref<RegEx>();
	_mml_regex_last_index = 0;
}

void MMLParser::set_user_defined_event_map(HashMap<String, int> p_event_map) {
	// Original code checks if the map is the same before assigning. This can be expensive and a problem to check for us.

	_user_defined_event_map = p_event_map;
	_clear_mml_regex();
}

void MMLParser::set_global_event_flags(Vector<bool> p_event_flags) {
	_event_global_flags = p_event_flags;
}

void MMLParser::get_command_letters(HashMap<int, String> *r_letter_map) {
	if (!r_letter_map) {
		return;
	}

	(*r_letter_map)[MMLEvent::NOTE]         = "c";
	(*r_letter_map)[MMLEvent::REST]         = "r";
	(*r_letter_map)[MMLEvent::QUANT_RATIO]  = "q";
	(*r_letter_map)[MMLEvent::QUANT_COUNT]  = "@q";
	(*r_letter_map)[MMLEvent::VOLUME]       = "v";
	(*r_letter_map)[MMLEvent::FINE_VOLUME]  = "@v";
	(*r_letter_map)[MMLEvent::MOD_TYPE]     = "%";
	(*r_letter_map)[MMLEvent::MOD_PARAM]    = "@";
	(*r_letter_map)[MMLEvent::INPUT_PIPE]   = "@i";
	(*r_letter_map)[MMLEvent::OUTPUT_PIPE]  = "@o";
	(*r_letter_map)[MMLEvent::VOLUME_SHIFT] = "(";
	(*r_letter_map)[MMLEvent::SLUR]         = "&";
	(*r_letter_map)[MMLEvent::SLUR_WEAK]    = "&&";
	(*r_letter_map)[MMLEvent::PITCHBEND]    = "*";
	(*r_letter_map)[MMLEvent::PARAMETER]    = ",";
	(*r_letter_map)[MMLEvent::REPEAT_ALL]   = "$";
	(*r_letter_map)[MMLEvent::REPEAT_BEGIN] = "[";
	(*r_letter_map)[MMLEvent::REPEAT_END]   = "]";
	(*r_letter_map)[MMLEvent::REPEAT_BREAK] = "|";
	(*r_letter_map)[MMLEvent::TEMPO]        = "t";
}

int MMLParser::_register_system_event_string(String p_event) {
	if (_system_event_strings.size() <= _system_event_index) {
		_system_event_strings.resize_zeroed(_system_event_strings.size() * 2);
	}

	_system_event_strings.write[_system_event_index] = p_event;
	_system_event_index++;
	return _system_event_index - 1;
}

String MMLParser::get_system_event_string(MMLEvent *p_event) {
	ERR_FAIL_INDEX_V(p_event->data, _system_event_strings.size(), "");

	return _system_event_strings[p_event->data];
}

int MMLParser::_register_sequence_mml_strings(String p_mml) {
	if (_sequence_mml_strings.size() <= _sequence_mml_index) {
		_sequence_mml_strings.resize_zeroed(_sequence_mml_strings.size() * 2);
	}

	_sequence_mml_strings.write[_sequence_mml_index] = p_mml;
	_sequence_mml_index++;
	return _sequence_mml_index - 1;
}

String MMLParser::get_sequence_mml(MMLEvent *p_event) {
	if (p_event->length == -1) {
		return "";
	}
	ERR_FAIL_INDEX_V(p_event->length, _sequence_mml_strings.size(), "");

	return _sequence_mml_strings[p_event->length];
}

// Key.

void MMLParser::set_key_signature(String p_sign) {
	if (p_sign.is_empty()) {
		_key_signature = _key_signature_table[0];
		return;
	}

	// Please make sure to keep a sensible chord order below.

	if (p_sign == "C" || p_sign == "Am") {
		_key_signature = _key_signature_table[0];
		return;
	}
	if (p_sign == "G" || p_sign == "Em") {
		_key_signature = _key_signature_table[1];
		return;
	}
	if (p_sign == "D" || p_sign == "Bm") {
		_key_signature = _key_signature_table[2];
		return;
	}
	if (p_sign == "A" || p_sign == "F+m" || p_sign == "F#m") {
		_key_signature = _key_signature_table[3];
		return;
	}
	if (p_sign == "E" || p_sign == "C+m" || p_sign == "C#m") {
		_key_signature = _key_signature_table[4];
		return;
	}
	if (p_sign == "B" || p_sign == "G+m" || p_sign == "G#m") {
		_key_signature = _key_signature_table[5];
		return;
	}
	if (p_sign == "F+" || p_sign == "F#" || p_sign == "D+m" || p_sign == "D#m") {
		_key_signature = _key_signature_table[6];
		return;
	}
	if (p_sign == "C+" || p_sign == "C#" || p_sign == "A+m" || p_sign == "A#m") {
		_key_signature = _key_signature_table[7];
		return;
	}
	if (p_sign == "F" || p_sign == "Dm") {
		_key_signature = _key_signature_table[8];
		return;
	}
	if (p_sign == "B-" || p_sign == "Bb" || p_sign == "Gm") {
		_key_signature = _key_signature_table[9];
		return;
	}
	if (p_sign == "E-" || p_sign == "Eb" || p_sign == "Cm") {
		_key_signature = _key_signature_table[10];
		return;
	}
	if (p_sign == "A-" || p_sign == "Ab" || p_sign == "Fm") {
		_key_signature = _key_signature_table[11];
		return;
	}
	if (p_sign == "D-" || p_sign == "Db" || p_sign == "B-m" || p_sign == "Bbm") {
		_key_signature = _key_signature_table[12];
		return;
	}
	if (p_sign == "G-" || p_sign == "Gb" || p_sign == "E-m" || p_sign == "Ebm") {
		_key_signature = _key_signature_table[13];
		return;
	}
	if (p_sign == "C-" || p_sign == "Cb" || p_sign == "A-m" || p_sign == "Abm") {
		_key_signature = _key_signature_table[14];
		return;
	}

	// Generate a custom signature if there is no match.

	static const Vector<char32_t> note_letters = { 'c', 'd', 'e', 'f', 'g', 'a', 'b' };

	for (int i = 0; i < 7; i++) {
		_key_signature_custom.write[i] = 0;
	}

	PackedStringArray arr = split_string_by_regex(p_sign, "[\\s,]");
	// Note that the original code is broken here (it tries to get the first and the second character
	// on the Array object). I assume that the intention is to check each split substring and parse it
	// as a note in the key table. If there are duplicate notes, then the latter overrides the former.
	// If notes are missing then they are set to 0 in the table.
	for (int i = 0; i < arr.size(); i++) {
		String note_sign = arr[i].to_lower();
		int note_idx = note_letters.find(note_sign[0]);
		ERR_CONTINUE_MSG(note_idx == -1, vformat("MMLParser: Cannot recognize '%s' as a key signature.", p_sign));

		if (note_sign.length() > 1) {
			char32_t note_shift = note_sign[1];
			if (note_shift == '+' || note_shift == '#') {
				_key_signature_custom.write[note_idx] = 1;
			} else if (note_shift == '-' || note_shift == 'b') {
				_key_signature_custom.write[note_idx] = -1;
			} else {
				_key_signature_custom.write[note_idx] = 0;
			}
		} else {
			_key_signature_custom.write[note_idx] = 0;
		}
	}

	_key_signature = _key_signature_custom;
}

// Parsing and events.

MMLEvent *MMLParser::_push_mml_event(int p_event_id, int p_data, int p_length) {
	_last_event->next = alloc_event(p_event_id, p_data, p_length);
	_last_event = _last_event->next;

	return _last_event;
}

MMLEvent *MMLParser::_add_mml_event(int p_event_id, int p_data, int p_length, bool p_note_option) {
	if (p_note_option) {
		// Note option events are inserted after NOTE.
		ERR_FAIL_COND_V_MSG(_last_event->id != MMLEvent::NOTE, nullptr, "MMLParser: Commands '*' and '&' can only come after a note.");
		int length = _last_event->length;
		_last_event->length = 0;
		_push_mml_event(p_event_id, p_data, length);
	} else {
		// Create channel data chain.
		if (p_event_id == MMLEvent::SEQUENCE_HEAD) {
			_last_sequence_head->jump = _last_event;
			_last_sequence_head = _push_mml_event(p_event_id, p_data, p_length);
			_reset_state_track();

		// Concatenate REST events.
		} else if (p_event_id == MMLEvent::REST && _last_event->id == MMLEvent::REST) {
			_last_event->length += p_length;

		// Handle normally.
		} else {
			_push_mml_event(p_event_id, p_data, p_length);
			// Data is the count of global events.
			if (_event_global_flags[p_event_id]) {
				_last_sequence_head->data++;
			}
		}
	}

	_is_last_event_length = false;
	return _last_event;
}

void MMLParser::_reset_state() {
	MMLEvent *event = _terminator->next;
	while (event) {
		event = free_event(event);
	}

	_system_event_index = 0;
	_sequence_mml_index = 0;
	_last_event = _terminator;
	_last_sequence_head = _push_mml_event(MMLEvent::SEQUENCE_HEAD, 0, 0);

	_reset_state_track();
}

void MMLParser::_reset_state_track() {
	_static_length = _settings->get_default_length();
	_static_octave = _settings->get_default_octave();
	_static_note_shift = 0;
	_is_last_event_length = false;

	_repeat_stack.clear();
	_head_mml_index = _mml_regex_last_index;
}

void MMLParser::prepare_parse(MMLParserSettings *p_settings, String p_mml) {
	_settings = p_settings;
	_mml_string = p_mml;
	_parsing_time = Time::get_singleton()->get_ticks_msec();

	_create_mml_regex(true);
	_reset_state();
}

int MMLParser::_parse_length(const Ref<RegExMatch> &p_res) {
	// This is an abbreviation, return INT32_MIN.
	if (p_res->get_string(REX_PARAM).length() == 0) {
		return INT32_MIN;
	}

	int length = p_res->get_string(REX_PARAM).to_int();
	if (length == 0) {
		return 0;
	}

	length = _settings->resolution / length;
	OP_ERR_FAIL_RANGE_V(length, 1, _settings->resolution, "length", 0);

	return length;
}

int MMLParser::_parse_param(const Ref<RegExMatch> &p_res, int p_default) {
	if (p_res->get_string(REX_PARAM).length() > 0) {
		return p_res->get_string(REX_PARAM).to_int();
	}
	return p_default;
}

int MMLParser::_parse_period(const Ref<RegExMatch> &p_res) {
	return p_res->get_string(REX_PERIOD).length();
}

MMLEvent *MMLParser::parse(int p_interrupt) {
	_interrupt_interval = p_interrupt;
	_start_time = Time::get_singleton()->get_ticks_msec();

	_create_mml_regex(false);

	// Start parsing.

	// TODO: Test the _mml_regex_last_index logic. In the original code it's a native property of the RegExp object,
	// which is updated automatically. We use it, update it, and keep track of it manually.
	TypedArray<RegExMatch> matches = _mml_regex->search_all(_mml_string, _mml_regex_last_index);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> res = matches[i];
		_mml_regex_last_index = res->get_end() + 1;

		if (res->get_string(0).is_empty()) {
			break; // Stop parsing if there is an empty match.
		}

		if (!res->get_string(REX_WHITESPACE).is_empty()) {
			continue; // This is a comment.
		}

		// If this gets set to true, we will exit early with an empty result.
		bool halt = false;

		// Note events.
		if (!res->get_string(REX_NOTE).is_empty()) {
			// We want to convert the a-g range to the c-b range. We are guaranteed
			// to have letters a through g from the regex, so we subtract the code of C.
			// Then, if we underflow, we correct it by shifting the value by 7.

			int note = res->get_string(REX_NOTE).unicode_at(0) - (int)'c';
			if (note < 0) {
				note += 7;
			}

			int shift = _key_signature[note];
			String shift_string = res->get_string(REX_NOTE_SHIFT);
			if (shift_string == "+" || shift_string == "#") {
				shift++;
			} else if (shift_string == "-") {
				shift--;
			}

			_op_note(_key_scale[note] + shift + _settings->get_mml_to_note_offset(), _parse_length(res), _parse_period(res));

		// User defined events.
		} else if (!res->get_string(REX_USER_EVENT).is_empty()) {
			String event_str = res->get_string(REX_USER_EVENT);
			ERR_CONTINUE_MSG(!_user_defined_event_map.has(event_str), vformat("MMLParser: Unknown user-defined event: '%s'.", event_str));
			_add_mml_event(_user_defined_event_map[event_str], _parse_param(res));

		// Standard events.
		} else if (!res->get_string(REX_EVENT).is_empty()) {
			String event_str = res->get_string(REX_EVENT);

			// Formatting below is enforced like this for readability.

			// Rest events.
			if (event_str == "r") {
				_op_rest(_parse_length(res), _parse_period(res));
			}

			// Length events.

			else if (event_str == "l") {
				_op_length(_parse_length(res), _parse_period(res));
			}
			else if (event_str == "^") {
				_op_tie(_parse_length(res), _parse_period(res));
			}
			else if (event_str == "&") {
				_op_slur();
			}
			else if (event_str == "&&") {
				_op_slur_weak();
			}
			else if (event_str == "*") {
				_op_portament();
			}
			else if (event_str == "q") {
				_op_quant(_parse_param(res, _settings->default_quant_ratio));
			}
			else if (event_str == "@q") {
				_op_at_quant(_parse_param(res, _settings->default_quant_count));
			}

			// Pitch events.

			else if (event_str == "o") {
				_op_octave(_parse_param(res, _settings->get_default_octave()));
			}
			else if (event_str == "<") {
				_op_octave_shift( _parse_param(res, 1));
			}
			else if (event_str == ">") {
				_op_octave_shift(-_parse_param(res, 1));
			}
			else if (event_str == "!@ns") {
				_op_note_shift( _parse_param(res, 0));
			}
			else if (event_str == "v") {
				_op_volume(_parse_param(res, _settings->default_volume));
			}
			else if (event_str == "@v") {
				_op_at_volume(_parse_param(res, _settings->default_fine_volume));
			}
			else if (event_str == "(") {
				_op_volume_shift( _parse_param(res, 1));
			}
			else if (event_str == ")") {
				_op_volume_shift(-_parse_param(res, 1));
			}

			// Repeat events.

			else if (event_str == "$") {
				_op_repeat_point();
			}
			else if (event_str == "[") {
				_op_repeat_begin(_parse_param(res, 2));
			}
			else if (event_str == "|") {
				_op_repeat_break();
			}
			else if (event_str == "]") {
				_op_repeat_end(_parse_param(res));
			}

			// Other events.

			else if (event_str == "%") {
				_op_mod_type(_parse_param(res));
			}
			else if (event_str == "@") {
				_op_mod_param(_parse_param(res));
			}
			else if (event_str == "@i") {
				_op_input(_parse_param(res, 0));
			}
			else if (event_str == "@o") {
				_op_output(_parse_param(res, 0));
			}
			else if (event_str == ",") {
				_op_parameter(_parse_param(res));
			}
			else if (event_str == "t") {
				_op_tempo(_parse_param(res, _settings->default_bpm));
			}

			else if (event_str == ";") {
				halt = _op_end_sequence();
			}

			else {
				ERR_CONTINUE_MSG(true, vformat("MMLParser: Unknown standard event: '%s'.", event_str));
			}

		// System events.
		} else if (!res->get_string(REX_SYSTEM).is_empty()) {
			ERR_FAIL_COND_V_MSG(_last_event->id != MMLEvent::SEQUENCE_HEAD, nullptr, "MMLParser: System commands are only allowed at the top of the channel sequence.");

			_add_mml_event(MMLEvent::SYSTEM_EVENT, _register_system_event_string(res->get_string(REX_SYSTEM)));

		// Table events.
		} else if (!res->get_string(REX_TABLE).is_empty()) {
			_add_mml_event(MMLEvent::TABLE_EVENT, _register_system_event_string(res->get_string(REX_TABLE)));

		// Invalid syntax.
		} else {
			ERR_FAIL_V_MSG(nullptr, vformat("MMLParser: Invalid syntax encountered: '%s'.", res->get_string(0)));
		}

		if (halt) {
			return nullptr;
		}
	}

	// Done parsing.

	ERR_FAIL_COND_V_MSG(_repeat_stack.size() != 0, nullptr, "MMLParser: Too many items in the repeat stack for command '['.");

	if (_last_event->id != MMLEvent::SEQUENCE_HEAD) {
		_last_sequence_head->jump = _last_event;
	}

	_parsing_time = Time::get_singleton()->get_ticks_msec() - _parsing_time;

	MMLEvent *head_event = _terminator->next;
	_terminator->next = nullptr;

	return head_event;
}

double MMLParser::get_parse_progress() {
	if (_mml_string.is_empty()) {
		return 0;
	}

	return (double)_mml_regex_last_index / (_mml_string.length() + 1);
}

MMLEvent *MMLParser::alloc_event(int p_event_id, int p_data, int p_length) {
	MMLEvent *event = nullptr;

	if (_free_event_chain) {
		event = _free_event_chain;
		_free_event_chain = _free_event_chain->next;
	} else {
		event = memnew(MMLEvent(0));
	}

	event->initialize(p_event_id, p_data, p_length);
	return event;
}

MMLEvent *MMLParser::free_event(MMLEvent *p_event) {
	MMLEvent *next = p_event->next;
	p_event->next = _free_event_chain;
	_free_event_chain = p_event;

	return next;
}

void MMLParser::free_all_events(MMLSequence *p_sequence) {
	if (!p_sequence->get_head_event()) {
		return;
	}

	p_sequence->get_tail_event()->next = _free_event_chain;
	_free_event_chain = p_sequence->get_head_event();

	p_sequence->set_head_event(nullptr);
	p_sequence->set_tail_event(nullptr);
}

// Parsing operations.

int MMLParser::_calculate_length(int p_length, int p_period) {
	int length = p_length;
	if (length == INT32_MIN) {
		length = _static_length;
	}

	int length_step = length;
	int period = p_period;
	while (period > 0) {
		length += length_step >> period;
		period--;
	}

	return length;
}

/// Note operations.

void MMLParser::_op_note(int p_note, int p_length, int p_period) {
	int note = p_note + _static_octave * 12 + _static_note_shift;
	note = CLAMP(note, 0, 127);

	_add_mml_event(MMLEvent::NOTE, note, _calculate_length(p_length, p_period));
}

void MMLParser::_op_rest(int p_length, int p_period) {
	_add_mml_event(MMLEvent::REST, 0, _calculate_length(p_length, p_period));
}

/// Length operations.

void MMLParser::_op_length(int p_length, int p_period) {
	_static_length = _calculate_length(p_length, p_period);
	_is_last_event_length = true;
}

void MMLParser::_op_tie(int p_length, int p_period) {
	if (_is_last_event_length) {
		_static_length += _calculate_length(p_length, p_period);
	} else if (_last_event->id == MMLEvent::REST || _last_event->id == MMLEvent::NOTE) {
		_last_event->length += _calculate_length(p_length, p_period);
	} else {
		ERR_FAIL_MSG("MMLParser: Invalid tie command syntax.");
	}
}

void MMLParser::_op_slur() {
	_add_mml_event(MMLEvent::SLUR, 0, 0, true);
}

void MMLParser::_op_slur_weak() {
	_add_mml_event(MMLEvent::SLUR_WEAK, 0, 0, true);
}

void MMLParser::_op_portament() {
	_add_mml_event(MMLEvent::PITCHBEND, 0, 0, true);
}

void MMLParser::_op_quant(int p_value) {
	OP_ERR_FAIL_RANGE(p_value, _settings->min_quant_ratio, _settings->max_quant_ratio, "q");
	_add_mml_event(MMLEvent::QUANT_RATIO, p_value);
}

void MMLParser::_op_at_quant(int p_value) {
	OP_ERR_FAIL_RANGE(p_value, _settings->min_quant_count, _settings->max_quant_count, "@q");
	_add_mml_event(MMLEvent::QUANT_COUNT, p_value);
}

/// Pitch operations.

void MMLParser::_op_octave(int p_value) {
	OP_ERR_FAIL_RANGE(p_value, _settings->min_octave, _settings->max_octave, "o");
	_static_octave = p_value;
}

void MMLParser::_op_octave_shift(int p_value) {
	_static_octave += p_value * _settings->octave_polarization;
}

void MMLParser::_op_note_shift(int p_value) {
	_static_note_shift += p_value;
}

void MMLParser::_op_volume(int p_value) {
	OP_ERR_FAIL_RANGE(p_value, 0, _settings->max_volume, "v");
	_add_mml_event(MMLEvent::VOLUME, p_value);
}

void MMLParser::_op_at_volume(int p_value) {
	OP_ERR_FAIL_RANGE(p_value, 0, _settings->max_fine_volume, "@v");
	_add_mml_event(MMLEvent::FINE_VOLUME, p_value);
}

void MMLParser::_op_volume_shift(int p_value) {
	int value = p_value * _settings->volume_polarization;

	if (_last_event->id == MMLEvent::VOLUME_SHIFT || _last_event->id == MMLEvent::VOLUME) {
		_last_event->data += value;
	} else {
		_add_mml_event(MMLEvent::VOLUME_SHIFT, value);
	}
}

/// Repeat operations.

void MMLParser::_op_repeat_point() {
	_add_mml_event(MMLEvent::REPEAT_ALL, 0);
}

void MMLParser::_op_repeat_begin(int p_count) {
	OP_ERR_FAIL_RANGE(p_count, 1, 65535, "[");
	_add_mml_event(MMLEvent::REPEAT_BEGIN, p_count, 0);
	_repeat_stack.push_front(_last_event);
}

void MMLParser::_op_repeat_break() {
	ERR_FAIL_COND_MSG(_repeat_stack.size() == 0, "MMLParser: Not enough items in the repeat stack for command '|'.");
	_add_mml_event(MMLEvent::REPEAT_BREAK);
	_last_event->jump = _repeat_stack[0];
}

void MMLParser::_op_repeat_end(int p_count) {
	ERR_FAIL_COND_MSG(_repeat_stack.size() == 0, "MMLParser: Not enough items in the repeat stack for command ']'.");
	_add_mml_event(MMLEvent::REPEAT_END);

	MMLEvent *begin_event = _repeat_stack.front()->get();
	_repeat_stack.pop_front();

	_last_event->jump = begin_event;
	begin_event->jump = _last_event;

	if (p_count != INT32_MIN) {
		OP_ERR_FAIL_RANGE(p_count, 1, 65535, "]");
		begin_event->data = p_count;
	}
}

/// Other operations.

void MMLParser::_op_mod_type(int p_type) {
	_add_mml_event(MMLEvent::MOD_TYPE, p_type);
}

void MMLParser::_op_mod_param(int p_param) {
	_add_mml_event(MMLEvent::MOD_PARAM, p_param);
}

void MMLParser::_op_input(int p_pipe) {
	_add_mml_event(MMLEvent::INPUT_PIPE, p_pipe);
}

void MMLParser::_op_output(int p_pipe) {
	_add_mml_event(MMLEvent::OUTPUT_PIPE, p_pipe);
}

void MMLParser::_op_parameter(int p_param) {
	_add_mml_event(MMLEvent::PARAMETER, p_param);
}

void MMLParser::_op_tempo(int p_tempo) {
	_add_mml_event(MMLEvent::TEMPO, p_tempo);
}

bool MMLParser::_op_end_sequence() {
	if (_last_event->id != MMLEvent::SEQUENCE_HEAD) {
		if (_last_sequence_head->next && _last_sequence_head->next->id == MMLEvent::DEBUG_INFO) {
			_last_sequence_head->next->data = _register_sequence_mml_strings(_mml_string.substr(_head_mml_index, _head_mml_index + _mml_regex_last_index));
		}

		_add_mml_event(MMLEvent::SEQUENCE_HEAD, 0);
		// Returns true when it has to interrupt.
		if (_interrupt_interval == 0) {
			return false;
		}
		return _interrupt_interval < (Time::get_singleton()->get_ticks_msec() - _start_time);
	}

	return false;
}

#undef OP_ERR_FAIL_RANGE

//

MMLParser::MMLParser() {
	if (!_instance) {
		_instance = this; // Do this early so it can be self-referenced.
	}

	// This is only the initial size, it will grow automatically as needed.
	_system_event_strings.resize_zeroed(32);
	_sequence_mml_strings.resize_zeroed(32);

	_key_signature_custom.resize_zeroed(7);

	_terminator = memnew(MMLEvent(0));
}

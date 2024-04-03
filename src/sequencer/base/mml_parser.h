/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_PARSER_H
#define MML_PARSER_H

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/classes/reg_ex.hpp>

using namespace godot;

class MMLEvent;
class MMLParserSettings;
class MMLSequence;

// This is a stateful class, using it with multiple sequencers/drivers may cause issues.
class MMLParser {

	static MMLParser *_instance;

	// Settings.

	MMLParserSettings *_settings = nullptr;
	String _mml_string;

	HashMap<String, int> _user_defined_event_map;
	Vector<bool> _event_global_flags;

	Vector<String> _system_event_strings;
	Vector<String> _sequence_mml_strings;

	int _register_system_event_string(String p_event);
	int _register_sequence_mml_strings(String p_mml);

	// This is just a helper to make the code clearer when we access
	// substrings parsed by the regex.
	enum MMLRegexIndex {
        REX_WHITESPACE = 1,
        REX_SYSTEM     = 2,
        REX_COMMAND    = 3,
        REX_NOTE       = 4,
        REX_NOTE_SHIFT = 5,
        REX_USER_EVENT = 6,
        REX_EVENT      = 7,
        REX_TABLE      = 8,
        REX_PARAM      = 9,
        REX_PERIOD     = 10,
	};

	Ref<RegEx> _mml_regex;
	// Starting offset for subsequent searches. In the original code it's a part of the RegExp object.
	int _mml_regex_last_index = 0;

	void _create_mml_regex(bool p_reset);
	void _clear_mml_regex();

	// Key.

	Vector<Vector<int>> _key_signature_table = {
		{  0, 0, 0, 0, 0, 0, 0 },
		{  0, 0, 0, 1, 0, 0, 0 },
		{  1, 0, 0, 1, 0, 0, 0 },
		{  1, 0, 0, 1, 1, 0, 0 },
		{  1, 1, 0, 1, 1, 0, 0 },
		{  1, 1, 0, 1, 1, 1, 0 },
		{  1, 1, 1, 1, 1, 1, 0 },
		{  1, 1, 1, 1, 1, 1, 1 },
		{  0, 0, 0, 0, 0, 0,-1 },
		{  0, 0,-1, 0, 0, 0,-1 },
		{  0, 0,-1, 0, 0,-1,-1 },
		{  0,-1,-1, 0, 0,-1,-1 },
		{  0,-1,-1, 0,-1,-1,-1 },
		{ -1,-1,-1, 0,-1,-1,-1 },
		{ -1,-1,-1,-1,-1,-1,-1 }
	};

	Vector<int> _key_scale = { 0,2,4,5,7,9,11 };
	Vector<int> _key_signature = _key_signature_table[0];
	Vector<int> _key_signature_custom;

	// Parsing and events.

	MMLEvent *_free_event_chain = nullptr;

	int _system_event_index = 0;
	int _sequence_mml_index = 0;
	int _head_mml_index = 0;
	bool _is_last_event_length = false;

	MMLEvent *_terminator = nullptr;
	MMLEvent *_last_event = nullptr;
	MMLEvent *_last_sequence_head = nullptr;
	List<MMLEvent *> _repeat_stack;

	MMLEvent *_push_mml_event(int p_event_id, int p_data, int p_length);
	MMLEvent *_add_mml_event(int p_event_id, int p_data = 0, int p_length = 0, bool p_note_option = false);
	void _reset_state();
	void _reset_state_track();

	int _parse_length(const Ref<RegExMatch> &p_res);
	int _parse_param(const Ref<RegExMatch> &p_res, int p_default = INT32_MIN);
	int _parse_period(const Ref<RegExMatch> &p_res);

	// Timers.

	int _interrupt_interval = 0;
	int _start_time = 0;
	int _parsing_time = 0;

	// Static values.

	int _static_length = 0;
	int _static_octave = 0;
	int _static_note_shift = 0;

	// Parsing operations.

	int _calculate_length(int p_length, int p_period);

	/// Note operations.

	void _op_note(int p_note, int p_length, int p_period);
	void _op_rest(int p_length, int p_period);

	/// Length operations.

	void _op_length(int p_length, int p_period);
	void _op_tie(int p_length, int p_period);
	void _op_slur();
	void _op_slur_weak();
	void _op_portament();
	// Gate time.
	void _op_quant(int p_value);
	// Absolute gate time.
	void _op_at_quant(int p_value);

	/// Pitch operations.

	void _op_octave(int p_value);
	void _op_octave_shift(int p_value);
	void _op_note_shift(int p_value);
	void _op_volume(int p_value);
	// Fine volume
	void _op_at_volume(int p_value);
	void _op_volume_shift(int p_value);

	/// Repeat operations.

	void _op_repeat_point();
	void _op_repeat_begin(int p_count);
	void _op_repeat_break();
	void _op_repeat_end(int p_count);

	/// Other operations.

	void _op_mod_type(int p_type);
	void _op_mod_param(int p_param);
	void _op_input(int p_pipe);
	void _op_output(int p_pipe);
	void _op_parameter(int p_param);
	void _op_tempo(int p_tempo);

	bool _op_end_sequence();

public:
	static MMLParser *get_instance() { return _instance; }
	static void initialize();
	static void finalize() {}

	// Settings.

	void set_user_defined_event_map(HashMap<String, int> p_event_map);
	void set_global_event_flags(Vector<bool> p_event_flags);

	void get_command_letters(HashMap<int, String> *r_letter_map);
	String get_system_event_string(MMLEvent *p_event);
	String get_sequence_mml(MMLEvent *p_event);

	// Key.

	// The string for the signature is expected in the following format: /[A-G][+\-#b]?m?/.
	// A custom signature can be set by listing multiple signatures in the same manner,
	// separated by a space or a comma.
	void set_key_signature(String p_sign);

	// Parsing and events.

	void prepare_parse(MMLParserSettings *p_settings, String p_mml);
	// Takes the interval for interruptions, in msec. 0 means no interruptions. Interruptions occur between sequences.
	MMLEvent *parse(int p_interrupt = 0);
	double get_parse_progress();

	MMLEvent *alloc_event(int p_event_id, int p_data, int p_length = 0);
	MMLEvent *free_event(MMLEvent *p_event);
	void free_all_events(MMLSequence *p_sequence);

	//
	MMLParser();
	~MMLParser() {}
};

#endif // MML_PARSER_H

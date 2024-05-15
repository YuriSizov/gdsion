/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_SEQUENCER_H
#define MML_SEQUENCER_H

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_data.h"

class BeatsPerMinute;
class MMLExecutor;
class MMLParserSettings;
class MMLSequence;
class MMLSequenceGroup;

// MMLSequencer is the base class that bridges MMLEvents, sound modules, and sound systems.
//
// You should follow this in your inherited classes:
// 1) Register MML event listeners by set_mml_event_listener() or new_mml_event_listener().
// 2) Override on...() functions.
// 3) Override prepare_compile() and compile() if necessary.
// 4) Override prepare_process() and process() to process audio data.
//
// Then your implementation can be used as follows:
// 1) Call initialize() to initialize.
// 2) Call prepare_compile() and compile() to compile the MML string to MMLData.
// 3) Call prepare_process() and process() to process audio in inherited class.
class MMLSequencer : public Object {
	GDCLASS(MMLSequencer, Object)

	static MMLExecutor *_temp_executor;

	// Events.

	int _next_user_defined_event_id = MMLEvent::USER_DEFINED;
	HashMap<String, int> _user_defined_event_map;
	HashMap<int, String> _event_command_letter_map;

	Vector<Callable> _event_handlers;
	Vector<bool> _event_global_flags;

	// Event handlers.

	MMLEvent *_no_process(MMLEvent *p_event);
	MMLEvent *_dummy_on_process(MMLEvent *p_event);       // MMLEvent::PROCESS
	MMLEvent *_dummy_on_process_event(MMLEvent *p_event); // Other process events.

	MMLEvent *_default_on_no_operation(MMLEvent *p_event);  // MMLEvent::NOP
	MMLEvent *_default_on_global_wait(MMLEvent *p_event);   // MMLEvent::GLOBAL_WAIT
	MMLEvent *_default_on_process(MMLEvent *p_event);       // MMLEvent::PROCESS
	MMLEvent *_default_on_repeat_all(MMLEvent *p_event);    // MMLEvent::REPEAT_ALL
	MMLEvent *_default_on_repeat_begin(MMLEvent *p_event);  // MMLEvent::REPEAT_BEGIN
	MMLEvent *_default_on_repeat_break(MMLEvent *p_event);  // MMLEvent::REPEAT_BREAK
	MMLEvent *_default_on_repeat_end(MMLEvent *p_event);    // MMLEvent::REPEAT_END
	MMLEvent *_default_on_sequence_tail(MMLEvent *p_event); // MMLEvent::SEQUENCE_TAIL
	MMLEvent *_default_on_tempo(MMLEvent *p_event);         // MMLEvent::TEMPO
	MMLEvent *_default_on_timer(MMLEvent *p_event);         // MMLEvent::TIMER
	MMLEvent *_default_on_internal_wait(MMLEvent *p_event); // MMLEvent::INTERNAL_WAIT
	MMLEvent *_default_on_internal_call(MMLEvent *p_event); // MMLEvent::INTERNAL_CALL

	// Compilation and processing.

	// Leftover of buffer sample count in processing.
	int _process_buffer_sample_count = 0;
	// Leftover of buffer sample count in global sequence.
	int _global_buffer_sample_count = 0;
	// Executing buffer length in global sequence.
	int _global_execute_sample_count = 0;

	int _buffer_length = 0;

	// Compilation and processing handlers.

	void _extract_global_sequence();

	// To be implemented by extending classes.

	// Takes the MML string to parse and returns a new MML string. Returning an empty string
	// means that the original string will be parsed.
	virtual String _on_before_compile(String p_mml) { return String(); }
	virtual void _on_after_compile(MMLSequenceGroup *p_group) {}
	virtual void _on_process(int p_length, MMLEvent *p_event) {}
	virtual void _on_timer_interruption() {}
	virtual void _on_beat(int p_delay_samples, int p_beat_counter) {}
	virtual void _on_table_parse(MMLEvent *p_prev, String p_table) {}
	virtual void _on_tempo_changed(double p_tempo_ratio) {}

protected:
	MMLParserSettings *_parser_settings = nullptr;
	int _sample_rate = 44100;

	MMLExecutor *_global_executor = nullptr;
	MMLExecutor *_current_executor = nullptr;
	Ref<MMLData> mml_data;
	Ref<BeatsPerMinute> _adjustible_bpm;
	Ref<BeatsPerMinute> _bpm;

	int _global_buffer_index = 0;
	double _global_beat_16th = 0;
	// Filter for the on-beat callback, 0 = 16th beat, 1 = 8th beat, 3 = 4th beat, 7 = 2nd beat, 15 = whole tone.
	int _on_beat_callback_filter = 3;

	// Events.

	void _set_mml_event_listener(int p_event_id, const Callable &p_handler, bool p_global = false);
	int _create_mml_event_listener(String p_letter, const Callable &p_handler, bool p_global = false);

	//

	static void _bind_methods();

public:
	static const int FIXED_BITS = 8;
	// Filter for decimal fraction area.
	static const int FIXED_FILTER = (1 << FIXED_BITS) - 1;

	static void initialize();
	static MMLExecutor *get_temp_executor() { return _temp_executor; }

	MMLParserSettings *get_parser_settings() const { return _parser_settings; }
	int get_sample_rate() const { return _sample_rate; }

	double get_default_bpm() const;
	void set_default_bpm(double p_value);

	double get_bpm() const;
	void set_bpm(double p_value);

	// Events.

	int get_event_id(String p_mml_command);
	String get_event_letter(int p_event_id);

	// Compilation and processing.

	// Returns false if compilation is not needed.
	virtual bool prepare_compile(const Ref<MMLData> &p_data, String p_mml);
	// Returns compilation progress [0-1].
	virtual double compile(int p_interval = 1000);
	virtual void prepare_process(const Ref<MMLData> &p_data, int p_sample_rate, int p_buffer_length);
	virtual void process() {}

	// Must be called between prepare_process() and process().
	void set_global_sequence(MMLSequence *p_sequence);
	void start_global_sequence();
	// Returns the executed sample count.
	int execute_global_sequence();
	bool check_global_sequence_end();

	// Process audio by one executor. Returns true if the sequence has ended.
	bool process_executor(MMLExecutor *p_executor, int p_buffer_sample_count);

	int calculate_sample_count(int p_length);
	double calculate_sample_length(double p_beat_16th);
	double calculate_sample_delay(int p_sample_offset = 0, double p_beat_16th_offset = 0, double p_quant = 0);

	int get_current_tick_count();
	void parse_table_event(MMLEvent *p_prev);

	//

	MMLSequencer();
	~MMLSequencer();
};

#endif // MML_SEQUENCER_H

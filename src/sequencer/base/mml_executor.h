/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_EXECUTOR_H
#define MML_EXECUTOR_H

#include "templates/singly_linked_list.h"

class MMLEvent;
class MMLSequence;

// MMLExecutor has MMLSequence and executing pointer. Each track has one executor,
// and the sequencer also has one for the global sequence.
class MMLExecutor {

	MMLSequence *_sequence = nullptr;
	MMLEvent *_pointer = nullptr;

	int _repeat_end_counter = 0;
	MMLEvent *_repeat_point = nullptr;
	MMLEvent *_process_event = nullptr;
	MMLEvent *_bend_from = nullptr;
	MMLEvent *_bend_event = nullptr;
	MMLEvent *_note_event = nullptr;

	int _current_tick_count = 0;
	// Stack of counters.
	SinglyLinkedList<int> *_repeat_counters = nullptr;
	int _residue_sample_count = 0;
	int _decimal_fraction_sample_count = 0;

public:
	MMLSequence *get_sequence() const { return _sequence; }
	MMLEvent *get_pointer() const { return _pointer; }
	void set_pointer(MMLEvent *p_event) { _pointer = p_event; }
	MMLEvent *get_current_event() const;

	int get_repeat_end_counter() const { return _repeat_end_counter; }
	// Note that's awaiting "note on" execution, or -1.
	int get_waiting_note() const;

	int get_current_tick_count() const { return _current_tick_count; }
	int get_residue_sample_count() const { return _residue_sample_count; }
	void set_residue_sample_count(int p_value) { _residue_sample_count = p_value; }
	void adjust_residue_sample_count(int p_diff) { _residue_sample_count += p_diff; }
	int get_decimal_fraction_sample_count() const { return _decimal_fraction_sample_count; }
	void set_decimal_fraction_sample_count(int p_value) { _decimal_fraction_sample_count = p_value; }

	// Execution.

	void reset_pointer();
	void stop();
	void execute_single_note(int p_note, int p_tick_length);
	bool pitch_bend_from(int p_note, int p_tick_length);

	MMLEvent *publish_processing_event(MMLEvent *p_event);

	// Handlers.

	void on_tempo_changed(double p_changing_ratio);
	MMLEvent *on_repeat_all(MMLEvent *p_event);
	MMLEvent *on_repeat_begin(MMLEvent *p_event);
	MMLEvent *on_repeat_break(MMLEvent *p_event);
	MMLEvent *on_repeat_end(MMLEvent *p_event);
	MMLEvent *on_sequence_tail(MMLEvent *p_event);

	//

	void initialize(MMLSequence *p_sequence);
	void clear();

	MMLExecutor();
	~MMLExecutor() {}
};

#endif // MML_EXECUTOR_H

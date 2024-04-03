/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_SEQUENCE_H
#define MML_SEQUENCE_H

#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/callable.hpp>

using namespace godot;

class MMLData;
class MMLEvent;

// Sequence of 1 sound channel. MMLData > MMLSequenceGroup > MMLSequence > MMLEvent (">" means "has a").
class MMLSequence {

	MMLData *_owner = nullptr;

	// Chain of sequences.

	MMLSequence *_prev_sequence = nullptr;
	MMLSequence *_next_sequence = nullptr;
	bool _is_terminal = false;

	// Events.

	// First MMLEvent. The ID is always MMLEvent::SEQUENCE_HEAD.
	MMLEvent *_head_event = nullptr;
	// Last MMLEvent. The ID is always MMLEvent::SEQUENCE_TAIL and tail_event->next is always null.
	MMLEvent *_tail_event = nullptr;
	// The sequence is skipped to play when this value is false.
	bool _is_active = true;

	// Callback functions for Event::INTERNAL_CALL.
	List<Callable> _callbacks_for_internal_call;

	// MML string.

	String _mml_string;
	// Length in resolution units (1920 = whole-tone in default).
	int _mml_length = -1;
	bool _has_repeat_all = false;

	void _update_mml_length();

public:
	MMLData *get_owner() const { return _owner; }
	void set_owner(MMLData *p_owner) { _owner = p_owner; }

	// Chain of sequences.

	void insert_before(MMLSequence *p_next);
	void insert_after(MMLSequence *p_prev);
	MMLSequence *remove_from_chain();
	MMLSequence *get_next_sequence() const;

	// Temporarily connect to sequences via the head event pointer. Call connect_before(nullptr) to unset the connection.
	void connect_before(MMLEvent *p_second_head);

	// Events.

	bool is_empty() const;
	bool is_active() const { return _is_active; }
	void set_active(bool p_active) { _is_active = p_active; }

	bool is_system_command() const;
	String get_system_command() const;

	MMLEvent *get_head_event() const { return _head_event; }
	void set_head_event(MMLEvent *p_event) { _head_event = p_event; }
	MMLEvent *get_tail_event() const { return _tail_event; }
	void set_tail_event(MMLEvent *p_event) { _tail_event = p_event; }

	MMLEvent *append_new_event(int p_event_id, int p_data, int p_length = 0);
	MMLEvent *append_new_callback(const Callable &p_callback, int p_data);
	MMLEvent *prepend_new_event(int p_event_id, int p_data, int p_length = 0);

	void push_back(MMLEvent *p_event);
	void push_front(MMLEvent *p_event);
	MMLEvent *pop_back();
	MMLEvent *pop_front();
	MMLEvent *cutout(MMLEvent *p_head);

	List<Callable> get_callbacks_for_internal_call() const { return _callbacks_for_internal_call; }

	// MML string.

	String get_mml_string() const { return _mml_string; }
	int get_mml_length();
	bool has_repeat_all();

	void update_mml_string();

	//

	String to_string() const;
	List<MMLEvent *> to_vector(int p_max_length = 0, int p_offset = 0, int p_event_id = -1);
	void from_vector(List<MMLEvent *> p_events);

	void initialize();
	void free();

	MMLSequence(bool p_terminal = false);
	~MMLSequence() {}
};

#endif // MML_SEQUENCE_H

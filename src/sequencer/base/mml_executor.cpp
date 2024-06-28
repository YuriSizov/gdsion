/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_executor.h"

#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_parser.h"
#include "sequencer/base/mml_sequence.h"

MMLEvent *MMLExecutor::get_current_event() const {
	return _pointer == _process_event ? _pointer->get_jump() : _pointer;
}

int MMLExecutor::get_waiting_note() const {
	return _pointer == _note_event ? _note_event->get_data() : -1;
}

// Execution.

void MMLExecutor::reset_pointer() {
	if (!_sequence) {
		return;
	}

	_pointer = _sequence->get_head_event()->get_next();

	_repeat_end_counter = 0;
	_repeat_point = nullptr;

	SinglyLinkedList<int>::free_list(_repeat_counters);
	_repeat_counters = nullptr;
	_current_tick_count = 0;
	_residue_sample_count = 0;
	_decimal_fraction_sample_count = 0;
}

void MMLExecutor::stop() {
	if (!_pointer) {
		return;
	}

	if (_pointer == _process_event) {
		_process_event->set_jump(_nop_event);
	} else {
		_pointer = nullptr;
	}
}

void MMLExecutor::execute_single_note(int p_note, int p_tick_length) {
	_note_event->set_next(nullptr);
	_note_event->set_data(p_note);
	_note_event->set_length(p_tick_length);
	_pointer = _note_event;

	_sequence = nullptr;

	_repeat_end_counter = 0;
	_repeat_point = nullptr;

	SinglyLinkedList<int>::free_list(_repeat_counters);
	_repeat_counters = nullptr;
	_current_tick_count = 0;
}

bool MMLExecutor::pitch_bend_from(int p_note, int p_tick_length) {
	if (_pointer != _note_event || p_tick_length == 0) {
		return false;
	}

	int tick_length = p_tick_length;
	if (_note_event->get_length() != 0) {
		// TODO: This looks suspicious.
		if (tick_length < _note_event->get_length()) {
			tick_length = _note_event->get_length() - 1;
		}
		_note_event->set_length(_note_event->get_length() - tick_length);
	}

	_bend_from_event->set_length(0);
	_bend_from_event->set_data(p_note);
	_bend_event->set_length(tick_length);
	_pointer = _bend_from_event;

	return true;
}

MMLEvent *MMLExecutor::publish_processing_event(MMLEvent *p_event) {
	if (p_event->get_length() > 0) {
		_current_tick_count += p_event->get_length();

		_process_event->set_length(p_event->get_length());
		_process_event->set_jump(p_event);

		return _process_event;
	}

	return p_event->get_next();
}

// Handlers.

void MMLExecutor::on_tempo_changed(double p_changing_ratio) {
	double ratio = p_changing_ratio;
	if (_residue_sample_count < 0) {
		ratio = 1.0 / ratio;
	}

	_residue_sample_count *= ratio;
	_decimal_fraction_sample_count *= ratio;
}

MMLEvent *MMLExecutor::on_repeat_all(MMLEvent *p_event) {
	_repeat_point = p_event->get_next();

	return p_event->get_next();
}

MMLEvent *MMLExecutor::on_repeat_begin(MMLEvent *p_event) {
	SinglyLinkedList<int> *counter = SinglyLinkedList<int>::alloc(p_event->get_data());
	counter->next = _repeat_counters;
	_repeat_counters = counter;

	return p_event->get_next();
}

MMLEvent *MMLExecutor::on_repeat_break(MMLEvent *p_event) {
	if (_repeat_counters->value == 1) {
		SinglyLinkedList<int> *counter = _repeat_counters->next;
		SinglyLinkedList<int>::free(_repeat_counters);
		_repeat_counters = counter;

		// Jump to repeat start -> repeat end -> next.
		return p_event->get_jump()->get_jump()->get_next();
	}

	return p_event->get_next();
}

MMLEvent *MMLExecutor::on_repeat_end(MMLEvent *p_event) {
	_repeat_counters->value -= 1;
	if (_repeat_counters->value == 0) {
		SinglyLinkedList<int> *counter = _repeat_counters->next;
		SinglyLinkedList<int>::free(_repeat_counters);
		_repeat_counters = counter;

		return p_event->get_next();
	}

	// Jump to repeat start -> next.
	return p_event->get_jump()->get_next();
}

MMLEvent *MMLExecutor::on_sequence_tail(MMLEvent *p_event) {
	_repeat_end_counter++;

	return _repeat_point;
}

//

void MMLExecutor::initialize(MMLSequence *p_sequence) {
	clear();

	if (p_sequence) {
		_sequence = p_sequence;
		_pointer = _sequence->get_head_event()->get_next();
	}
}

void MMLExecutor::clear() {
	_sequence = nullptr;
	_pointer = nullptr;

	_repeat_end_counter = 0;
	_repeat_point = nullptr;

	SinglyLinkedList<int>::free_list(_repeat_counters);
	_repeat_counters = nullptr;
	_current_tick_count = 0;
	_residue_sample_count = 0;
	_decimal_fraction_sample_count = 0;
}

MMLExecutor::MMLExecutor() {
	_nop_event = MMLParser::get_instance()->alloc_event(MMLEvent::NO_OP, 0);
	_process_event = MMLParser::get_instance()->alloc_event(MMLEvent::PROCESS, 0);
	_note_event = MMLParser::get_instance()->alloc_event(MMLEvent::DRIVER_NOTE, 0);
	_bend_from_event = MMLParser::get_instance()->alloc_event(MMLEvent::NOTE, 0);
	_bend_event = MMLParser::get_instance()->alloc_event(MMLEvent::PITCHBEND, 0);

	_bend_from_event->set_next(_bend_event);
	_bend_event->set_next(_note_event);
}

MMLExecutor::~MMLExecutor() {
	MMLParser::get_instance()->free_event(_nop_event);
	MMLParser::get_instance()->free_event(_process_event);
	MMLParser::get_instance()->free_event(_note_event);
	MMLParser::get_instance()->free_event(_bend_from_event);
	MMLParser::get_instance()->free_event(_bend_event);

	_nop_event = nullptr;
	_process_event = nullptr;
	_note_event = nullptr;
	_bend_from_event = nullptr;
	_bend_event = nullptr;
}

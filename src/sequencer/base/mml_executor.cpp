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
	return _pointer == _process_event ? _pointer->jump : _pointer;
}

int MMLExecutor::get_waiting_note() const {
	return _pointer == _note_event ? _note_event->data : -1;
}

// Execution.

void MMLExecutor::reset_pointer() {
	if (!_sequence) {
		return;
	}

	_pointer = _sequence->get_head_event()->next;

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
		_process_event->jump = MMLEvent::get_nop_event();
	} else {
		_pointer = nullptr;
	}
}

void MMLExecutor::execute_single_note(int p_note, int p_tick_length) {
	_note_event->next = nullptr;
	_note_event->data = p_note;
	_note_event->length = p_tick_length;
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
	if (_note_event->length != 0) {
		// TODO: This looks suspicious.
		if (tick_length < _note_event->length) {
			tick_length = _note_event->length - 1;
		}
		_note_event->length -= tick_length;
	}

	_bend_from->length = 0;
	_bend_from->data = p_note;
	_bend_event->length = tick_length;
	_pointer = _bend_from;

	return true;
}

MMLEvent *MMLExecutor::publish_processing_event(MMLEvent *p_event) {
	if (p_event->length > 0) {
		_current_tick_count += p_event->length;

		_process_event->length = p_event->length;
		_process_event->jump = p_event;

		return _process_event;
	}

	return p_event->next;
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
	_repeat_point = p_event->next;

	return p_event->next;
}

MMLEvent *MMLExecutor::on_repeat_begin(MMLEvent *p_event) {
	SinglyLinkedList<int> *counter = SinglyLinkedList<int>::alloc(p_event->data);
	counter->next = _repeat_counters;
	_repeat_counters = counter;

	return p_event->next;
}

MMLEvent *MMLExecutor::on_repeat_break(MMLEvent *p_event) {
	if (_repeat_counters->value == 1) {
		SinglyLinkedList<int> *counter = _repeat_counters->next;
		SinglyLinkedList<int>::free(_repeat_counters);
		_repeat_counters = counter;

		// Jump to repeat start -> repeat end -> next.
		return p_event->jump->jump->next;
	}

	return p_event->next;
}

MMLEvent *MMLExecutor::on_repeat_end(MMLEvent *p_event) {
	_repeat_counters->value -= 1;
	if (_repeat_counters->value == 0) {
		SinglyLinkedList<int> *counter = _repeat_counters->next;
		SinglyLinkedList<int>::free(_repeat_counters);
		_repeat_counters = counter;

		return p_event->next;
	}

	// Jump to repeat start -> next.
	return p_event->jump->next;
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
		_pointer = _sequence->get_head_event()->next;
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
	_process_event = MMLParser::get_instance()->alloc_event(MMLEvent::PROCESS, 0);
	_note_event = MMLParser::get_instance()->alloc_event(MMLEvent::DRIVER_NOTE, 0);
	_bend_from = MMLParser::get_instance()->alloc_event(MMLEvent::NOTE, 0);
	_bend_event = MMLParser::get_instance()->alloc_event(MMLEvent::PITCHBEND, 0);
	_bend_from->next = _bend_event;
	_bend_event->next = _note_event;
}

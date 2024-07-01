/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_sequence.h"

#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_executor.h"
#include "sequencer/base/mml_parser.h"
#include "sequencer/base/mml_sequencer.h"

// Chain of sequences.

void MMLSequence::insert_before(MMLSequence *p_next) {
	_prev_sequence = p_next->_prev_sequence;
	_next_sequence = p_next;
	_prev_sequence->_next_sequence = this;
	_next_sequence->_prev_sequence = this;
}

void MMLSequence::insert_after(MMLSequence *p_prev) {
	_prev_sequence = p_prev;
	_next_sequence = p_prev->_next_sequence;
	_prev_sequence->_next_sequence = this;
	_next_sequence->_prev_sequence = this;
}

MMLSequence *MMLSequence::get_next_sequence() const {
	if (_next_sequence->_is_terminal) {
		return nullptr;
	}
	return _next_sequence;
}

MMLSequence *MMLSequence::remove_from_chain() {
	MMLSequence *ret = _prev_sequence;

	_prev_sequence->_next_sequence = _next_sequence;
	_next_sequence->_prev_sequence = _prev_sequence;
	_prev_sequence = nullptr;
	_next_sequence = nullptr;

	if (ret == this) {
		return nullptr;
	}
	return ret;
}

void MMLSequence::connect_before(MMLEvent *p_second_head) {
	// Simply connect first tail to second head.
	if (p_second_head) {
		_head_event->get_jump()->set_next(p_second_head);
	} else {
		_head_event->get_jump()->set_next(_tail_event);
	}
}

// Events.

bool MMLSequence::is_empty() const {
	return _head_event == nullptr;
}

bool MMLSequence::is_system_command() const {
	return _head_event->get_next()->get_id() == MMLEvent::SYSTEM_EVENT;
}

String MMLSequence::get_system_command() const {
	return MMLParser::get_instance()->get_system_event_string(_head_event->get_next());
}

MMLEvent *MMLSequence::append_new_event(int p_event_id, int p_data, int p_length) {
	MMLEvent *event = MMLParser::get_instance()->alloc_event(p_event_id, p_data, p_length);
	push_back(event);

	return event;
}

MMLEvent *MMLSequence::append_new_callback(const Callable &p_callback, int p_data) {
	_callbacks_for_internal_call.push_back(p_callback);
	MMLEvent *event = MMLParser::get_instance()->alloc_event(MMLEvent::INTERNAL_CALL, _callbacks_for_internal_call.size() - 1, p_data);
	push_back(event);

	return event;
}

MMLEvent *MMLSequence::prepend_new_event(int p_event_id, int p_data, int p_length) {
	MMLEvent *event = MMLParser::get_instance()->alloc_event(p_event_id, p_data, p_length);
	push_front(event);

	return event;
}

void MMLSequence::push_back(MMLEvent *p_event) {
	_head_event->get_jump()->set_next(p_event);
	p_event->set_next(_tail_event);

	_head_event->set_jump(p_event);
}

void MMLSequence::push_front(MMLEvent *p_event) {
	p_event->set_next(_head_event->get_next());
	_head_event->set_next(p_event);

	if (_head_event->get_jump() == _head_event) {
		_head_event->set_jump(p_event);
	}
}

MMLEvent *MMLSequence::pop_back() {
	if (_head_event->get_jump() == _head_event) {
		return nullptr;
	}

	MMLEvent *event = _head_event->get_next();
	while (event) {
		if (event->get_next() == _head_event->get_jump()) {
			MMLEvent *ret = event->get_next();
			event->set_next(_tail_event);
			_head_event->set_jump(event);

			ret->set_next(nullptr);
			return ret;
		}

		event = event->get_next();
	}

	return nullptr;
}

MMLEvent *MMLSequence::pop_front() {
	if (_head_event->get_jump() == _head_event) {
		return nullptr;
	}

	MMLEvent *ret = _head_event->get_next();
	_head_event->set_next(ret->get_next());

	ret->set_next(nullptr);
	return ret;
}

MMLEvent *MMLSequence::cutout(MMLEvent *p_head) {
	MMLEvent *last = p_head->get_jump(); // Last event of this sequence.
	MMLEvent *next = last->get_next(); // Head of next sequence.

	_head_event = p_head;
	_tail_event = MMLParser::get_instance()->alloc_event(MMLEvent::SEQUENCE_TAIL, 0);
	last->set_next(_tail_event);

	return next;
}

// MML string.

void MMLSequence::_update_mml_length() {
	MMLExecutor *exec = MMLSequencer::get_temp_executor();
	exec->initialize(this);

	_has_repeat_all = false;
	int length = 0;

	MMLEvent *event = _head_event->get_next();
	while (event) {
		// Note or rest.
		if (event->get_length() != 0) {
			length += event->get_length();
			event = event->get_next();
			continue;
		}

		// Everything else.
		switch (event->get_id()) {
			case MMLEvent::REPEAT_BEGIN: {
				event = exec->on_repeat_begin(event);
			} break;

			case MMLEvent::REPEAT_BREAK: {
				event = exec->on_repeat_break(event);
			} break;

			case MMLEvent::REPEAT_END: {
				event = exec->on_repeat_end(event);
			} break;

			case MMLEvent::REPEAT_ALL: {
				event = nullptr;
				_has_repeat_all = true;
			} break;

			case MMLEvent::SEQUENCE_TAIL: {
				event = nullptr;
			} break;

			default: {
				event = event->get_next();
			} break;
		}
	}

	_mml_length = length;
}

int MMLSequence::get_mml_length() {
	if (_mml_length == -1) {
		_update_mml_length();
	}
	return _mml_length;
}

bool MMLSequence::has_repeat_all() {
	if (_mml_length == -1) {
		_update_mml_length();
	}
	return _has_repeat_all;
}

void MMLSequence::update_mml_string() {
	if (_head_event->get_next()->get_id() != MMLEvent::DEBUG_INFO) {
		return;
	}

	_mml_string = MMLParser::get_instance()->get_sequence_mml(_head_event->get_next());
	_head_event->set_length(0);
}

//

String MMLSequence::to_string() const {
	if (_is_terminal) {
		return "terminator";
	}

	MMLEvent *event = _head_event->get_next();
	String str;

	for (int i = 0; i < 32 && event; i++) {
		str += itos(event->get_id()) + " ";
		event = event->get_next();
	}

	return str;
}

List<MMLEvent *> MMLSequence::to_vector(int p_max_length, int p_offset, int p_event_id) {
	if (!_head_event) {
		return List<MMLEvent *>();
	}

	List<MMLEvent *> result;

	int i = 0;
	MMLEvent *event = _head_event->get_next();
	while (event && event->get_id() != MMLEvent::SEQUENCE_TAIL) {
		if (p_event_id == -1 || p_event_id == event->get_id()) {
			if (i >= p_offset) {
				result.push_back(event);
			}
			if (p_max_length > 0 && i >= p_max_length) {
				break;
			}

			i++;
		}

		event = event->get_next();
	}

	return result;
}

void MMLSequence::from_vector(List<MMLEvent *> p_events) {
	initialize();

	for (MMLEvent *event : p_events) {
		push_back(event);
	}
}

void MMLSequence::initialize() {
	if (!is_empty()) {
		MMLParser::get_instance()->free_all_events(this);
		_callbacks_for_internal_call.clear();
	}

	_head_event = MMLParser::get_instance()->alloc_event(MMLEvent::SEQUENCE_HEAD, 0);
	_tail_event = MMLParser::get_instance()->alloc_event(MMLEvent::SEQUENCE_TAIL, 0);
	_head_event->set_next(_tail_event);
	_head_event->set_jump(_head_event);
	_is_active = true;
}

void MMLSequence::clear() {
	if (_head_event) {
		MMLParser::get_instance()->free_all_events(this);

		_prev_sequence = nullptr;
		_next_sequence = nullptr;
	} else if (_is_terminal) {
		_prev_sequence = this;
		_next_sequence = this;
	}

	_mml_string = "";
}

MMLSequence::MMLSequence(bool p_terminal) {
	_prev_sequence = p_terminal ? this : nullptr;
	_next_sequence = p_terminal ? this : nullptr;
	_is_terminal = p_terminal;
}

MMLSequence::~MMLSequence() {
	if (_head_event) {
		MMLParser::get_instance()->free_all_events(this);
	}

	_prev_sequence = nullptr;
	_next_sequence = nullptr;
}

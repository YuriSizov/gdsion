/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_event.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/string.hpp>
#include "sequencer/base/mml_parser.h"

using namespace godot;

MMLEvent *MMLEvent::_nop_event = nullptr;

int MMLEvent::get_event_id(String p_mml) {
	if (p_mml == "c" || p_mml == "d" || p_mml == "e" || p_mml == "f" || p_mml == "g" || p_mml == "a" || p_mml == "b") {
		return MMLEvent::NOTE;
	}
	if (p_mml == "r") {
		return MMLEvent::REST;
	}
	if (p_mml == "q") {
		return MMLEvent::QUANT_RATIO;
	}
	if (p_mml == "@q") {
		return MMLEvent::QUANT_COUNT;
	}
	if (p_mml == "v") {
		return MMLEvent::VOLUME;
	}
	if (p_mml == "@v") {
		return MMLEvent::FINE_VOLUME;
	}
	if (p_mml == "%") {
		return MMLEvent::MOD_TYPE;
	}
	if (p_mml == "@") {
		return MMLEvent::MOD_PARAM;
	}
	if (p_mml == "@i") {
		return MMLEvent::INPUT_PIPE;
	}
	if (p_mml == "@o") {
		return MMLEvent::OUTPUT_PIPE;
	}
	if (p_mml == "(" || p_mml == ")") {
		return MMLEvent::VOLUME_SHIFT;
	}
	if (p_mml == "&") {
		return MMLEvent::SLUR;
	}
	if (p_mml == "&&") {
		return MMLEvent::SLUR_WEAK;
	}
	if (p_mml == "*") {
		return MMLEvent::PITCHBEND;
	}
	if (p_mml == ",") {
		return MMLEvent::PARAMETER;
	}
	if (p_mml == "$") {
		return MMLEvent::REPEAT_ALL;
	}
	if (p_mml == "[") {
		return MMLEvent::REPEAT_BEGIN;
	}
	if (p_mml == "]") {
		return MMLEvent::REPEAT_END;
	}
	if (p_mml == "|") {
		return MMLEvent::REPEAT_BREAK;
	}
	if (p_mml == "t") {
		return MMLEvent::TEMPO;
	}

	return 0;
}

MMLEvent *MMLEvent::get_parameters(Vector<int> *r_params, int p_length) const {
	MMLEvent *event = const_cast<MMLEvent *>(this);

	int i = 0;
	while (i < p_length) {
		r_params->write[i] = event->data;
		i++;

		if (event->next == nullptr || event->next->id != EventID::PARAMETER) {
			break;
		}

		event = event->next;
	}
	while (i < p_length) {
		r_params->write[i] = INT32_MIN;
		i++;
	}

	return event;
}

String MMLEvent::to_string() const {
	return "#" + itos(id) + "; " + itos(data);
}

void MMLEvent::initialize(int p_id, int p_data, int p_length) {
	id = p_id & 0x7f;
	data = p_data; // Prefer values below 0xffffff.
	length = p_length;

	next = nullptr;
	jump = nullptr;
}

void MMLEvent::free() {
	if (next == nullptr) {
		MMLParser::get_instance()->free_event(this);
	}
}

MMLEvent *MMLEvent::get_nop_event() {
	if (!_nop_event) {
		_nop_event = memnew(MMLEvent(0));
		_nop_event->initialize(EventID::NOP, 0, 0);
	}

	return _nop_event;
}

MMLEvent::MMLEvent(int p_id, int p_data, int p_length) {
	if (p_id > 1) {
		initialize(p_id, p_data, p_length);
	}
}

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

int MMLEvent::get_id_from_mml(String p_mml) {
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

// Helpers.

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

// Object management.

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

void MMLEvent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &MMLEvent::get_id);
	ClassDB::bind_method(D_METHOD("set_id", "value"), &MMLEvent::set_id);
	ClassDB::add_property("MMLEvent", PropertyInfo(Variant::INT, "id"), "set_id", "get_id");

	ClassDB::bind_method(D_METHOD("get_data"), &MMLEvent::get_data);
	ClassDB::bind_method(D_METHOD("set_data", "value"), &MMLEvent::set_data);
	ClassDB::add_property("MMLEvent", PropertyInfo(Variant::INT, "data"), "set_data", "get_data");

	ClassDB::bind_method(D_METHOD("get_length"), &MMLEvent::get_length);
	ClassDB::bind_method(D_METHOD("set_length", "value"), &MMLEvent::set_length);
	ClassDB::add_property("MMLEvent", PropertyInfo(Variant::INT, "length"), "set_length", "get_length");

	ClassDB::bind_method(D_METHOD("get_next"), &MMLEvent::get_next);
	ClassDB::bind_method(D_METHOD("set_next", "event"), &MMLEvent::set_next);
	ClassDB::add_property("MMLEvent", PropertyInfo(Variant::OBJECT, "next", PROPERTY_HINT_RESOURCE_TYPE, "MMLEvent"), "set_next", "get_next");

	ClassDB::bind_method(D_METHOD("get_jump"), &MMLEvent::get_jump);
	ClassDB::bind_method(D_METHOD("set_jump", "event"), &MMLEvent::set_jump);
	ClassDB::add_property("MMLEvent", PropertyInfo(Variant::OBJECT, "jump", PROPERTY_HINT_RESOURCE_TYPE, "MMLEvent"), "set_jump", "get_jump");

	BIND_ENUM_CONSTANT(NO_OP);
	BIND_ENUM_CONSTANT(PROCESS);
	BIND_ENUM_CONSTANT(REST);
	BIND_ENUM_CONSTANT(NOTE);
	BIND_ENUM_CONSTANT(KEY_ON_DELAY);
	BIND_ENUM_CONSTANT(QUANT_RATIO);
	BIND_ENUM_CONSTANT(QUANT_COUNT);
	BIND_ENUM_CONSTANT(VOLUME);
	BIND_ENUM_CONSTANT(VOLUME_SHIFT);
	BIND_ENUM_CONSTANT(FINE_VOLUME);
	BIND_ENUM_CONSTANT(SLUR);
	BIND_ENUM_CONSTANT(SLUR_WEAK);
	BIND_ENUM_CONSTANT(PITCHBEND);
	BIND_ENUM_CONSTANT(REPEAT_BEGIN);
	BIND_ENUM_CONSTANT(REPEAT_BREAK);
	BIND_ENUM_CONSTANT(REPEAT_END);
	BIND_ENUM_CONSTANT(MOD_TYPE);
	BIND_ENUM_CONSTANT(MOD_PARAM);
	BIND_ENUM_CONSTANT(INPUT_PIPE);
	BIND_ENUM_CONSTANT(OUTPUT_PIPE);
	BIND_ENUM_CONSTANT(REPEAT_ALL);
	BIND_ENUM_CONSTANT(PARAMETER);
	BIND_ENUM_CONSTANT(SEQUENCE_HEAD);
	BIND_ENUM_CONSTANT(SEQUENCE_TAIL);
	BIND_ENUM_CONSTANT(SYSTEM_EVENT);
	BIND_ENUM_CONSTANT(TABLE_EVENT);
	BIND_ENUM_CONSTANT(GLOBAL_WAIT);
	BIND_ENUM_CONSTANT(TEMPO);
	BIND_ENUM_CONSTANT(TIMER);
	BIND_ENUM_CONSTANT(REGISTER);
	BIND_ENUM_CONSTANT(DEBUG_INFO);
	BIND_ENUM_CONSTANT(INTERNAL_CALL);
	BIND_ENUM_CONSTANT(INTERNAL_WAIT);
	BIND_ENUM_CONSTANT(DRIVER_NOTE);
	BIND_ENUM_CONSTANT(USER_DEFINED);
	BIND_ENUM_CONSTANT(COMMAND_MAX);
}

MMLEvent::MMLEvent(int p_id, int p_data, int p_length) {
	if (p_id > 1) {
		initialize(p_id, p_data, p_length);
	}
}

MMLEvent::~MMLEvent() {
	next = nullptr;
	jump = nullptr;
}

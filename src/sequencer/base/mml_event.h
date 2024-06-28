/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_EVENT_H
#define MML_EVENT_H

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/vector.hpp>

using namespace godot;

class MMLEvent : public Object {
	GDCLASS(MMLEvent, Object)

public:
	enum EventID {
		// Default MML commands
		NO_OP         = 0,
		PROCESS       = 1,
		REST          = 2,
		NOTE          = 3,
		// The following 4 are not implemented in the original code.
		//LENGTH        = 4,
		//TEI           = 5,
		//OCTAVE        = 6,
		//OCTAVE_SHIFT  = 7,
		KEY_ON_DELAY  = 8,
		QUANT_RATIO   = 9,
		QUANT_COUNT   = 10,
		VOLUME        = 11,
		VOLUME_SHIFT  = 12,
		FINE_VOLUME   = 13,
		SLUR          = 14,
		SLUR_WEAK     = 15,
		PITCHBEND     = 16,
		REPEAT_BEGIN  = 17,
		REPEAT_BREAK  = 18,
		REPEAT_END    = 19,
		MOD_TYPE      = 20,
		MOD_PARAM     = 21,
		INPUT_PIPE    = 22,
		OUTPUT_PIPE   = 23,
		REPEAT_ALL    = 24,
		PARAMETER     = 25,
		SEQUENCE_HEAD = 26,
		SEQUENCE_TAIL = 27,
		SYSTEM_EVENT  = 28,
		TABLE_EVENT   = 29,
		GLOBAL_WAIT   = 30,
		TEMPO         = 31,
		TIMER         = 32,
		REGISTER      = 33,
		DEBUG_INFO    = 34,
		INTERNAL_CALL = 35,
		INTERNAL_WAIT = 36,
		DRIVER_NOTE   = 37,

		// First user defined command.
		USER_DEFINED   = 64,

		COMMAND_MAX   = 128
	};

private:
	int id = EventID::NO_OP;
	int data = 0;
	int length = 0;

	MMLEvent *next = nullptr;
	// Repeating event.
	MMLEvent *jump = nullptr;

protected:
	static void _bind_methods();

public:
	static int get_id_from_mml(String p_mml);

	int get_id() const { return id; }
	void set_id(int p_value) { id = p_value; }
	int get_data() const { return data; }
	void set_data(int p_value) { data = p_value; }
	int get_length() const { return length; }
	void set_length(int p_value) { length = p_value; }

	MMLEvent *get_next() const { return next; }
	void set_next(MMLEvent *p_event) { next = p_event; }
	MMLEvent *get_jump() const { return jump; }
	void set_jump(MMLEvent *p_event) { jump = p_event; }

	MMLEvent *get_parameters(Vector<int> *r_params, int p_length) const;
	String to_string() const;

	void initialize(int p_id, int p_data, int p_length);
	void free();

	MMLEvent(int p_id = 0, int p_data = 0, int p_length = 0);
	~MMLEvent();
};

VARIANT_ENUM_CAST(MMLEvent::EventID);

#endif // MML_EVENT_H

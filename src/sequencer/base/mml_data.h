/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_DATA_H
#define MML_DATA_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/list.hpp>
#include "sequencer/base/beats_per_minute.h"
#include "sequencer/base/mml_system_command.h"

using namespace godot;

class MMLEvent;
class MMLSequence;
class MMLSequenceGroup;

class MMLData : public RefCounted {
	GDCLASS(MMLData, RefCounted)

public:
	// Controls what tcommand argument is.
	enum TCommandMode {
		TCOMMAND_BPM = 0,    // BPM.
		TCOMMAND_TIMERB = 1, // OPNA's TIMERB with 48 ticks/beat.
		TCOMMAND_FRAME = 2,  // Frame count.
	};

private:
	MMLSequenceGroup *_sequence_group = nullptr;
	MMLSequence *_global_sequence = nullptr;

	String _title;
	String _author;

	int _default_fps = 60;
	TCommandMode _tcommand_mode = TCOMMAND_BPM;
	double _tcommand_resolution = 1;

	int _default_velocity_shift = 4;
	int _default_velocity_mode = 0;
	int _default_expression_mode = 0;

	Ref<BeatsPerMinute> _initial_bpm;
	// System commands that cannot be parsed by the system.
	List<Ref<MMLSystemCommand>> _system_commands;

protected:
	static void _bind_methods() {}

public:
	MMLSequenceGroup *get_sequence_group() const { return _sequence_group; }
	int get_sequence_count() const;
	int get_tick_count();
	bool has_repeat_all();

	MMLSequence *get_global_sequence() const { return _global_sequence; }

	String get_title() const { return _title; }
	void set_title(String p_title) { _title = p_title; }

	int get_default_fps() const { return _default_fps; }
	void set_default_fps(int p_value) { _default_fps = p_value; }

	void set_tcommand_mode(TCommandMode p_mode) { _tcommand_mode = p_mode; }
	void set_tcommand_resolution(double p_value) { _tcommand_resolution = p_value; }

	int get_default_velocity_shift() const { return _default_velocity_shift; }
	void set_default_velocity_shift(int p_value) { _default_velocity_shift = p_value; }
	int get_default_velocity_mode() const { return _default_velocity_mode; }
	void set_default_velocity_mode(int p_value) { _default_velocity_mode = p_value; }
	int get_default_expression_mode() const { return _default_expression_mode; }
	void set_default_expression_mode(int p_value) { _default_expression_mode = p_value; }

	double get_bpm() const;
	// Setting this to 0 makes data dependent on the driver's BPM.
	void set_bpm(double p_value);

	Ref<BeatsPerMinute> get_bpm_settings() const { return _initial_bpm; }
	void set_bpm_settings(const Ref<BeatsPerMinute> &p_settings) { _initial_bpm = p_settings; }

	double get_bpm_from_tcommand(int p_param);

	List<Ref<MMLSystemCommand>> get_system_commands() const { return _system_commands; }

	MMLSequence *append_new_sequence(List<MMLEvent *> p_events = List<MMLEvent *>());
	MMLSequence *get_sequence(int p_index);

	//

	virtual void clear();

	MMLData();
	~MMLData() {}
};

#endif // MML_DATA_H

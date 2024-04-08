/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_data.h"

#include <godot_cpp/core/memory.hpp>
#include "sequencer/base/beats_per_minute.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/base/mml_sequence_group.h"

using namespace godot;

int MMLData::get_sequence_count() const {
	return _sequence_group->get_sequence_count();
}

int MMLData::get_tick_count() {
	return _sequence_group->get_tick_count();
}

bool MMLData::has_repeat_all() {
	return _sequence_group->has_repeat_all();
}

double MMLData::get_bpm() const {
	return _initial_bpm.is_valid() ? _initial_bpm->get_bpm() : 0.0;
}

void MMLData::set_bpm(double p_value) {
	if (p_value > 0) {
		Ref<BeatsPerMinute> bpm = memnew(BeatsPerMinute(p_value, 44100));
		_initial_bpm = bpm;
	} else {
		Ref<BeatsPerMinute> bpm;
		bpm.instantiate();
		_initial_bpm = bpm;
	}
}

double MMLData::get_bpm_from_tcommand(int p_param) {
	switch(_tcommand_mode) {
		case TCOMMAND_BPM:
			return p_param * _tcommand_resolution;
		case TCOMMAND_FRAME:
			return p_param != 0 ? (_tcommand_resolution / p_param) : 120.0;
		case TCOMMAND_TIMERB:
			return (p_param >= 0 && p_param < 256) ? (_tcommand_resolution / (256 - p_param)) : 120.0;
	}

	return 0.0;
}

MMLSequence *MMLData::append_new_sequence(List<MMLEvent *> p_events) {
	MMLSequence *sequence = _sequence_group->append_new_sequence();
	if (sequence) {
		sequence->from_vector(p_events);
	}

	return sequence;
}

MMLSequence *MMLData::get_sequence(int p_index) {
	return _sequence_group->get_sequence(p_index);
}

//

void MMLData::clear() {
	_sequence_group->free();
	_global_sequence->free();

	_title = "";
	_author = "";

	_default_fps = 60;
	_tcommand_mode = TCOMMAND_BPM;
	_tcommand_resolution = 1;

	_default_velocity_mode = 0;
	_default_expression_mode = 0;

	Ref<BeatsPerMinute> bpm;
	bpm.instantiate();
	_initial_bpm = bpm;
	_system_commands.clear();

	// Reset.
	_global_sequence->initialize();
}

MMLData::MMLData() {
	_sequence_group = memnew(MMLSequenceGroup(this));
	_global_sequence = memnew(MMLSequence);
}

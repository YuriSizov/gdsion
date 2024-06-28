/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_SEQUENCE_GROUP_H
#define MML_SEQUENCE_GROUP_H

#include <godot_cpp/templates/list.hpp>
#include "sequencer/base/mml_data.h"

using namespace godot;

class MMLEvent;
class MMLSequence;

// Group of MMLSequences. MMLData > MMLSequenceGroup > MMLSequence > MMLEvent (">" means "has a").
class MMLSequenceGroup {

	static List<MMLSequence *> _free_list;

	// Owner data.
	Ref<MMLData> _owner;

	// Terminator.
	List<MMLSequence *> _sequences;
	MMLSequence *_term = nullptr;

public:
	int get_sequence_count() const;
	MMLSequence *get_head_sequence() const;

	// Get the song length by tick count (1920 for wholetone).
	int get_tick_count();
	bool has_repeat_all();

	MMLSequence *get_new_sequence();
	MMLSequence *append_new_sequence();
	MMLSequence *get_sequence(int p_index) const;

	void alloc(MMLEvent *p_head_event);
	void free();

	MMLSequenceGroup(const Ref<MMLData> &p_owner);
	~MMLSequenceGroup();
};

#endif // MML_SEQUENCE_GROUP_H

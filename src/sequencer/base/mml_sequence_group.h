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

	List<MMLSequence *> _free_list;

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
	MMLEvent *populate_sequences(MMLEvent *p_head_event);
	MMLSequence *get_sequence(int p_index) const;

	void clear();

	MMLSequenceGroup();
	~MMLSequenceGroup();
};

#endif // MML_SEQUENCE_GROUP_H

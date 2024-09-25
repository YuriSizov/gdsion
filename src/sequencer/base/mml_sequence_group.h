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
class MMLSequenceGroup : public Object {
	GDCLASS(MMLSequenceGroup, Object)

	List<MMLSequence *> _free_list;

	// Terminator.
	List<MMLSequence *> _sequences;
	MMLSequence *_term = nullptr;

protected:
	static void _bind_methods();

public:
	MMLSequence *create_new_sequence();
	MMLSequence *append_new_sequence();
	MMLEvent *populate_sequences(MMLEvent *p_head_event);

	MMLSequence *get_head_sequence() const;
	MMLSequence *get_sequence(int p_index) const;
	int get_sequence_count() const;

	//

	int get_tick_count();
	bool has_repeat_all();

	//

	void clear();

	MMLSequenceGroup();
	~MMLSequenceGroup();
};

#endif // MML_SEQUENCE_GROUP_H

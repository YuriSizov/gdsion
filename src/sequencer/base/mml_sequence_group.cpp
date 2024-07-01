/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_sequence_group.h"

#include <godot_cpp/core/memory.hpp>
#include "sequencer/base/mml_data.h"
#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_sequence.h"

using namespace godot;

int MMLSequenceGroup::get_sequence_count() const {
	return _sequences.size();
}

MMLSequence *MMLSequenceGroup::get_head_sequence() const {
	return _term->get_next_sequence();
}

/** Get song length by tick count (1920 for wholetone). */
int MMLSequenceGroup::get_tick_count() {
	int tick_count = 0;

	for (MMLSequence *sequence : _sequences) {
		int length = sequence->get_mml_length();
		if (length > tick_count) {
			tick_count = length;
		}
	}

	return tick_count;
}

bool MMLSequenceGroup::has_repeat_all() {
	for (MMLSequence *sequence : _sequences) {
		if (sequence->has_repeat_all()) {
			return true;
		}
	}
	return false;
}

//

MMLSequence *MMLSequenceGroup::get_sequence(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _sequences.size(), nullptr);

	return _sequences[p_index];
}

MMLSequence *MMLSequenceGroup::get_new_sequence() {
	MMLSequence *sequence = nullptr;
	if (!_free_list.is_empty()) {
		sequence = _free_list.front()->get();
		_free_list.pop_front();
	} else {
		sequence = memnew(MMLSequence);
	}

	_sequences.push_back(sequence);
	return sequence;
}

MMLSequence *MMLSequenceGroup::append_new_sequence() {
	MMLSequence *sequence = get_new_sequence();
	sequence->insert_before(_term);
	sequence->set_active(false);
	return sequence;
}

MMLEvent *MMLSequenceGroup::populate_sequences(MMLEvent *p_head_event) {
	MMLEvent *event = p_head_event;
	while (event && event->get_jump()) {
		ERR_FAIL_COND_V_MSG(event->get_id() != MMLEvent::SEQUENCE_HEAD, event, vformat("MMLSequenceGroup: Invalid event in the head event sequence (%s).", event->to_string()));

		MMLSequence *sequence = append_new_sequence();
		event = sequence->cutout(event);
		sequence->update_mml_string();
		sequence->set_active(true);
	}

	if (event) {
		// This can happen normally, as we always add an extra head after finishing previous sequence. But anything else
		// is a problem with data or a bug.
		ERR_FAIL_COND_V_MSG(event->get_id() != MMLEvent::SEQUENCE_HEAD, event, vformat("MMLSequenceGroup: Invalid events at the end of the sequence (starting with %s).", event->to_string()));
		ERR_FAIL_COND_V_MSG(event->get_next(), event, vformat("MMLSequenceGroup: Invalid events at the end of the sequence (starting with %s).", event->get_next()->to_string()));
	}

	// Return the remainder, if any, so the caller can decide what to do with it.
	return event;
}

//

void MMLSequenceGroup::clear() {
	for (MMLSequence *sequence : _sequences) {
		sequence->clear();
		_free_list.push_back(sequence);
	}
	_sequences.clear();
	_term->clear();
}

MMLSequenceGroup::MMLSequenceGroup() {
	_term = memnew(MMLSequence(true));
}

MMLSequenceGroup::~MMLSequenceGroup() {
	for (MMLSequence *sequence : _sequences) {
		memdelete(sequence);
	}
	_sequences.clear();

	for (MMLSequence *sequence : _free_list) {
		memdelete(sequence);
	}
	_free_list.clear();

	memdelete(_term);
}

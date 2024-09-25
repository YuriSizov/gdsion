/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_ENVELOPE_TABLE_H
#define SIMML_ENVELOPE_TABLE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include "templates/singly_linked_list.h"

using namespace godot;

class SiMMLEnvelopeTable : public RefCounted {
	GDCLASS(SiMMLEnvelopeTable, RefCounted)

	void _initialize(SinglyLinkedList<int> *p_head, SinglyLinkedList<int> *p_tail);

protected:
	static void _bind_methods() {}

public:
	SinglyLinkedList<int> *head = nullptr;
	SinglyLinkedList<int> *tail = nullptr;

	void parse_mml(String p_table_numbers, String p_postfix, int p_max_index = 65536);
	// NOTE: Original code can implicitly create the destination vector and return it. We require creating it ahead of the call.
	void to_vector(int p_length, Vector<int> *r_destination, int p_min = -65536, int p_max = 65536);

	void copy_from(const Ref<SiMMLEnvelopeTable> &p_source);
	void free();

	SiMMLEnvelopeTable(Vector<int> p_table = Vector<int>(), int p_loop_point = -1);
};

#endif // SIMML_ENVELOPE_TABLE_H

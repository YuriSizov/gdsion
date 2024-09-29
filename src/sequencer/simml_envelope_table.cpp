/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_envelope_table.h"

#include "utils/translator_util.h"

void SiMMLEnvelopeTable::parse_mml(String p_table_numbers, String p_postfix, int p_max_index) {
	TranslatorUtil::MMLTableNumbers result = TranslatorUtil::parse_table_numbers(p_table_numbers, p_postfix, p_max_index);
	if (result.head) {
		_initialize(result.head, result.tail);
	}
}

void SiMMLEnvelopeTable::to_vector(int p_length, Vector<int> *r_destination, int p_min, int p_max) {
	r_destination->resize_zeroed(p_length);

	SinglyLinkedList<int> *current = head;
	for (int i = 0; i < p_length; i++) {
		int value = 0;
		if (current) {
			value = current->get()->value;
			current = current->next();
		}

		CLAMP(value, p_min, p_max);
		r_destination->write[i] = value;
	}
}

void SiMMLEnvelopeTable::copy_from(const Ref<SiMMLEnvelopeTable> &p_source) {
	free();

	if (!p_source->head) {
		return;
	}

	head = p_source->head->clone_element();

	SinglyLinkedList<int> *target = head;
	SinglyLinkedList<int> *current = p_source->head->next();

	while (current != p_source->tail) {
		target = target->append(current->get()->value);
		current = current->next();
	}
}

void SiMMLEnvelopeTable::_initialize(SinglyLinkedList<int> *p_head, SinglyLinkedList<int> *p_tail) {
	head = p_head;
	tail = p_tail;

	// Looping last data.
	if (tail->next() == nullptr) {
		tail->link(tail);
	}
}

void SiMMLEnvelopeTable::free() {
	if (head) {
		tail->unlink();
		SinglyLinkedList<int>::free_list(head);

		head = nullptr;
		tail = nullptr;
	}
}

SiMMLEnvelopeTable::SiMMLEnvelopeTable(Vector<int> p_table, int p_loop_point) {
	if (p_table.is_empty()) {
		head = nullptr;
		tail = nullptr;
		return;
	}

	head = SinglyLinkedList<int>::alloc_list(p_table.size());
	tail = head;

	SinglyLinkedList<int> *loop = nullptr;
	int i = 0;
	for (; i < p_table.size() - 1; i++) {
		if (p_loop_point == i) {
			loop = tail;
		}

		tail->get()->value = p_table[i];
		tail = tail->next();
	}

	tail->get()->value = p_table[i];
	tail->link(loop);
}

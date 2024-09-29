/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_SLL_INT_H
#define SION_SLL_INT_H

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/list.hpp>

using namespace godot;

// A singly-linked list implementation (used for ints and doubles).
// TODO: Consider replacing with one of the native Godot/GDExtension types.
// FIXME: This implementation assumes we would never keep a pointer to any number in the list, other than the first one.
// This is because when we "free" elements of this singly-linked list, whatever number references this element will be left with a bad memory pointer.
// This is perhaps expected with an SLL, but makes code relying on this type dangerous.
// It may be better to move to a different data structure, unless there is a really good reason not to.
template <class T>
class SinglyLinkedList {

	// Reusable to reduce the number of allocations.
	// FIXME: There is no mechanism to free this memory at any point. Original code probably relies on some automatic memory freeing.
	// In other words, this is going to leak on exit, unless memory is properly managed.
	static SinglyLinkedList<T> *_free_list;

public:
	class Element {
	public:
		T value = 0;
	};

private:
	Element *_element = nullptr;

	SinglyLinkedList<T> *_next = nullptr;

public:
	static SinglyLinkedList<T> *alloc(T p_value) {
		SinglyLinkedList<T> *ret;

		if (_free_list) {
			ret = _free_list;
			_free_list = _free_list->next();

			ret->get()->value = p_value;
			ret->unlink();
		} else {
			ret = memnew(SinglyLinkedList<T>(p_value));
		}

		return ret;
	}

	static SinglyLinkedList<T> *alloc_list(int p_size, T p_default_value = 0, bool p_ring = false) {
		SinglyLinkedList<T> *head = alloc(p_default_value);

		SinglyLinkedList<T> *elem = head;
		for (int i = 1; i < p_size; i++) {
			elem = elem->append(p_default_value);
		}

		if (p_ring) {
			elem->link(head); // Loop makes the ring.
		}

		return head;
	}

	static void free(SinglyLinkedList<T> *p_element) {
		// If this element is part of a list, you must be careful here, as this method does NOT
		// handle the list itself gracefully and you may leave bad pointers unattended as a result.

		// Append the current allocated free list and then assume its position.
		p_element->link(_free_list);
		_free_list = p_element;
	}

	static void free_list(SinglyLinkedList<T> *p_head) {
		if (!p_head) {
			return;
		}

		// For a ring list, the argument can be any element. In a non-ring, it MUST be the first
		// element in the list, otherwise we leave bad pointers unattended.
		SinglyLinkedList<T> *tail = p_head->last();

		// Append the current allocated free list and then assume its position.
		tail->link(_free_list);
		_free_list = p_head;
	}

	//

	SinglyLinkedList<T> *next() const {
		return _next;
	}

	SinglyLinkedList<T> *last() const {
		SinglyLinkedList<T> *first = const_cast<SinglyLinkedList<T> *>(this);
		SinglyLinkedList<T> *elem = first;

		while (elem->next() && elem->next() != first) {
			elem = elem->next();
		}

		return elem;
	}

	SinglyLinkedList<T> *append(T p_value) {
		SinglyLinkedList<T> *tail = alloc(p_value);
		link(tail);
		return next();
	}

	SinglyLinkedList<T> *prepend(T p_value) {
		SinglyLinkedList<T> *head = alloc(p_value);
		head->link(this);
		return head;
	}

	void link(SinglyLinkedList<T> *p_element) {
		_next = p_element;
	}

	void unlink() {
		_next = nullptr;
	}

	//

	Element *get() const {
		return _element;
	}

	void reset() {
		SinglyLinkedList<T> *first = this;
		SinglyLinkedList<T> *elem = first;

		while (elem->next() && elem->next() != first) {
			elem->get()->value = 0;
			elem = elem->next();
		}
		elem->get()->value = 0;
	}

	//

	SinglyLinkedList<T> *index(int p_index) const {
		SinglyLinkedList<T> *element = const_cast<SinglyLinkedList<T> *>(this);

		// This isn't very efficient, but should be fast enough for now.
		for (int i = 0; i < p_index; i++) {
			element = element->next();
		}

		return element;
	}

	SinglyLinkedList<T> *clone_element() {
		return alloc(_element->value);
	}

	SinglyLinkedList(T p_value) {
		_element = memnew(Element);
		_element->value = p_value;
	}

	~SinglyLinkedList() {
		if (_element) {
			memdelete(_element);
			_element = nullptr;
		}
	}
};

template <class T>
SinglyLinkedList<T> *SinglyLinkedList<T>::_free_list = nullptr;

#endif // SION_SLL_INT_H

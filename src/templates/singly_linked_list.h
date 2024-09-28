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

	SinglyLinkedList<T> *_next = nullptr;

public:
	// NOTE: Original code called it `i`. There is no special accessor, so we opt for a better public name.
	T value = 0;

	static SinglyLinkedList<T> *alloc(int p_value = 0) {
		SinglyLinkedList<T> *ret;

		if (_free_list) {
			ret = _free_list;
			_free_list = _free_list->_next;

			ret->value = p_value;
			ret->_next = nullptr;
		} else {
			ret = memnew(SinglyLinkedList<T>(p_value));
		}

		return ret;
	}

	static SinglyLinkedList<T> *alloc_list(int p_size, int p_default_value = 0) {
		SinglyLinkedList<T> *ret = alloc(p_default_value);

		SinglyLinkedList<T> *elem = ret;
		for (int i = 1; i < p_size; i++) {
			elem = elem->append(p_default_value);
		}

		return ret;
	}

	static SinglyLinkedList<T> *alloc_ring(int p_size, int p_default_value = 0) {
		SinglyLinkedList<T> *ret = alloc(p_default_value);

		SinglyLinkedList<T> *elem = ret;
		for (int i = 1; i < p_size; i++) {
			elem = elem->append(p_default_value);
		}
		elem->_next = ret;

		return ret;
	}

	static void free(SinglyLinkedList<T> *p_first_element) {
		// Append the current allocated free list and then assume its position.
		// The argument MUST be the first element in any list, otherwise we leave bad pointers.
		p_first_element->_next = _free_list;
		_free_list = p_first_element;
	}

	static void free_list(SinglyLinkedList<T> *p_first_element) {
		if (!p_first_element) {
			return;
		}

		// The argument MUST be the first element in any list, otherwise we leave bad pointers.
		SinglyLinkedList<T> *elem = p_first_element;
		while (elem->_next) {
			elem = elem->_next;
		}

		// Append the current allocated free list and then assume its position.
		elem->_next = _free_list;
		_free_list = p_first_element;
	}

	static void free_ring(SinglyLinkedList<T> *p_first_element) {
		if (!p_first_element) {
			return;
		}

		// The argument MUST be the first element in any list, otherwise we leave bad pointers.
		SinglyLinkedList<T> *elem = p_first_element;
		while (elem->_next != p_first_element) {
			elem = elem->_next;
		}

		// Append the current allocated free list and then assume its position.
		elem->_next = _free_list;
		_free_list = p_first_element;
	}

	// FIXME: Original code supports creating a vector of a fixed size, but we don't have such data structure. May require code changes.
	static List<SinglyLinkedList<T> *> create_ring_pager(SinglyLinkedList<T> *p_first_element) {
		if (!p_first_element) {
			return List<SinglyLinkedList<T> *>();
		}

		List<SinglyLinkedList<T> *> pager;
		pager.push_back(p_first_element);

		SinglyLinkedList<T> *elem = p_first_element->_next;
		while (elem != p_first_element) {
			pager.push_back(elem);
			elem = elem->_next;
		}

		return pager;
	}

	//

	SinglyLinkedList<T> *next() const {
		return _next;
	}

	SinglyLinkedList<T> *append(T p_value) {
		_next = alloc(p_value);
		return _next;
	}

	void link(SinglyLinkedList<T> *p_element) {
		_next = p_element;
	}

	void unlink() {
		_next = nullptr;
	}

	SinglyLinkedList(T p_value) {
		value = p_value;
	}
};

template <class T>
SinglyLinkedList<T> *SinglyLinkedList<T>::_free_list = nullptr;

#endif // SION_SLL_INT_H

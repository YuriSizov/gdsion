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
template <class T>
class SinglyLinkedList {

	// Reusable to reduce the number of allocations.
	static SinglyLinkedList<T> *_element_pool;

public:
	class Element {
		friend class SinglyLinkedList<T>;

		Element *_next_ptr = nullptr;

	public:
		T value = 0;

		// Returns the next linked element. Can be safely used without moving the cursor (e.g. for checks).
		Element *next() const {
			return _next_ptr;
		}
	};

private:
	Element *_first = nullptr;
	Element *_last = nullptr;
	Element *_current = nullptr;
	int _size = 0;

	// Creates or reuses an instance of Element and returns it with the new value assigned.
	Element *_alloc_element(T p_value) {
		Element *ret;

		if (_element_pool && _element_pool->has_any()) {
			ret = _element_pool->pop_front_element();
		} else {
			ret = memnew(Element);
		}

		ret->value = p_value;

		return ret;
	}

	// Unlinks the element from the next and adds it to the element pool. Does NOT handle existing references to this element!
	void _release_element(Element *p_element) {
		p_element->_next_ptr = nullptr;

		if (_element_pool) {
			_element_pool->push_back_element(p_element);
		}
	}

public:

	// Initializes the static element pool for this templated type.
	static void initialize_pool() {
		_element_pool = memnew(SinglyLinkedList<T>);
	}

	// Frees the static element pool for this templated type.
	static void finalize_pool() {
		if (!_element_pool) {
			return;
		}

		while (_element_pool->has_any()) {
			Element *elem = _element_pool->pop_front_element();
			memdelete(elem);
		}

		memdelete(_element_pool);
	}

	//

	int size() const {
		return _size;
	}

	bool has_any() const {
		return _size > 0;
	}

	bool is_empty() const {
		return _size == 0;
	}

	// Resets the cursor to the first element and returns it.
	Element *front() {
		if (_size == 0) {
			return nullptr;
		}

		_current = _first;
		return _current;
	}

	// Resets the cursor to the last element and returns it.
	Element *back() {
		if (_size == 0) {
			return nullptr;
		}

		_current = _last;
		return _current;
	}

	// Moves the cursor to the next element and returns it.
	Element *next() {
		if (_size == 0) {
			return nullptr;
		}
		if (!_current) {
			return nullptr;
		}

		_current = _current->_next_ptr;
		return _current;
	}

	// Moves the cursor to the next element and returns it. Hard stops on the last element, avoiding loops.
	Element *next_safe() {
		if (_size == 0) {
			return nullptr;
		}

		if (_current == _last) {
			return nullptr;
		}

		_current = _current->_next_ptr;
		return _current;
	}

	// Moves the cursor by the given distance and returns the current element. Follows loops; can end early
	// if the distance is beyond the length of the list.
	Element *advance(int p_distance) {
		if (_size == 0) {
			return nullptr;
		}
		if (!_current) {
			return nullptr;
		}

		for (int i = 0; i < p_distance && _current; i++) {
			_current = _current->_next_ptr;
		}
		return _current;
	}

	//

	// Returns the current element.
	Element *get() const {
		if (_size == 0) {
			return nullptr;
		}

		return _current;
	}

	// Sets the cursor to the given element in the list. There is no check whether the element belongs to
	// this list, use with care.
	void set(Element *p_element) {
		if (_size == 0) {
			return; // This is just wrong...
		}

		// FIXME: There is currently no quick check to make sure it belongs to this list.
		_current = p_element;
	}

	// Returns the first element.
	Element *get_front() const {
		if (_size == 0) {
			return nullptr;
		}

		return _first;
	}

	// Returns the last element.
	Element *get_back() const {
		if (_size == 0) {
			return nullptr;
		}

		return _last;
	}

	Element *get_index(int p_index) const {
		if (_size == 0 || p_index >= _size) {
			return nullptr;
		}

		Element *elem = _first;
		for (int i = 0; i < p_index; i++) {
			elem = elem->next();
		}

		return elem;
	}

	// Adds a new value to the end of the list and returns the element.
	Element *append(T p_value) {
		Element *elem = _alloc_element(p_value);

		push_back_element(elem);
		_current = _last;

		return elem;
	}

	// Adds a new value to the start of the list and returns the element.
	Element *prepend(T p_value) {
		Element *elem = _alloc_element(p_value);

		push_front_element(elem);
		_current = _first;

		return elem;
	}

	// Removes the first element from the list. This is the only element that can be safely removed in SLL.
	void remove_front() {
		Element *elem = pop_front_element();
		_release_element(elem);
	}

	//

	// Adds an existing element to the end of the list. Does NOT handle existing references to this element!
	void push_back_element(Element *p_element) {
		if (_size == 0) {
			_first = p_element;
			_last = p_element;
		} else {
			_last->_next_ptr = p_element;
			_last = _last->_next_ptr;
		}

		_size += 1;
	}

	// Adds an existing element to the start of the list. Does NOT handle existing references to this element!
	void push_front_element(Element *p_element) {
		if (_size == 0) {
			_first = p_element;
			_last = p_element;
		} else {
			p_element->_next_ptr = _first;
			_first = p_element;
		}

		_size += 1;
	}

	// Removes the first element from the list and returns it. The cursor is updated if it points to this element.
	Element *pop_front_element() {
		if (_size == 0) {
			return nullptr;
		}

		Element *elem = _first;
		_size -= 1;

		if (_size == 0) {
			_first = nullptr;
			_last = nullptr;
			_current = nullptr;
		} else {
			_first = elem->_next_ptr;

			if (_current == elem) {
				_current = _first;
			}
		}

		// Unlink.
		elem->_next_ptr = nullptr;

		return elem;
	}

	void loop(Element *p_element = nullptr) {
		if (_size == 0) {
			return;
		}

		Element *loop_point = p_element;
		if (!loop_point) {
			loop_point = _first;
		}

		_last->_next_ptr = loop_point;
	}

	//

	// Resets all values in the list to zero.
	void reset() {
		for (Element *elem = front(); elem; elem = next_safe()) {
			elem->value = 0;
		}
	}

	// Removes all elements from the list and resets its size.
	void clear() {
		for (Element *elem = front(); elem; elem = next_safe()) {
			_release_element(elem);
		}

		_first = nullptr;
		_last = nullptr;
		_current = nullptr;
		_size = 0;
	}

	SinglyLinkedList() {
	}

	SinglyLinkedList(int p_size, T p_default_value = 0, bool p_ring = false) {
		if (p_size <= 0) {
			return;
		}

		for (int i = 0; i < p_size; i++) {
			append(p_default_value);
		}

		if (p_ring) { // Loop makes the ring.
			_last->_next_ptr = _first;
		}

		front();
	}

	~SinglyLinkedList() {
		clear();
	}
};

template <class T>
SinglyLinkedList<T> *SinglyLinkedList<T>::_element_pool = nullptr;

#endif // SION_SLL_INT_H

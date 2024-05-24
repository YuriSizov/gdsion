/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_GODOT_UTIL_H
#define SION_GODOT_UTIL_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

PackedStringArray split_string_by_regex(const String &p_string, const String &p_regex);

template <class T, size_t S>
Vector<T> make_vector(T (&p_array)[S]) {
	Vector<T> vector;
	vector.resize_zeroed(S);

	for (int i = 0; i < S; i++) {
		vector.write[i] = p_array[i];
	}

	return vector;
}

// Ideally TypedArrays should be natively convertable with Vectors, but it's not implemented.

template <class T>
Vector<T> make_vector_from_typed_array(const TypedArray<T> &p_array) {
	Vector<T> data;

	for (int i = 0; i < p_array.size(); i++) {
		data.append(p_array[i]);
	}

	return data;
}

template <class T>
TypedArray<T> make_typed_array_from_vector(const Vector<T> &p_data) {
	TypedArray<T> array;

	for (const T &item : p_data) {
		array.push_back(item);
	}

	return array;
}

#endif // SION_GODOT_UTIL_H

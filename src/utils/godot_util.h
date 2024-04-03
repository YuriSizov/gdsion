/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_GODOT_UTIL_H
#define SION_GODOT_UTIL_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

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

#endif // SION_TRANSLATOR_H

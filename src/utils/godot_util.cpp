/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "godot_util.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>

PackedStringArray split_string_by_regex(const String &p_string, const String &p_regex) {
	// This is boilerplate to split a string by a regex in Godot.
	PackedStringArray arr;

	Ref<RegEx> re_split = RegEx::create_from_string(p_regex);
	TypedArray<RegExMatch> matches = re_split->search_all(p_string);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> match = matches[i];
		arr.append(match->get_string());
	}

	return arr;
}

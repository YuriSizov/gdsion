/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_parser_settings.h"

int MMLParserSettings::get_default_length() const {
	return resolution / default_l_value;
}

void MMLParserSettings::set_default_octave(int p_value) {
	_default_octave = p_value;
	_mml_to_note_number = 60 - _default_octave * 12;

	int octave_limit = (int)((128 - _mml_to_note_number) / 12) - 1;
	if (max_octave > octave_limit) {
		max_octave = octave_limit;
	}
}

MMLParserSettings::MMLParserSettings() {
	set_default_octave(5);
}

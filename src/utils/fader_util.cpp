/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "fader_util.h"

#include <godot_cpp/variant/variant.hpp>

bool FaderUtil::is_active() const {
	return _counter > 0;
}

bool FaderUtil::is_incrementing() const {
	return _step > 0;
}

bool FaderUtil::execute() {
	if (_counter > 0) {
		_value += _step;
		_counter -= 1;

		if (_counter == 0) {
			_value = _end; // Ensure there is no imprecision.
			if (_callback.is_valid()) {
				_callback.call(_value);
			}

			return true;
		} else {
			if (_callback.is_valid()) {
				_callback.call(_value);
			}
		}
	}

	return false;
}

void FaderUtil::stop() {
	_counter = 0;
}

void FaderUtil::set_fade(double p_value_from, double p_value_to, int p_frames) {
	_value = p_value_from;

	if (p_frames == 0 || !_callback.is_valid()) {
		_counter = 0;
		return;
	}

	_end = p_value_to;
	_step = (p_value_to - p_value_from) / p_frames;
	_counter = p_frames;
	_callback.call(_value);
}

FaderUtil::FaderUtil(const Callable &p_callback, double p_value_from, double p_value_to, int p_frames) {
	set_callback(p_callback);
	set_fade(p_value_from, p_value_to, p_frames);
}

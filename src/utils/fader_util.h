/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_FADER_UTIL_H
#define SION_FADER_UTIL_H

#include <godot_cpp/variant/callable.hpp>

using namespace godot;

class FaderUtil {

	double _end = 0;
	double _step = 0;
	int _counter = 0;
	double _value = 0;

	Callable _callback;

public:
	bool is_active() const;
	bool is_incrementing() const;
	double get_value() const { return _value; }
	void set_callback(const Callable &p_callback) { _callback = p_callback; }

	// Return true if the end value has been reached.
	bool execute();
	void stop();

	void set_fade(double p_value_from = 0, double p_value_to = 1, int p_frames = 60);

	FaderUtil(const Callable &p_callback = Callable(), double p_value_from = 0, double p_value_to = 1, int p_frames = 60);
	~FaderUtil() {}
};

#endif // SION_FADER_UTIL_H

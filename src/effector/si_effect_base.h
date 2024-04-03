/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_BASE_H
#define SI_EFFECT_BASE_H

#include <godot_cpp/templates/vector.hpp>

using namespace godot;

// Base class for all effects. Doesn't implement any behavior by default, extending
// classes must be used.
class SiEffectBase {

	bool _is_free = true;

public:
	bool is_free() const { return _is_free; }
	void set_free(bool p_free) { _is_free = p_free; }

	// Returns the requested channel count.
	virtual int prepare_process() { return 1; }

	// Process the effect to the stream buffer.
	// When called as mono, same data is expected in buffer[i*2] and buffer[i*2+1]. I.e. the buffer
	// is always in stereo. The order in the buffer is the same as wave format ([L0,R0,L1,R1,L2,R2 ... ]).
	// Start index and length must be adjusted internally to account for the stereo nature of the buffer.
	// Returns the output channel count.
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) { return p_channels; }

	virtual void set_by_mml(Vector<double> p_args) {}

	virtual void initialize() {}

	SiEffectBase() {}
};

#endif // SI_EFFECT_BASE_H

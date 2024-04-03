/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "beats_per_minute.h"

#include <godot_cpp/core/math.hpp>

#include "sequencer/base/mml_sequencer.h"

using namespace godot;

bool BeatsPerMinute::update(double p_bpm, int p_sample_rate) {
	double bpm = CLAMP(p_bpm, 1, 511);

	if (bpm == _bpm && p_sample_rate == _sample_rate) {
		return false;
	}

	_bpm = bpm;
	_sample_rate = p_sample_rate;

	_tick_per_sample = _resolution * _bpm / (_sample_rate * 240);
	_beat_16th_per_sample = _bpm / (_sample_rate * 15); // 60 / 4
	_sample_per_beat_16th = 1.0 / _beat_16th_per_sample;
	_sample_per_tick = (1.0 / _tick_per_sample) * (1 << MMLSequencer::FIXED_BITS);

	return true;
}

BeatsPerMinute::BeatsPerMinute(double p_bpm, int p_sample_rate, int p_resolution) {
	_resolution = p_resolution;
	update(p_bpm, p_sample_rate);
}

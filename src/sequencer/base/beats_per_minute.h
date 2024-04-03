/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef BEATS_PER_MINUTE_H
#define BEATS_PER_MINUTE_H

// Abstraction to calculate BPM-releated numbers automatically.
class BeatsPerMinute {

	double _bpm = 0;
	int _sample_rate = 0;
	int _resolution = 0;

	double _tick_per_sample = 0;
	// Sample per tick in fixed unit.
	double _sample_per_tick = 0;
	// 16th beat per sample
	double _beat_16th_per_sample = 0;
	// Sample per 16th beat
	double _sample_per_beat_16th = 0;

public:
	double get_bpm() const { return _bpm; }
	int get_sample_rate() const { return _sample_rate; }

	double get_tick_per_sample() const { return _tick_per_sample; }
	double get_sample_per_tick() const { return _sample_per_tick; }
	double get_beat_16th_per_sample() const { return _beat_16th_per_sample; }
	double get_sample_per_beat_16th() const { return _sample_per_beat_16th; }

	bool update(double p_bpm, int p_sample_rate);

	BeatsPerMinute(double p_bpm, int p_sample_rate, int p_resolution = 1920);
	~BeatsPerMinute() {}
};

#endif // BEATS_PER_MINUTE_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_SAMPLER_DATA_H
#define SIOPM_WAVE_SAMPLER_DATA_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
#include "chip/wave/siopm_wave_base.h"

using namespace godot;

class SiOPMWaveSamplerData : public SiOPMWaveBase {
	GDCLASS(SiOPMWaveSamplerData, SiOPMWaveBase)

	Vector<double> _wave_data;
	int _channel_count = 0;
	int _pan = 0;
	// This flag is only available for non-loop samples.
	bool _ignore_note_off = false;

	void _prepare_wave_data(const Variant &p_data, int p_src_channel_count, int p_channel_count);

	//

	// Wave positions in the sample count.
	int _start_point = 0;
	int _end_point = 0;
	int _loop_point = -1; // -1 means no looping.

	// Seek head and end gaps in the sample.
	int _seek_head_silence();
	int _seek_end_gap();
	void _slice();

protected:
	static void _bind_methods() {};

public:
	Vector<double> get_wave_data() const { return _wave_data; }
	int get_channel_count() const { return _channel_count; }
	int get_pan() const { return _pan; }
	int get_length() const;

	bool is_ignoring_note_off() const { return _ignore_note_off; }
	void set_ignore_note_off(bool p_ignore);

	//

	int get_start_point() const { return _start_point; }
	int get_end_point() const { return _end_point; }
	int get_loop_point() const { return _loop_point; }
	int get_initial_sample_index(double p_phase = 0) const;

	void slice(int p_start_point = -1, int p_end_point = -1, int p_loop_point = -1);

	//

	SiOPMWaveSamplerData(const Variant &p_data = Variant(), bool p_ignore_note_off = false, int p_pan = 0, int p_src_channel_count = 2, int p_channel_count = 0);
	~SiOPMWaveSamplerData() {}
};

#endif // SIOPM_WAVE_SAMPLER_DATA_H

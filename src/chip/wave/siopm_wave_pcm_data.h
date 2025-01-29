/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_PCM_DATA_H
#define SIOPM_WAVE_PCM_DATA_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
#include "chip/wave/siopm_wave_base.h"

using namespace godot;

class SiOPMWavePCMData : public SiOPMWaveBase {
	GDCLASS(SiOPMWavePCMData, SiOPMWaveBase)

	static Vector<double> _sin_table;

	Vector<int> _wavelet;
	int _channel_count = 0;
	int _sampling_pitch = 0;

	void _prepare_wavelet(const Variant &p_data, int p_src_channel_count, int p_channel_count);

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
	static void _bind_methods() {}

public:
	Vector<int> get_wavelet() const { return _wavelet; }
	int get_channel_count() const { return _channel_count; }
	int get_sampling_pitch() const { return _sampling_pitch; }

	int get_sample_count() const;
	int get_sampling_octave() const;

	int get_start_point() const { return _start_point; }
	int get_end_point() const { return _end_point; }
	int get_loop_point() const { return _loop_point; }
	int get_initial_sample_index(double p_phase = 0) const;

	void slice(int p_start_point = -1, int p_end_point = -1, int p_loop_point = -1);
	void loop_tail_samples(int p_sample_count = 2205, int p_tail_margin = 0, bool p_crossfade = true);

	SiOPMWavePCMData(const Variant &p_data = Variant(), int p_sampling_pitch = 4416, int p_src_channel_count = 2, int p_channel_count = 0);
	~SiOPMWavePCMData() {}
};

#endif // SIOPM_WAVE_PCM_DATA_H

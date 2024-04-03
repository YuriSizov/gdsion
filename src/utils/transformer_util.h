/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_TRANSFORMER_UTIL_H
#define SION_TRANSFORMER_UTIL_H

#include <godot_cpp/templates/vector.hpp>

using namespace godot;

// Based partially on the original code's SiONUtil class.
class TransformerUtil {

	static void _amplify_log_data(Vector<int> *r_src, int p_gain);

public:
	// Logarithmic transformation of wave PCM data.
	static Vector<int> transform_pcm_data(Vector<double> p_source, int p_src_channel_count = 2, int p_channel_count = 0, bool p_maximize = true);

	static Vector<double> transform_sampler_data(Vector<double> p_source, int p_src_channel_count = 2, int p_channel_count = 0);

	static Vector<double> wave_color_to_vector(uint32_t p_color, int p_wave_type = 0);
};

#endif // SION_TRANSFORMER_UTIL_H

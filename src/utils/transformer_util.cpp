/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "transformer_util.h"

#include "processor/siopm_table.h"
#include "processor/wave/siopm_wave_table.h"

void TransformerUtil::_amplify_log_data(Vector<int> *r_src, int p_gain) {
	int gain = p_gain & ~1;

	for (int i = 0; i < r_src->size(); i++) {
		r_src->write[i] -= gain;
	}
}

Vector<int> TransformerUtil::transform_pcm_data(Vector<double> p_source, int p_src_channel_count, int p_channel_count, bool p_maximize) {
	Vector<int> result;
	int max_gain = SiOPMTable::LOG_TABLE_BOTTOM;

	if (p_src_channel_count == p_channel_count || p_channel_count == 0) {
		int target_size = p_source.size();
		result.resize_zeroed(target_size);

		for (int i = 0; i < target_size; i++) {
			result.write[i] = SiOPMTable::calculate_log_table_index(p_source[i]);

			if (result[i] < max_gain) {
				max_gain = result[i];
			}
		}

	} else if (p_src_channel_count == 2) { // p_channel_count == 1
		int target_size = p_source.size() >> 1;
		result.resize_zeroed(target_size);

		int j = 0;
		for (int i = 0; i < target_size; i++) {
			double value = p_source[j];
			j++;
			value += p_source[j];
			j++;

			result.write[i] = SiOPMTable::calculate_log_table_index(value * 0.5);

			if (result[i] < max_gain) {
				max_gain = result[i];
			}
		}

	} else { // p_src_channel_count == 1, p_channel_count == 2
		int target_size = p_source.size();
		result.resize_zeroed(target_size << 1);

		int j = 0;
		for (int i = 0; i < target_size; i++) {
			result.write[j] = SiOPMTable::calculate_log_table_index(p_source[i]);
			result.write[j + 1] = result[j];
			j += 2;

			if (result[j] < max_gain) {
				max_gain = result[j];
			}
		}
	}

	if (p_maximize && max_gain > 1) {
		_amplify_log_data(&result, max_gain);
	}
	return result;
}

Vector<double> TransformerUtil::transform_sampler_data(Vector<double> p_source, int p_src_channel_count, int p_channel_count) {
	Vector<double> result;

	if (p_src_channel_count == p_channel_count || p_channel_count == 0) {
		return p_source;

	} else if (p_src_channel_count == 2) { // p_channel_count == 1
		int target_size = p_source.size() >> 1;
		result.resize_zeroed(target_size);

		for (int i = 0, j = 0; i < target_size; i++, j += 2) {
			result.write[i] = (p_source[j] + p_source[j + 1]) * 0.5;
		}

	} else { // p_src_channel_count == 1, p_channel_count == 2
		int target_size = p_source.size();
		result.resize_zeroed(target_size << 1);

		for (int i = 0, j = 0; i < target_size; i++, j += 2) {
			result.write[j] = p_source[i];
			result.write[j + 1] = p_source[i];
		}

	}

	return result;
}

Vector<double> TransformerUtil::wave_color_to_vector(uint32_t p_color, int p_wave_type) {
	Vector<double> result;

	int bits = 0;
	for (int length = SiOPMTable::SAMPLING_TABLE_SIZE >> 1; length != 0; length >>= 1) {
		bits++;
	}
	result.resize_zeroed(1 << bits);

	double bars[7];
	uint32_t color = p_color;
	for (int i = 0; i < 7; i++) {
		bars[i] = (color & 15) * 0.0625;
		color >>= 4;
	}

	int barr[7] = { 1,2,3,4,5,6,8 };
	int (&log_table)[SiOPMTable::LOG_TABLE_SIZE * 3] = SiOPMTable::get_instance()->log_table;
	SiOPMWaveTable *wave_table = SiOPMTable::get_instance()->get_wave_table(p_wave_type + (color >> 28));
	int envelope_top = (-SiOPMTable::ENV_TOP) << 3;

	double value_max = 0;

	bits = SiOPMTable::PHASE_BITS - bits;
	int step = SiOPMTable::PHASE_MAX >> bits;
	for (int i = 0; i < SiOPMTable::PHASE_MAX; i += step) {
		int j = i >> bits;

		result.write[j] = 0;
		for (int mult_idx = 0; mult_idx < 7; mult_idx++) {
			int gain_idx = (((i * barr[mult_idx]) & SiOPMTable::PHASE_FILTER) >> wave_table->get_fixed_bits());
			int gain = wave_table->get_wavelet()[gain_idx] + envelope_top;

			result.write[j] += log_table[gain] * bars[mult_idx];
		}

		double value = ABS(result[j]);
		if (value_max < value) {
			value_max = value;
		}
	}

	value_max = MAX(value_max, 8192);
	double value_coef = 1.0 / value_max;
	for (int i = 0; i < result.size(); i++) {
		result.write[i] *= value_coef;
	}

	return result;
}

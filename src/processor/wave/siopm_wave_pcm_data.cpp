/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_pcm_data.h"

#include <godot_cpp/core/math.hpp>
#include "processor/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"
#include "utils/transformer_util.h"

using namespace godot;

Vector<double> SiOPMWavePCMData::_sin_table;

int SiOPMWavePCMData::get_sample_count() const {
	if (_wavelet.is_empty()) {
		return 0;
	}

	return _wavelet.size() >> (_channel_count - 1);
}

int SiOPMWavePCMData::get_sampling_octave() const {
	return (int)(_sampling_pitch * 0.001272264631043257);
}

int SiOPMWavePCMData::get_initial_sample_index(double p_phase) const {
	return (int)(_start_point * (1 - p_phase) + _end_point * p_phase);
}

//

int SiOPMWavePCMData::_seek_head_silence() {
	int threshold = SiOPMRefTable::LOG_TABLE_BOTTOM - SiOPMRefTable::LOG_TABLE_RESOLUTION * 14; // 1/128

	int i = 0;
	for (; i < _wavelet.size(); i++) {
		if (_wavelet[i] < threshold) {
			break;
		}
	}

	return i >> (_channel_count - 1);
}

int SiOPMWavePCMData::_seek_end_gap() {
	int threshold = SiOPMRefTable::LOG_TABLE_BOTTOM - SiOPMRefTable::LOG_TABLE_RESOLUTION * 2; // 1/4096

	int i = _wavelet.size() - 1;
	for (; i > 0; i--) {
		if (_wavelet[i] < threshold) {
			break;
		}
	}

	return (i >> (_channel_count - 1)) - 100; // 100 = 1 cycle margin
}

void SiOPMWavePCMData::_slice() {
	if (_start_point < 0) {
		_start_point = _seek_head_silence();
	}

	if (_loop_point < -1) {
		// Set to loop infinitely.
		if (_end_point >= 0) {
			loop_tail_samples(-_loop_point);
			if (_start_point >= _end_point) {
				_end_point = _start_point;
			}
		} else {
			loop_tail_samples(-_loop_point, -_end_point);
		}
	} else {
		int wavelet_length = get_sample_count();
		if (_end_point < 0) {
			_end_point = _seek_end_gap() + _end_point;
		} else if (_end_point < _start_point) {
			_end_point = _start_point;
		} else if (_end_point > wavelet_length) {
			_end_point = wavelet_length - 1;
		}

		if (_loop_point != -1 && _loop_point < _start_point) {
			_loop_point = _start_point;
		} else if (_loop_point > _end_point) {
			_loop_point = -1;
		}
	}
}

void SiOPMWavePCMData::slice(int p_start_point, int p_end_point, int p_loop_point) {
	_start_point = p_start_point;
	_end_point = p_end_point;
	_loop_point = p_loop_point;

	_slice();
}

void SiOPMWavePCMData::loop_tail_samples(int p_sample_count, int p_tail_margin, bool p_crossfade) {
	_end_point = _seek_end_gap() - p_tail_margin;

	if (_end_point < (_start_point + p_sample_count)) {
		if (_end_point < _start_point) {
			_end_point = _start_point;
		}
		_loop_point = _start_point;
		return;
	}

	_loop_point = _end_point - p_sample_count;

	if (p_crossfade && _loop_point > (_start_point + p_sample_count)) {
		int max_idx = p_sample_count << (_channel_count - 1);
		double delta_sin = 1.5707963267948965 / max_idx;

		if (_sin_table.size() != max_idx) {
			_sin_table.resize_zeroed(max_idx);

			double sin_value = 0;
			for (int i = 0; i < max_idx; i++) {
				_sin_table.write[i] = Math::sin(sin_value);
				sin_value += delta_sin;
			}
		}

		int offset = _loop_point << (_channel_count - 1);
		int envelope_top = (-SiOPMRefTable::ENV_TOP) << 3;
		int (&log_table)[SiOPMRefTable::LOG_TABLE_SIZE * 3] = SiOPMRefTable::get_instance()->log_table;
		double i2n = 1.0 / (1 << SiOPMRefTable::LOG_VOLUME_BITS);

		for (int i = 0; i < max_idx; i++) {
			int idx0 = offset + i;
			int idx1 = idx0 - max_idx;
			int val0 = _wavelet[idx0] + envelope_top;
			int val1 = _wavelet[idx1] + envelope_top;

			int j = max_idx - 1 - i;
			_wavelet.write[idx0] = SiOPMRefTable::calculate_log_table_index((log_table[val0] * _sin_table[j] + log_table[val1] * _sin_table[i]) * i2n);
		}
	}
}

//

SiOPMWavePCMData::SiOPMWavePCMData(const Variant &p_data, int p_sampling_pitch, int p_src_channel_count, int p_channel_count) :
		SiOPMWaveBase(SiMMLRefTable::MT_PCM) {
	if (!p_data) {
		return;
	}

	int source_channels = (p_src_channel_count == 1 ? 1 : 2);
	int target_channels = (p_channel_count == 0 ? source_channels : p_channel_count);
	_channel_count = (target_channels == 1 ? 1 : 2);

	Variant::Type data_type = p_data.get_type();
	switch (data_type) {
		case Variant::PACKED_INT32_ARRAY: {
			// TODO: If someday Vector<T> and Packed*Arrays become friends, this can be simplified.
			for (int value : (PackedInt32Array)p_data) {
				_wavelet.append(value);
			}
		} break;

		case Variant::PACKED_FLOAT32_ARRAY: {
			// TODO: If someday Vector<T> and Packed*Arrays become friends, this can be simplified.
			Vector<double> raw_data;
			for (double value : (PackedFloat32Array)p_data) {
				raw_data.append(value);
			}
			_wavelet = TransformerUtil::transform_pcm_data(raw_data, source_channels, _channel_count);
		} break;

		case Variant::NIL: {
			_wavelet.clear();
		} break;

		default: {
			ERR_FAIL_MSG("SiOPMWavePCMData: Unsupported data type.");
		} break;
	}

	_sampling_pitch = p_sampling_pitch;
	_end_point = get_sample_count() - 1;
}

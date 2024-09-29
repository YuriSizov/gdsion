/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_sampler_data.h"

#include <godot_cpp/core/error_macros.hpp>
#include "sion_enums.h"
#include "templates/singly_linked_list.h"
#include "utils/transformer_util.h"

using namespace godot;

int SiOPMWaveSamplerData::get_length() const {
	if (_is_extracted) {
		return _wave_data.size() >> (_channel_count - 1);
	}

	return 0;
}

void SiOPMWaveSamplerData::set_ignore_note_off(bool p_ignore) {
	_ignore_note_off = (_loop_point == -1) && p_ignore;
}

//

int SiOPMWaveSamplerData::get_initial_sample_index(double p_phase) const {
	return (int)(_start_point * (1 - p_phase) + _end_point * p_phase);
}

int SiOPMWaveSamplerData::_seek_head_silence() {
	if (_wave_data.is_empty()) {
		return 0;
	}

	SinglyLinkedList<double> *ms_window = SinglyLinkedList<double>::alloc_list(22, 0.0, true); // 0.5ms
	int i = 0;

	if (_channel_count == 1) {
		double ms = 0;

		for (; i < _wave_data.size(); i++) {
			ms -= ms_window->get()->value;

			ms_window = ms_window->next();
			ms_window->get()->value = _wave_data[i] * _wave_data[i];
			ms += ms_window->get()->value;

			if (ms > 0.0011) {
				break;
			}
		}

	} else {
		double ms = 0;

		for (; i < _wave_data.size();) {
			ms -= ms_window->get()->value;

			// SUS: For the mono version we would break the loop before the final increment.
			// Here we would increment and then break. This is inconsistent and needs to be validated.
			// Keeping as in the original implementation for now.

			ms_window = ms_window->next();
			ms_window->get()->value = _wave_data[i] * _wave_data[i];
			i++;
			ms_window->get()->value += _wave_data[i] * _wave_data[i];
			i++;
			ms += ms_window->get()->value;

			if (ms > 0.0022) {
				break;
			}
		}

		// To add to above, maybe this is why there is this step?
		i >>= 1;
	}


	SinglyLinkedList<double>::free_list(ms_window);
	return i - 22;
}

int SiOPMWaveSamplerData::_seek_end_gap() {
	if (_wave_data.is_empty()) {
		return 0;
	}

	int i = _wave_data.size() - 1;

	if (_channel_count == 1) {
		for (; i >= 0; i--) {
			double ms = _wave_data[i] * _wave_data[i];

			if (ms > 0.0001) {
				break;
			}
		}

	} else {
		// SUS: See notes in _seek_head_silence, they apply here as well.

		for (; i >= 0;) {
			double ms = _wave_data[i] * _wave_data[i];
			i--;
			ms += _wave_data[i] * _wave_data[i];
			i--;

			if (ms > 0.0002) {
				break;
			}
		}

		i >>= 1;
	}

	return MAX(i, (get_length() - 1152));
}

void SiOPMWaveSamplerData::_slice() {
	if (_start_point < 0) {
		_start_point = _seek_head_silence();
	}
	if (_loop_point < 0) {
		_loop_point = -1;
	}
	if (_end_point < 0) {
		_end_point = _seek_end_gap();
	}

	if (_end_point < _loop_point) {
		_loop_point = -1;
	}
	if (_end_point < _start_point) {
		_end_point = get_length() - 1;
	}
}

void SiOPMWaveSamplerData::slice(int p_start_point, int p_end_point, int p_loop_point) {
	_start_point = p_start_point;
	_end_point = p_end_point;
	_loop_point = p_loop_point;

	_slice();
}

//

SiOPMWaveSamplerData::SiOPMWaveSamplerData(const Variant &p_data, int p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count) :
		SiOPMWaveBase(SiONModuleType::MODULE_SAMPLE) {
	if (!p_data) {
		return;
	}

	int source_channels = (p_src_channel_count == 1 ? 1 : 2);
	int target_channels = (p_channel_count == 0 ? source_channels : p_channel_count);
	_channel_count = (target_channels == 1 ? 1 : 2);

	Variant::Type data_type = p_data.get_type();
	switch (data_type) {
		case Variant::PACKED_FLOAT32_ARRAY: {
			// TODO: If someday Vector<T> and Packed*Arrays become friends, this can be simplified.
			Vector<double> raw_data;
			for (double value : (PackedFloat32Array)p_data) {
				raw_data.append(value);
			}
			_wave_data = TransformerUtil::transform_sampler_data(raw_data, source_channels, _channel_count);
			_is_extracted = true;
		} break;

		case Variant::NIL: {
			_wave_data.clear();
			_is_extracted = false;
		} break;

		default: {
			ERR_FAIL_MSG("SiOPMWaveSamplerData: Unsupported data type.");
		} break;
	}

	_end_point = get_length();
	set_ignore_note_off(p_ignore_note_off);
	_pan = p_pan;
}

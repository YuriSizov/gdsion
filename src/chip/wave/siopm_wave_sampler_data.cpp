/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_sampler_data.h"

#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/audio_stream.hpp>

#include "sion_enums.h"
#include "templates/singly_linked_list.h"
#include "utils/transformer_util.h"

using namespace godot;

void SiOPMWaveSamplerData::_prepare_wave_data(const Variant &p_data, int p_src_channel_count, int p_channel_count) {
	int source_channels = CLAMP(p_src_channel_count, 1, 2);
	int target_channels = (p_channel_count == 0 ? source_channels : CLAMP(p_channel_count, 1, 2));

	Variant::Type data_type = p_data.get_type();
	switch (data_type) {
		case Variant::PACKED_FLOAT32_ARRAY: {
			// TODO: If someday Vector<T> and Packed*Arrays become friends, this can be simplified.
			Vector<double> raw_data;
			for (double value : (PackedFloat32Array)p_data) {
				raw_data.append(value);
			}

			_wave_data = TransformerUtil::transform_sampler_data(raw_data, source_channels, target_channels);
		} break;

		case Variant::OBJECT: {
			Ref<AudioStream> audio_stream = p_data;
			if (audio_stream.is_valid()) {
				Vector<double> raw_data = _extract_wave_data(audio_stream, &source_channels);
				if (p_channel_count == 0) { // Update if necessary.
					target_channels = source_channels;
				}

				_wave_data = TransformerUtil::transform_sampler_data(raw_data, source_channels, target_channels);
				break;
			}

			ERR_FAIL_MSG("SiOPMWaveSamplerData: Unsupported data type.");
		} break;

		case Variant::NIL: {
			// Nothing to do.
		} break;

		default: {
			ERR_FAIL_MSG("SiOPMWaveSamplerData: Unsupported data type.");
		} break;
	}

	_channel_count = target_channels;
	_end_point = get_length();
}

int SiOPMWaveSamplerData::get_length() const {
	if (_channel_count > 0) {
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

	// Note: Original code is pretty much broken here. The intent seems to be to keep track of the
	// last 22 sample points and check if their sum goes over a threshold. However, the order of
	// calls was wrong, and we ended up adding and immediately removing the value from the sum, and
	// constantly overriding our ring buffer without reusing its values.
	// This method has been adjusted to fix the code according to the assumed intent. But it's not
	// tested, and I can't say if the original idea behind the code is wrong somehow.

	SinglyLinkedList<double> *ms_window = memnew(SinglyLinkedList<double>(22, 0.0, true)); // 0.5ms
	int i = 0;

	if (_channel_count == 1) {
		double ms = 0.0;

		for (; i < _wave_data.size(); i++) {
			ms -= ms_window->get()->value;
			ms_window->get()->value = _wave_data[i] * _wave_data[i];
			ms += ms_window->get()->value;

			ms_window->next();

			if (ms > 0.0011) {
				break;
			}
		}

	} else {
		double ms = 0.0;

		for (; i < _wave_data.size(); i += 2) {
			ms -= ms_window->get()->value;
			ms_window->get()->value  = _wave_data[i]     * _wave_data[i];
			ms_window->get()->value += _wave_data[i + 1] * _wave_data[i + 1];
			ms += ms_window->get()->value;

			ms_window->next();

			if (ms > 0.0022) {
				break;
			}
		}

		i >>= 1;
	}

	memdelete(ms_window);
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
		for (; i >= 0; i -= 2) {
			double ms = _wave_data[i]     * _wave_data[i];
			ms       += _wave_data[i - 1] * _wave_data[i - 1];

			if (ms > 0.0002) {
				break;
			}
		}

		i >>= 1;
	}

	// SUS: What is 1152? Should be extracted into a clearly named constant.
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

SiOPMWaveSamplerData::SiOPMWaveSamplerData(const Variant &p_data, bool p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count) :
		SiOPMWaveBase(SiONModuleType::MODULE_SAMPLE) {

	_prepare_wave_data(p_data, p_src_channel_count, p_channel_count);
	set_ignore_note_off(p_ignore_note_off);
	_pan = p_pan;
}

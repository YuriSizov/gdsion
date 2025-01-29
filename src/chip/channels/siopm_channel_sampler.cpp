/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_sampler.h"

#include "sion_enums.h"
#include "chip/siopm_channel_params.h"
#include "chip/siopm_sound_chip.h"
#include "chip/siopm_stream.h"
#include "chip/wave/siopm_wave_base.h"
#include "chip/wave/siopm_wave_sampler_data.h"
#include "chip/wave/siopm_wave_sampler_table.h"

void SiOPMChannelSampler::get_channel_params(const Ref<SiOPMChannelParams> &p_params) const {
	for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		p_params->set_master_volume(i, _volumes[i]);
	}
	p_params->set_pan(_pan);
}

void SiOPMChannelSampler::set_channel_params(const Ref<SiOPMChannelParams> &p_params, bool p_with_volume, bool p_with_modulation) {
	if (p_params->get_operator_count() == 0) {
		return;
	}

	if (p_with_volume) {
		for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
			_volumes.write[i] = p_params->get_master_volume(i);
		}

		_has_effect_send = false;
		for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
			if (_volumes[i] > 0) {
				_has_effect_send = true;
				break;
			}
		}

		_pan = p_params->get_pan();
	}
}

void SiOPMChannelSampler::set_wave_data(const Ref<SiOPMWaveBase> &p_wave_data) {
	_sampler_table = p_wave_data;
	_sample_data = p_wave_data;
}

void SiOPMChannelSampler::set_types(int p_pg_type, SiONPitchTableType p_pt_type) {
	_bank_number = p_pg_type & 3;
}

int SiOPMChannelSampler::get_pitch() const {
	return _wave_number << 6;
}

void SiOPMChannelSampler::set_pitch(int p_value) {
	_wave_number = p_value >> 6;
}

void SiOPMChannelSampler::set_phase(int p_value) {
	_sample_start_phase = p_value;
}

// Volume control.

void SiOPMChannelSampler::offset_volume(int p_expression, int p_velocity) {
	_expression = p_expression * p_velocity * 0.00006103515625; // 1/16384
}

// Processing.

void SiOPMChannelSampler::note_on() {
	if (_wave_number < 0) {
		return;
	}

	if (_sampler_table.is_valid()) {
		_sample_data = _sampler_table->get_sample(_wave_number & 127);
	}
	if (_sample_data.is_valid() && _sample_start_phase != 255) {
		_sample_index = _sample_data->get_initial_sample_index(_sample_start_phase * 0.00390625); // 1/256
		_sample_pan = CLAMP(_pan + _sample_data->get_pan(), 0, 128);
	}

	_is_idling = (_sample_data == nullptr);
	_is_note_on = !_is_idling;
}

void SiOPMChannelSampler::note_off() {
	if (_sample_data.is_null() || _sample_data->is_ignoring_note_off()) {
		return;
	}

	_is_note_on = false;
	_is_idling = true;

	if (_sampler_table.is_valid()) {
		_sample_data = Ref<SiOPMWaveSamplerData>();
	}
}

void SiOPMChannelSampler::buffer(int p_length) {
	if (_is_idling || _sample_data == nullptr || !_sample_data->is_extracted() || _mute) {
		buffer_no_process(p_length);
		return;
	}

	// Stream extracted data.
	int residue = p_length;
	while (residue > 0) {
		int remaining = _sample_data->get_end_point() - _sample_index;
		int processed = MIN(residue, remaining);

		if (_has_effect_send) {
			for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
				if (_volumes[i] > 0) {
					SiOPMStream *stream = _streams[i] ? _streams[i] : _sound_chip->get_stream_slot(i);
					if (stream) {
						double volume = _volumes[i] * _expression * _sound_chip->get_sampler_volume();
						Vector<double> wave_data = _sample_data->get_wave_data();
						stream->write_from_vector(&wave_data, _sample_index, _buffer_index, processed, volume, _sample_pan, _sample_data->get_channel_count());
					}
				}
			}
		} else {
			SiOPMStream *stream = _streams[0] ? _streams[0] : _sound_chip->get_output_stream();

			double volume = _volumes[0] * _expression * _sound_chip->get_sampler_volume();
			Vector<double> wave_data = _sample_data->get_wave_data();
			stream->write_from_vector(&wave_data, _sample_index, _buffer_index, processed, volume, _sample_pan, _sample_data->get_channel_count());
		}

		_sample_index += processed;
		residue -= processed;

		// If processed length is not enough, try to loop; and if not, stop streaming.
		if (residue > 0) {
			if (_sample_data->get_loop_point() >= 0) {
				if (_sample_data->get_loop_point() > _sample_data->get_start_point()) {
					_sample_index = _sample_data->get_loop_point();
				} else {
					_sample_index = _sample_data->get_start_point();
				}
			} else {
				_is_idling = true;
				if (_sampler_table.is_valid()) {
					_sample_data = Ref<SiOPMWaveSamplerData>();
				}
				break;
			}
		}
	}

	_buffer_index += p_length;
}

void SiOPMChannelSampler::buffer_no_process(int p_length) {
	_buffer_index += p_length;
}

//

void SiOPMChannelSampler::initialize(SiOPMChannelBase *p_prev, int p_buffer_index) {
	SiOPMChannelBase::initialize(p_prev, p_buffer_index);
	reset();
}

void SiOPMChannelSampler::reset() {
	_is_note_on = false;
	_is_idling = true;

	_bank_number = 0;
	_wave_number = -1;
	_expression = 1;

	_sampler_table = _table->sampler_tables[0];
	_sample_data = Ref<SiOPMWaveSamplerData>();

	_sample_start_phase = 0;
	_sample_index = 0;
	_sample_pan = 0;
}

String SiOPMChannelSampler::_to_string() const {
	String params = "";

	params += "vol=" + rtos(_volumes[0] * _expression) + ", ";
	params += "pan=" + itos(_pan - 64) + "";

	return "SiOPMChannelSampler: " + params;
}

SiOPMChannelSampler::SiOPMChannelSampler(SiOPMSoundChip *p_chip) : SiOPMChannelBase(p_chip) {
	// Empty.
}

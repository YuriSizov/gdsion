/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_ks.h"

#include "sion_enums.h"
#include "chip/channels/siopm_operator.h"
#include "chip/siopm_sound_chip.h"
#include "chip/siopm_stream.h"
#include "chip/wave/siopm_wave_pcm_table.h"
#include "chip/wave/siopm_wave_table.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_voice.h"

void SiOPMChannelKS::set_karplus_strong_params(int p_attack_rate, int p_decay_rate, int p_total_level, int p_fixed_pitch, int p_wave_shape, int p_tension) {
	int wave_shape = p_wave_shape;
	if (wave_shape == -1) {
		wave_shape = SiONPulseGeneratorType::PULSE_NOISE_PINK;
	}

	_ks_seed_type = KS_SEED_DEFAULT;

	set_algorithm(1, 0);
	set_feedback(0, 0);
	set_params_by_value(p_attack_rate, p_decay_rate, 0, 63, 15, p_total_level, 0, 0, 1, 0, 0, 0, 0, p_fixed_pitch);

	_active_operator->set_pulse_generator_type(p_wave_shape);
	Ref<SiOPMWaveTable> wave_table = _table->get_wave_table(_active_operator->get_pulse_generator_type());
	_active_operator->set_pitch_table_type(wave_table->get_default_pitch_table_type());

	set_all_release_rate(p_tension);
}

void SiOPMChannelKS::set_parameters(Vector<int> p_params) {
	_ks_seed_type = (p_params[0] == INT32_MIN ? KS_SEED_DEFAULT : (KSSeedType)p_params[0]);
	_ks_seed_index = (p_params[1] == INT32_MIN ? 0 : p_params[1]);

	switch (_ks_seed_type) {
		case KS_SEED_FM: {
			ERR_FAIL_INDEX(_ks_seed_index, SiMMLRefTable::VOICE_MAX);

			Ref<SiMMLVoice> voice = SiMMLRefTable::get_instance()->get_voice(_ks_seed_index);
			if (voice.is_valid()) {
				set_channel_params(voice->get_channel_params(), false);
			}
		} break;

		case KS_SEED_PCM: {
			ERR_FAIL_INDEX(_ks_seed_index, SiOPMRefTable::PCM_DATA_MAX);

			Ref<SiOPMWavePCMTable> pcm_table = _table->get_pcm_data(_ks_seed_index);
			if (pcm_table.is_valid()) {
				set_wave_data(pcm_table);
			}
		} break;

		default: {
			_ks_seed_type = KS_SEED_DEFAULT;
			set_params_by_value(p_params[1], p_params[2], 0, 63, 15, p_params[3], 0, 0, 1, 0, 0, 0, 0, p_params[4]);

			_active_operator->set_pulse_generator_type(p_params[5] == INT32_MIN ? SiONPulseGeneratorType::PULSE_NOISE_PINK : p_params[5]);
			Ref<SiOPMWaveTable> wave_table = _table->get_wave_table(_active_operator->get_pulse_generator_type());
			_active_operator->set_pitch_table_type(wave_table->get_default_pitch_table_type());
		} break;
	}
}

void SiOPMChannelKS::set_types(int p_pg_type, SiONPitchTableType p_pt_type) {
	_ks_seed_type = (KSSeedType)p_pg_type;
	_ks_seed_index = 0;
}

void SiOPMChannelKS::set_all_attack_rate(int p_value) {
	_operators[0]->set_attack_rate(p_value);
	_operators[0]->set_decay_rate(p_value > 48 ? 48 : p_value);
	_operators[0]->set_total_level(p_value > 48 ? 0 : (48 - p_value));
}

void SiOPMChannelKS::set_all_release_rate(int p_value) {
	_ks_decay_lpf = 1 - p_value * 0.015625; // 1/64
}

void SiOPMChannelKS::set_release_rate(int p_value) {
	_ks_decay_lpf = 1 - p_value * 0.015625; // 1/64
}

void SiOPMChannelKS::set_fixed_pitch(int p_value) {
	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->set_fixed_pitch_index(i);
	}
}

// Volume control.

void SiOPMChannelKS::offset_volume(int p_expression, int p_velocity) {
	_expression = p_expression * 0.0078125;
	SiOPMChannelFM::offset_volume(128, p_velocity);
}

// LFO control.

void SiOPMChannelKS::_set_lfo_state(bool p_enabled) {
	_lfo_on = 0;
}

// Processing.

void SiOPMChannelKS::note_on() {
	_output = 0;

	for (int i = 0; i < KS_BUFFER_SIZE; i++) {
		_ks_delay_buffer.write[i] *= 0.3;
	}

	_decay_lpf = _ks_decay_lpf;
	_decay     = _ks_decay;

	SiOPMChannelFM::note_on();
}

void SiOPMChannelKS::note_off() {
	_decay_lpf = _ks_mute_decay_lpf;
	_decay     = _ks_mute_decay;
}

void SiOPMChannelKS::reset_channel_buffer_status() {
	_buffer_index = 0;
	_is_idling = false;
}

void SiOPMChannelKS::_apply_karplus_strong(SinglyLinkedList<int> *p_target, int p_length) {
	SinglyLinkedList<int> *target = p_target;
	const int pitch_idx_max = SiOPMRefTable::PITCH_TABLE_SIZE - 1;

	int pitch_idx = _ks_pitch_index + _operators[0]->get_detune() + _pitch_modulation_output_level;
	pitch_idx = CLAMP(pitch_idx, 0, pitch_idx_max);
	double wave_length_max = _table->pitch_wave_length[pitch_idx];

	for (int i = 0; i < p_length; i++) {
		// Update LFO.
		_lfo_timer -= _lfo_timer_step;
		if (_lfo_timer < 0) {
			_lfo_phase = (_lfo_phase + 1) & 255;

			int value_base = _lfo_wave_table[_lfo_phase];
			_pitch_modulation_output_level = (((value_base << 1) - 255) * _pitch_modulation_depth) >> 8;

			pitch_idx = _ks_pitch_index + _operators[0]->get_detune() + _pitch_modulation_output_level;
			pitch_idx = CLAMP(pitch_idx, 0, pitch_idx_max);
			wave_length_max = _table->pitch_wave_length[pitch_idx];

			_lfo_timer += _lfo_timer_initial;
		}

		// Update KS.
		_ks_delay_buffer_index++;
		if (_ks_delay_buffer_index >= wave_length_max) {
			_ks_delay_buffer_index = Math::fmod(_ks_delay_buffer_index, wave_length_max);
		}
		int buffer_index = (int)_ks_delay_buffer_index;

		_output *= _decay;
		_output += (_ks_delay_buffer[buffer_index] - _output) * _decay_lpf + target->value;

		_ks_delay_buffer.write[buffer_index] = _output;
		target->value = (int)_output;
		target = target->next;
	}
}

// This methods is, for the most part, a carbon copy of SiOPMChannelBase::buffer. Perhaps there is a way to
// reduce code duplication here and make the differences pluggable.
void SiOPMChannelKS::buffer(int p_length) {
	if (_is_idling) {
		buffer_no_process(p_length);
		return;
	}

	// Preserve the start of the output pipe.
	SinglyLinkedList<int> *mono_out = _out_pipe;
	// Update the output pipe for the provided length.
	if (_process_function.is_valid()) {
		_process_function.call(p_length);
	}

	if (_ring_pipe) {
		_apply_ring_modulation(mono_out, p_length);
	}

	_apply_karplus_strong(mono_out, p_length);

	if (_filter_on) {
		_apply_sv_filter(mono_out, p_length, _filter_variables);
	}

	if (_output_mode == OutputMode::OUTPUT_STANDARD && !_mute) {
		if (_has_effect_send) {
			for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
				if (_volumes[i] > 0) {
					SiOPMStream *stream = _streams[i] ? _streams[i] : _sound_chip->get_stream_slot(i);
					if (stream) {
						stream->write(mono_out, _buffer_index, p_length, _volumes[i] * _expression, _pan);
					}
				}
			}
		} else {
			SiOPMStream *stream = _streams[0] ? _streams[0] : _sound_chip->get_output_stream();
			stream->write(mono_out, _buffer_index, p_length, _volumes[0] * _expression, _pan);
		}
	}

	_buffer_index += p_length;
}

//

String SiOPMChannelKS::to_string() const {
	String str = "SiOPMChannelKS : operatorCount=";

	str += itos(_operator_count) + "\n";
	str += "  fb=" + itos(_input_level - 6) + "\n";
	str += "  vol=" + itos(_volumes[0]) + " / pan=" + itos(_pan - 64) + "\n";

	if (_operators[0]) {
		str += _operators[0]->to_string() + "\n";
	}
	if (_operators[1]) {
		str += _operators[1]->to_string() + "\n";
	}
	if (_operators[2]) {
		str += _operators[2]->to_string() + "\n";
	}
	if (_operators[3]) {
		str += _operators[3]->to_string() + "\n";
	}

	return str;
}

void SiOPMChannelKS::initialize(SiOPMChannelBase *p_prev, int p_buffer_index) {
	_ks_delay_buffer_index = 0;
	_ks_pitch_index = 0;

	_ks_decay_lpf = 0.875;
	_ks_decay = 0.98;
	_ks_mute_decay_lpf = 0.5;
	_ks_mute_decay = 0.75;

	_output = 0;
	_decay_lpf = _ks_mute_decay_lpf;
	_decay     = _ks_mute_decay;
	_expression = 1;

	SiOPMChannelFM::initialize(p_prev, p_buffer_index);

	_ks_seed_type = KS_SEED_DEFAULT;
	_ks_seed_index = 0;

	set_params_by_value(48, 48, 0, 63, 15, 0, 0, 0, 1, 0, 0, 0, -1, 0);
	_active_operator->set_pulse_generator_type(SiONPulseGeneratorType::PULSE_NOISE_PINK);
	_active_operator->set_pitch_table_type(SiONPitchTableType::PITCH_TABLE_PCM);
}

void SiOPMChannelKS::reset() {
	_ks_delay_buffer.fill(0);

	SiOPMChannelFM::reset();
}

SiOPMChannelKS::SiOPMChannelKS(SiOPMSoundChip *p_chip) : SiOPMChannelFM(p_chip) {
	_ks_delay_buffer.resize_zeroed(KS_BUFFER_SIZE);
}

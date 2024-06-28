/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_pcm.h"

#include "chip/channels/siopm_operator.h"
#include "chip/siopm_channel_params.h"
#include "chip/siopm_sound_chip.h"
#include "chip/siopm_stream.h"
#include "chip/wave/siopm_wave_pcm_data.h"
#include "chip/wave/siopm_wave_pcm_table.h"

void SiOPMChannelPCM::get_channel_params(const Ref<SiOPMChannelParams> &p_params) const {
	p_params->set_operator_count(1);

	p_params->set_algorithm(0);
	p_params->set_envelope_frequency_ratio(_frequency_ratio);

	p_params->set_feedback(0);
	p_params->set_feedback_connection(0);

	p_params->set_lfo_wave_shape(_lfo_wave_shape);
	p_params->set_lfo_frequency_step(_lfo_timer_step_buffer);

	p_params->set_amplitude_modulation_depth(_amplitude_modulation_depth);
	p_params->set_pitch_modulation_depth(_pitch_modulation_depth);

	for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		p_params->set_master_volume(i, _volumes[i]);
	}
	p_params->set_pan(_pan);

	_operator->get_operator_params(p_params->get_operator_params(0));

}

void SiOPMChannelPCM::set_channel_params(const Ref<SiOPMChannelParams> &p_params, bool p_with_volume, bool p_with_modulation) {
	if (p_params->get_operator_count() == 0) {
		return;
	}

	set_algorithm(p_params->get_operator_count(), p_params->get_algorithm());
	set_frequency_ratio(p_params->get_envelope_frequency_ratio());
	//set_feedback(p_params->get_feedback(), p_params->get_feedback_connection()); // Commented out in the original code.

	if (p_with_modulation) {
		initialize_lfo(p_params->get_lfo_wave_shape());
		_set_lfo_timer(p_params->get_lfo_frequency_step());

		set_amplitude_modulation(p_params->get_amplitude_modulation_depth());
		set_pitch_modulation(p_params->get_pitch_modulation_depth());
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

	_filter_type = p_params->get_filter_type();
	{
		int filter_cutoff = p_params->get_filter_cutoff();
		int filter_resonance = p_params->get_filter_resonance();
		int filter_ar = p_params->get_filter_attack_rate();
		int filter_dr1 = p_params->get_filter_decay_rate1();
		int filter_dr2 = p_params->get_filter_decay_rate2();
		int filter_rr = p_params->get_filter_release_rate();
		int filter_dc1 = p_params->get_filter_decay_offset1();
		int filter_dc2 = p_params->get_filter_decay_offset2();
		int filter_sc = p_params->get_filter_sustain_offset();
		int filter_rc = p_params->get_filter_release_offset();
		set_sv_filter(filter_cutoff, filter_resonance, filter_ar, filter_dr1, filter_dr2, filter_rr, filter_dc1, filter_dc2, filter_sc, filter_rc);
	}

	_operator->set_operator_params(p_params->get_operator_params(0));
}

void SiOPMChannelPCM::set_params_by_value(int p_ar, int p_dr, int p_sr, int p_rr, int p_sl, int p_tl, int p_ksr, int p_ksl, int p_mul, int p_dt1, int p_detune, int p_ams, int p_phase, int p_fix_note) {
#define SET_OP_PARAM(m_setter, m_value) \
	if (m_value != INT32_MIN) {         \
		_operator->m_setter(m_value);   \
	}

	SET_OP_PARAM(set_attack_rate,                p_ar);
	SET_OP_PARAM(set_decay_rate,                 p_dr);
	SET_OP_PARAM(set_sustain_rate,               p_sr);
	SET_OP_PARAM(set_release_rate,               p_rr);
	SET_OP_PARAM(set_sustain_level,              p_sl);
	SET_OP_PARAM(set_total_level,                p_tl);
	SET_OP_PARAM(set_key_scaling_rate,           p_ksr);
	SET_OP_PARAM(set_key_scaling_level,          p_ksl);
	SET_OP_PARAM(set_multiple,                   p_mul);
	SET_OP_PARAM(set_dt1,                        p_dt1);
	SET_OP_PARAM(set_detune,                     p_detune);
	SET_OP_PARAM(set_amplitude_modulation_shift, p_ams);
	SET_OP_PARAM(set_key_on_phase,               p_phase);

	if (p_fix_note != INT32_MIN) {
		_operator->set_fixed_pitch_index(p_fix_note << 6);
	}

#undef SET_OP_PARAM
}

void SiOPMChannelPCM::set_wave_data(SiOPMWaveBase *p_wave_data) {
	SiOPMWavePCMData *pcm_data = Object::cast_to<SiOPMWavePCMData>(p_wave_data);
	_pcm_table = Object::cast_to<SiOPMWavePCMTable>(p_wave_data);
	if (_pcm_table) {
		pcm_data = _pcm_table->get_note_data(60);
	}

	if (pcm_data) {
		_sample_pitch_shift = pcm_data->get_sampling_pitch() - 4416; // 69*64
	}

	_operator->set_pcm_data(pcm_data);
}

void SiOPMChannelPCM::set_parameters(Vector<int> p_params) {
	set_params_by_value(
			p_params[1],  p_params[2],  p_params[3],  p_params[4],  p_params[5],
			p_params[6],  p_params[7],  p_params[8],  p_params[9],  p_params[10],
			p_params[11], p_params[12], p_params[13], p_params[14]
	);
}

void SiOPMChannelPCM::set_types(int p_pg_type, SiONPitchTableType p_pt_type) {
	SiOPMWavePCMTable *pcm_table = _table->get_pcm_data(p_pg_type);
	if (pcm_table) {
		set_wave_data(pcm_table);
	} else {
		_sample_pitch_shift = 0;
		_operator->set_pcm_data(nullptr);
	}
}

void SiOPMChannelPCM::set_all_attack_rate(int p_value) {
	_operator->set_attack_rate(p_value);
}

void SiOPMChannelPCM::set_all_release_rate(int p_value) {
	_operator->set_release_rate(p_value);
}

int SiOPMChannelPCM::get_pitch() const {
	return _operator->get_pitch_index() + _sample_pitch_shift;
}

void SiOPMChannelPCM::set_pitch(int p_value) {
	if (_pcm_table) {
		int note = p_value >> 6;
		SiOPMWavePCMData *pcm_data = _pcm_table->get_note_data(note);

		if (pcm_data) {
			_sample_pitch_shift = pcm_data->get_sampling_pitch() - 4416; // 69*64
			_sample_volume = _pcm_table->get_note_volume(note);
			_sample_pan = _pcm_table->get_note_pan(note);
		}

		_operator->set_pcm_data(pcm_data);
	}

	_operator->set_pitch_index(p_value - _sample_pitch_shift);
}

void SiOPMChannelPCM::set_release_rate(int p_value) {
	_operator->set_release_rate(p_value);
}

void SiOPMChannelPCM::set_total_level(int p_value) {
	_operator->set_total_level(p_value);
}

void SiOPMChannelPCM::set_fine_multiple(int p_value) {
	_operator->set_fine_multiple(p_value);
}

void SiOPMChannelPCM::set_phase(int p_value) {
	_operator->set_key_on_phase(p_value);
}

void SiOPMChannelPCM::set_detune(int p_value) {
	_operator->set_detune(p_value);
}

void SiOPMChannelPCM::set_fixed_pitch(int p_value) {
	_operator->set_fixed_pitch_index(p_value);
}

void SiOPMChannelPCM::set_ssg_envelope_control(int p_value) {
	_operator->set_ssg_type_envelope_control(p_value);
}

void SiOPMChannelPCM::set_envelope_reset(bool p_reset) {
	_operator->set_envelope_reset_on_attack(p_reset);
}

// Volume control.

void SiOPMChannelPCM::offset_volume(int p_expression, int p_velocity) {
	int expression_index = p_expression << 1;
	int offset = _expression_table[expression_index] + _velocity_table[p_velocity];

	_operator->offset_total_level(offset);
}

// LFO control.

void SiOPMChannelPCM::_set_lfo_state(bool p_enabled) {
	_lfo_on = (int)p_enabled;
	_lfo_timer_step = p_enabled ? _lfo_timer_step_buffer : 0;
}

void SiOPMChannelPCM::_set_lfo_timer(int p_value) {
	_lfo_timer = (p_value > 0 ? 1 : 0);
	_lfo_timer_step = p_value;
	_lfo_timer_step_buffer = p_value;
}

void SiOPMChannelPCM::set_frequency_ratio(int p_ratio) {
	_frequency_ratio = p_ratio;

	double value_coef = (p_ratio != 0) ? (100.0 / p_ratio) : 1.0;
	_eg_timer_initial = (int)(SiOPMRefTable::ENV_TIMER_INITIAL * value_coef);
	_lfo_timer_initial = (int)(SiOPMRefTable::LFO_TIMER_INITIAL * value_coef);
}

void SiOPMChannelPCM::initialize_lfo(int p_waveform, Vector<int> p_custom_wave_table) {
	SiOPMChannelBase::initialize_lfo(p_waveform, p_custom_wave_table);

	_set_lfo_state(false);

	_amplitude_modulation_depth = 0;
	_pitch_modulation_depth = 0;
	_amplitude_modulation_output_level = 0;
	_pitch_modulation_output_level = 0;

	_pcm_table = nullptr;
	_operator->set_detune2(0);
}

void SiOPMChannelPCM::set_amplitude_modulation(int p_depth) {
	_amplitude_modulation_depth = p_depth << 2;
	_amplitude_modulation_output_level = (_lfo_wave_table[_lfo_phase] * _amplitude_modulation_depth) >> 7 << 3;

	_set_lfo_state(_pitch_modulation_depth != 0 || _amplitude_modulation_depth > 0);
}

void SiOPMChannelPCM::set_pitch_modulation(int p_depth) {
	_pitch_modulation_depth = p_depth;
	_pitch_modulation_output_level = (((_lfo_wave_table[_lfo_phase] << 1) - 255) * _pitch_modulation_depth) >> 8;

	_set_lfo_state(_pitch_modulation_depth != 0 || _amplitude_modulation_depth > 0);

	if (_pitch_modulation_depth == 0) {
		_operator->set_detune2(0);
	}
}

// Processing.

void SiOPMChannelPCM::_no_process(int p_length) {
	// Rotate the output buffer.
	int pipe_index = (_buffer_index + p_length) & (_sound_chip->get_buffer_length() - 1);
	_out_pipe = _sound_chip->get_pipe(4, pipe_index);
	_out_pipe2 = _sound_chip->get_pipe(3, pipe_index);
}

void SiOPMChannelPCM::_update_lfo() {
	_lfo_timer -= _lfo_timer_step;
	if (_lfo_timer >= 0) {
		return;
	}

	_lfo_phase = (_lfo_phase + 1) & 255;

	int value_base = _lfo_wave_table[_lfo_phase];
	_amplitude_modulation_output_level = (value_base * _amplitude_modulation_depth) >> 7 << 3;
	_pitch_modulation_output_level = (((value_base << 1) - 255) * _pitch_modulation_depth) >> 8;

	_operator->set_detune2(_pitch_modulation_output_level);

	_lfo_timer += _lfo_timer_initial;
}

void SiOPMChannelPCM::_process_operator_mono(int p_length, bool p_mix) {
	SinglyLinkedList<int> *base_pipe = (p_mix ? _out_pipe : _sound_chip->get_zero_buffer());
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	// Noop.
	if (_operator->get_pcm_end_point() <= 0) {
		for (int i = 0; i < p_length; i++) {
			out_pipe->value = base_pipe->value;
			out_pipe = out_pipe->next;
			base_pipe = base_pipe->next;
		}

		_out_pipe = out_pipe;
		return;
	}

	SiOPMOperator *ope0 = _operator;

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update LFO.
		_update_lfo();

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);

		// Update PG.
		{
			ope0->tick_pulse_generator();

			int t = ope0->get_phase() >> SiOPMOperator::PCM_WAVE_FIXED_BITS;

			if (t >= ope0->get_pcm_end_point()) {
				if (ope0->get_pcm_loop_point() == -1) {
					ope0->set_eg_state(SiOPMOperator::EG_OFF);
					ope0->update_eg_output();

					// Fast forward.
					for (; i < p_length; i++) {
						out_pipe->value = 0;
						out_pipe = out_pipe->next;
					}
					break;
				} else {
					t -= ope0->get_pcm_end_point() - ope0->get_pcm_loop_point();
					int phase_diff = (ope0->get_pcm_end_point() - ope0->get_pcm_loop_point()) << SiOPMOperator::PCM_WAVE_FIXED_BITS;
					ope0->adjust_phase(-phase_diff);
				}
			}

			int log_idx = ope0->get_wave_value(t);
			log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
			output = _table->log_table[log_idx];
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;
			out_pipe = out_pipe->next;
			base_pipe = base_pipe->next;
		}
	}

	_out_pipe = out_pipe;
}

void SiOPMChannelPCM::_process_operator_stereo(int p_length, bool p_mix) {
	SinglyLinkedList<int> *base_pipe = (p_mix ? _out_pipe : _sound_chip->get_zero_buffer());
	SinglyLinkedList<int> *out_pipe  = _out_pipe;
	SinglyLinkedList<int> *base_pipe2 = (p_mix ? _out_pipe2 : _sound_chip->get_zero_buffer());
	SinglyLinkedList<int> *out_pipe2  = _out_pipe2;

	// Noop.
	if (_operator->get_pcm_end_point() <= 0) {
		for (int i = 0; i < p_length; i++) {
			out_pipe->value = base_pipe->value;
			out_pipe = out_pipe->next;
			base_pipe = base_pipe->next;

			out_pipe2->value = base_pipe2->value;
			out_pipe2 = out_pipe2->next;
			base_pipe2 = base_pipe2->next;
		}

		_out_pipe = out_pipe;
		_out_pipe2 = out_pipe2;
		return;
	}

	SiOPMOperator *ope0 = _operator;

	for (int i = 0; i < p_length; i++) {
		int output_left = 0;
		int output_right = 0;

		// Update LFO.
		_update_lfo();

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);

		// Update PG.
		{
			ope0->tick_pulse_generator();

			int t = ope0->get_phase() >> SiOPMOperator::PCM_WAVE_FIXED_BITS;

			if (t >= ope0->get_pcm_end_point()) {
				if (ope0->get_pcm_loop_point() == -1) {
					ope0->set_eg_state(SiOPMOperator::EG_OFF);
					ope0->update_eg_output();

					// Fast forward.
					for (; i < p_length; i++) {
						out_pipe->value = 0;
						out_pipe = out_pipe->next;
						out_pipe2->value = 0;
						out_pipe2 = out_pipe2->next;
					}
					break;
				} else {
					t -= ope0->get_pcm_end_point() - ope0->get_pcm_loop_point();
					int phase_diff = (ope0->get_pcm_end_point() - ope0->get_pcm_loop_point()) << SiOPMOperator::PCM_WAVE_FIXED_BITS;
					ope0->adjust_phase(-phase_diff);
				}
			}

			// Left output.
			{
				t <<= 1;
				int log_idx = ope0->get_wave_value(t);
				log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				output_left = _table->log_table[log_idx];
			}

			// Right output.
			{
				t++;
				int log_idx = ope0->get_wave_value(t);
				log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				output_right = _table->log_table[log_idx];
			}
		}

		// Output and increment pointers.
		{
			out_pipe->value = output_left + base_pipe->value;
			out_pipe = out_pipe->next;
			base_pipe = base_pipe->next;

			out_pipe2->value = output_right + base_pipe2->value;
			out_pipe2 = out_pipe2->next;
			base_pipe2 = base_pipe2->next;
		}
	}

	_out_pipe = out_pipe;
	_out_pipe2 = out_pipe2;
}

void SiOPMChannelPCM::_write_stream_mono(SinglyLinkedList<int> *p_output, int p_length) {
	double volume_coef = _sample_volume * _sound_chip->get_pcm_volume();
	int pan = CLAMP(_pan + _sample_pan, 0, 128);

	if (_has_effect_send) {
		for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
			if (_volumes[i] > 0) {
				SiOPMStream *stream = _streams[i] ? _streams[i] : _sound_chip->get_stream_slot(i);
				if (stream) {
					stream->write(p_output, _buffer_index, p_length, _volumes[i] * volume_coef, pan);
				}
			}
		}
	} else {
		SiOPMStream *stream = _streams[0] ? _streams[0] : _sound_chip->get_output_stream();
		stream->write(p_output, _buffer_index, p_length, _volumes[0] * volume_coef, pan);
	}
}

void SiOPMChannelPCM::_write_stream_stereo(SinglyLinkedList<int> *p_output_left, SinglyLinkedList<int> *p_output_right, int p_length) {
	double volume_coef = _sample_volume * _sound_chip->get_pcm_volume();
	int pan = CLAMP(_pan + _sample_pan, 0, 128);

	if (_has_effect_send) {
		for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
			if (_volumes[i] > 0) {
				SiOPMStream *stream = _streams[i] ? _streams[i] : _sound_chip->get_stream_slot(i);
				if (stream) {
					stream->write_stereo(p_output_left, p_output_right, _buffer_index, p_length, _volumes[i] * volume_coef, pan);
				}
			}
		}
	} else {
		SiOPMStream *stream = _streams[0] ? _streams[0] : _sound_chip->get_output_stream();
		stream->write_stereo(p_output_left, p_output_right, _buffer_index, p_length, _volumes[0] * volume_coef, pan);
	}
}

void SiOPMChannelPCM::note_on() {
	_operator->note_on();
	_is_note_on = true;
	_is_idling = false;

	SiOPMChannelBase::note_on();
}

void SiOPMChannelPCM::note_off() {
	_operator->note_off();
	_is_note_on = false;

	SiOPMChannelBase::note_off();
}

void SiOPMChannelPCM::reset_channel_buffer_status() {
	_buffer_index = 0;
	_is_idling = (_operator->get_eg_output() > IDLING_THRESHOLD && _operator->get_eg_state() != SiOPMOperator::EG_ATTACK);
}

void SiOPMChannelPCM::buffer(int p_length) {
	if (_is_idling) {
		buffer_no_process(p_length);
		return;
	}

	if (_operator->get_pcm_channel_num() == 1) {
		// Preserve the start of the output pipe.
		SinglyLinkedList<int> *mono_out = _out_pipe;

		_process_operator_mono(p_length, false);

		if (_filter_on) {
			_apply_sv_filter(mono_out, p_length, _filter_variables);
		}

		if (!_mute) {
			_write_stream_mono(mono_out, p_length);
		}


	} else {
		// Preserve the start of output pipes.
		SinglyLinkedList<int> *left_out = _out_pipe;
		SinglyLinkedList<int> *right_out = _out_pipe2;

		_process_operator_stereo(p_length, false);

		if (_filter_on) {
			_apply_sv_filter(left_out, p_length, _filter_variables);
			_apply_sv_filter(right_out, p_length, _filter_variables2);
		}

		if (!_mute) {
			_write_stream_stereo(left_out, right_out, p_length);
		}
	}
}

void SiOPMChannelPCM::buffer_no_process(int p_length) {
	_no_process(p_length);
	_buffer_index += p_length;
}

//

String SiOPMChannelPCM::to_string() const {
	String str = "SiOPMChannelPCM : \n";

	str += "  vol=" + itos(_volumes[0]) + " / pan=" + itos(_pan - 64) + "\n";
	str += _operator->to_string() + "\n";

	return str;
}

void SiOPMChannelPCM::initialize(SiOPMChannelBase *p_prev, int p_buffer_index) {
	_operator->initialize();

	_is_note_on = false;
	_out_pipe2 = _sound_chip->get_pipe(3, p_buffer_index);

	_filter_variables2[0] = 0;
	_filter_variables2[1] = 0;
	_filter_variables2[2] = 0;

	_sample_pitch_shift = 0;
	_sample_volume = 1;
	_sample_pan = 0;

	SiOPMChannelBase::initialize(p_prev, p_buffer_index);
}

void SiOPMChannelPCM::reset() {
	_operator->reset();
	_is_note_on = false;
	_is_idling = true;
}

void SiOPMChannelPCM::_bind_methods() {
	// To be used as callables.
	ClassDB::bind_method(D_METHOD("_no_process", "length"), &SiOPMChannelPCM::_no_process);
}

SiOPMChannelPCM::SiOPMChannelPCM(SiOPMSoundChip *p_chip) : SiOPMChannelBase(p_chip) {
	_operator = memnew(SiOPMOperator(p_chip));
	_process_function = Callable(this, "_no_process");

	initialize(nullptr, 0);
}

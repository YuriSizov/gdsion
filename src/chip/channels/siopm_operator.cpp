/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_operator.h"

#include <godot_cpp/classes/random_number_generator.hpp>
#include "chip/siopm_operator_params.h"
#include "chip/siopm_ref_table.h"
#include "chip/siopm_sound_chip.h"
#include "chip/wave/siopm_wave_pcm_data.h"
#include "chip/wave/siopm_wave_table.h"
#include "utils/godot_util.h"

const int SiOPMOperator::_eg_next_state_table[2][EG_MAX] = {
	// EG_ATTACK,  EG_DECAY,   EG_SUSTAIN, EG_RELEASE, EG_OFF
	{  EG_DECAY,   EG_SUSTAIN, EG_OFF,     EG_OFF,     EG_OFF }, // normal
	{  EG_DECAY,   EG_SUSTAIN, EG_ATTACK,  EG_OFF,     EG_OFF }  // ssgev
};

// FM module parameters.

void SiOPMOperator::set_attack_rate(int p_value) {
	_attack_rate = p_value & 63;

	if (_ssg_type_envelope_control == 8 || _ssg_type_envelope_control == 12) {
		_eg_ssgec_attack_rate = (_attack_rate >= 56) ? 1 : 0;
	} else {
		_eg_ssgec_attack_rate = (_attack_rate >= 60) ? 1 : 0;
	}
}

void SiOPMOperator::set_decay_rate(int p_value) {
	_decay_rate = p_value & 63;
}

void SiOPMOperator::set_sustain_rate(int p_value) {
	_sustain_rate = p_value & 63;
}

void SiOPMOperator::set_release_rate(int p_value) {
	_release_rate = p_value & 63;
}

void SiOPMOperator::set_sustain_level(int p_value) {
	_sustain_level = p_value & 15;
	_eg_sustain_level = _table->eg_sustain_level_table[p_value];
}

void SiOPMOperator::_update_total_level() {
	_eg_total_level = ((_total_level + (_key_code >> _eg_key_scale_level_rshift)) << SiOPMRefTable::ENV_LSHIFT) + _eg_tl_offset + _mute;

	if (_eg_total_level > SiOPMRefTable::ENV_BOTTOM) {
		_eg_total_level = SiOPMRefTable::ENV_BOTTOM;
	}
	_eg_total_level -= SiOPMRefTable::ENV_TOP; // Table index + 192.

	update_eg_output();
}

void SiOPMOperator::set_total_level(int p_value) {
	_total_level = CLAMP(p_value, 0, 127);
	_update_total_level();
}

void SiOPMOperator::offset_total_level(int p_offset) {
	_eg_tl_offset = p_offset;
	_update_total_level();
}

int SiOPMOperator::get_key_scaling_rate() const {
	return 5 - _key_scaling_rate;
}

void SiOPMOperator::set_key_scaling_rate(int p_value) {
	_key_scaling_rate = 5 - (p_value & 3);
	_eg_key_scale_rate = _key_code >> _key_scaling_rate;
}

void SiOPMOperator::set_key_scaling_level(int p_value, bool p_silent) {
	_key_scaling_level = p_value & 3;
	// [0,1,2,3]->[8,4,3,2]
	_eg_key_scale_level_rshift = (_key_scaling_level == 0) ? 8 : (5 - _key_scaling_level);

	if (!p_silent) {
		_update_total_level();
	}
}

int SiOPMOperator::get_multiple() const {
	return (_multiple >> 7);
}

void SiOPMOperator::set_multiple(int p_value) {
	int multiple = p_value & 15;
	_multiple = (multiple != 0) ? (multiple << 7) : 64;
	_update_pitch();
}

void SiOPMOperator::set_dt1(int p_value) {
	_dt1 = p_value & 7;
	_update_pitch();
}

void SiOPMOperator::set_dt2(int p_value) {
	_dt2 = p_value & 3;
	_pitch_index_shift = _table->dt2_table[_dt2];
	_update_pitch();
}

bool SiOPMOperator::is_amplitude_modulation_enabled() const {
	return _amplitude_modulation_shift != 16;
}

void SiOPMOperator::set_amplitude_modulation_enabled(bool p_enabled) {
	_amplitude_modulation_shift = p_enabled ? 2 : 16;
}

int SiOPMOperator::get_amplitude_modulation_shift() const {
	return (_amplitude_modulation_shift == 16) ? 0 : (3 - _amplitude_modulation_shift);
}

void SiOPMOperator::set_amplitude_modulation_shift(int p_value) {
	_amplitude_modulation_shift = (p_value != 0) ? (3 - p_value) : 16;
}

void SiOPMOperator::_update_key_code(int p_value) {
	_key_code = p_value;
	_eg_key_scale_rate = _key_code >> _key_scaling_rate;
	_update_total_level();
}

void SiOPMOperator::set_key_code(int p_value) {
	if (_pitch_fixed) {
		return;
	}

	_update_key_code(p_value & 127);
	_pitch_index = ((_key_code - (_key_code >> 2)) << 6) | (_pitch_index & 63);
	_update_pitch();
}

bool SiOPMOperator::is_mute() const {
	return _mute != 0;
}

void SiOPMOperator::set_mute(bool p_mute) {
	_mute = p_mute ? SiOPMRefTable::ENV_BOTTOM : 0;
	_update_total_level();
}

void SiOPMOperator::set_ssg_type_envelope_control(int p_value) {
	if (p_value > 7) {
		_eg_state_table_index = 1;
		_ssg_type_envelope_control = p_value;
		if (_ssg_type_envelope_control > 17) {
			_ssg_type_envelope_control = 9;
		}
	} else {
		_eg_state_table_index = 0;
		_ssg_type_envelope_control = 0;
	}
}

// Pulse generator.

void SiOPMOperator::_update_pitch() {
	int index = (_pitch_index + _pitch_index_shift + _pitch_index_shift2) & _pitch_table_filter;
	_update_phase_step(_pitch_table[index] >> _wave_phase_step_shift);
}

void SiOPMOperator::_update_phase_step(int p_step) {
	_phase_step = p_step;
	_phase_step += _table->dt1_table[_dt1][_key_code];
	_phase_step *= _multiple;
	_phase_step >>= (7 - _table->sample_rate_pitch_shift);  // 44kHz:1/128, 22kHz:1/256
}

void SiOPMOperator::set_pulse_generator_type(int p_type) {
	_pg_type = p_type & SiOPMRefTable::PG_FILTER;

	Ref<SiOPMWaveTable> wave_table = _table->get_wave_table(_pg_type);
	_wave_table = wave_table->get_wavelet();
	_wave_fixed_bits = wave_table->get_fixed_bits();
}

void SiOPMOperator::set_pitch_table_type(SiONPitchTableType p_type) {
	_pt_type = p_type;

	_wave_phase_step_shift = (SiOPMRefTable::PHASE_BITS - _wave_fixed_bits) & _table->phase_step_shift_filter[p_type];
	_pitch_table = _table->pitch_table[p_type];
	_pitch_table_filter = _pitch_table.size() - 1;
}

int SiOPMOperator::get_wave_value(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _wave_table.size(), -1);
	return _wave_table[p_index];
}

void SiOPMOperator::set_fixed_pitch_index(int p_value) {
	if (p_value > 0) {
		_pitch_index = p_value;

		_update_key_code(_table->note_number_to_key_code[(_pitch_index >> 6) & 127]);
		_update_pitch();
		_pitch_fixed = true;
	} else {
		_pitch_fixed = false;
	}
}

void SiOPMOperator::set_pitch_index(int p_value)
{
	if (_pitch_fixed) {
		return;
	}

	_pitch_index = p_value;
	_update_key_code(_table->note_number_to_key_code[(p_value >> 6) & 127]);
	_update_pitch();
}

void SiOPMOperator::set_detune(int p_value) {
	_dt2 = 0;
	_pitch_index_shift = p_value;
	_update_pitch();
}

void SiOPMOperator::set_detune2(int p_value) {
	_pitch_index_shift2 = p_value;
	_update_pitch();
}

void SiOPMOperator::set_fine_multiple(int p_value) {
	_multiple = p_value;
	_update_pitch();
}

int SiOPMOperator::get_key_on_phase() const {
	if (_key_on_phase >= 0) {
		return _key_on_phase >> (SiOPMRefTable::PHASE_BITS - 8);
	} else {
		return (_key_on_phase == -1) ? -1 : 255;
	}
}

void SiOPMOperator::set_key_on_phase(int p_phase) {
	if (p_phase == 255) {
		_key_on_phase = -2;
	} else if (p_phase == -1) {
		_key_on_phase = -1;
	} else {
		_key_on_phase = (p_phase & 255) << (SiOPMRefTable::PHASE_BITS - 8);
	}
}

int SiOPMOperator::get_fm_level() const {
	return (_fm_shift > 10) ? (_fm_shift - 10) : 0;
}

void SiOPMOperator::set_fm_level(int p_level) {
	_fm_shift = (p_level != 0) ? (p_level + 10) : 0;
}

int SiOPMOperator::get_key_fraction() const {
	return (_pitch_index & 63);
}

void SiOPMOperator::set_key_fraction(int p_value) {
	_pitch_index = (_pitch_index & 0xffc0) | (p_value & 63);
	_update_pitch();
}

void SiOPMOperator::set_fnumber(int p_value) {
	// Naive implementation.
	_update_key_code((p_value >> 7) & 127);
	_dt2 = 0;
	_pitch_index = 0;
	_pitch_index_shift = 0;
	_update_phase_step((p_value & 2047) << ((p_value >> 11) & 7));
}

void SiOPMOperator::tick_pulse_generator(int p_extra) {
	_phase += _phase_step + p_extra;
}

// Envelope generator.

void SiOPMOperator::_shift_eg_state(EGState p_state) {
	switch (p_state) {
		case EG_ATTACK: {
			_eg_ssgec_state++;
			if (_eg_ssgec_state == 3) {
				_eg_ssgec_state = 1;
			}

			if (_attack_rate + _eg_key_scale_rate < 62) {
				if (_envelope_reset_on_attack) {
					_eg_level = SiOPMRefTable::ENV_BOTTOM;
				}
				_eg_state = EG_ATTACK;
				_eg_level_table = make_vector<int>(_table->eg_level_tables[0]);

				int index = (_attack_rate != 0) ? (_attack_rate + _eg_key_scale_rate) : 96;
				_eg_increment_table = make_vector<int>(_table->eg_increment_tables_attack[_table->eg_table_selector[index]]);
				_eg_timer_step = _table->eg_timer_steps[index];
				break;
			}
		}
			[[fallthrough]];

		case EG_DECAY: {
			if (_eg_sustain_level) {
				_eg_state = EG_DECAY;

				if (_ssg_type_envelope_control != 0) {
					_eg_level = 0;

					_eg_state_shift_level = _eg_sustain_level >> 2;
					if (_eg_state_shift_level > SiOPMRefTable::ENV_BOTTOM_SSGEC) {
						_eg_state_shift_level = SiOPMRefTable::ENV_BOTTOM_SSGEC;
					}

					int level_index = _table->eg_ssg_table_index[_ssg_type_envelope_control - 8][_eg_ssgec_attack_rate][_eg_ssgec_state];
					_eg_level_table = make_vector<int>(_table->eg_level_tables[level_index]);
				} else {
					_eg_level = 0;
					_eg_state_shift_level = _eg_sustain_level;
					_eg_level_table = make_vector<int>(_table->eg_level_tables[0]);
				}

				int index = (_decay_rate != 0) ? (_decay_rate + _eg_key_scale_rate) : 96;
				_eg_increment_table = make_vector<int>(_table->eg_increment_tables[_table->eg_table_selector[index]]);
				_eg_timer_step = _table->eg_timer_steps[index];
				break;
			}
		}
			[[fallthrough]];

		case EG_SUSTAIN: {
			_eg_state = EG_SUSTAIN;

			if (_ssg_type_envelope_control != 0) {
				_eg_level = _eg_sustain_level >> 2;
				_eg_state_shift_level = SiOPMRefTable::ENV_BOTTOM_SSGEC;

				int level_index = _table->eg_ssg_table_index[_ssg_type_envelope_control - 8][_eg_ssgec_attack_rate][_eg_ssgec_state];
				_eg_level_table = make_vector<int>(_table->eg_level_tables[level_index]);
			} else {
				_eg_level = _eg_sustain_level;
				_eg_state_shift_level = SiOPMRefTable::ENV_BOTTOM;
				_eg_level_table = make_vector<int>(_table->eg_level_tables[0]);
			}

			int index = (_sustain_rate != 0) ? (_sustain_rate + _eg_key_scale_rate) : 96;
			_eg_increment_table = make_vector<int>(_table->eg_increment_tables[_table->eg_table_selector[index]]);
			_eg_timer_step = _table->eg_timer_steps[index];
		} break;

		case EG_RELEASE: {
			if (_eg_level < SiOPMRefTable::ENV_BOTTOM) {
				_eg_state = EG_RELEASE;
				_eg_state_shift_level = SiOPMRefTable::ENV_BOTTOM;
				_eg_level_table = make_vector<int>(_table->eg_level_tables[_ssg_type_envelope_control != 0 ? 1 : 0]);

				int index = _release_rate + _eg_key_scale_rate;
				_eg_increment_table = make_vector<int>(_table->eg_increment_tables[_table->eg_table_selector[index]]);
				_eg_timer_step = _table->eg_timer_steps[index];
				break;
			}
		}
			[[fallthrough]];

		case EG_OFF:
		default: {
			_eg_state = EG_OFF;
			_eg_level = SiOPMRefTable::ENV_BOTTOM;
			_eg_state_shift_level = SiOPMRefTable::ENV_BOTTOM + 1;
			_eg_level_table = make_vector<int>(_table->eg_level_tables[0]);

			_eg_increment_table = make_vector<int>(_table->eg_increment_tables[17]); // 17 = all zero
			_eg_timer_step = _table->eg_timer_steps[96]; // 96 = all zero
		} break;
	}
}

// Original implementation inlines all this code (with private member access)
// for performance reasons, as stated in the comments. This is probably irrelevant for us.
// But if it's not, there's got to be a better way to realize this optimization.
void SiOPMOperator::tick_eg(int p_timer_initial) {
	_eg_timer -= _eg_timer_step;
	if (_eg_timer >= 0) {
		return;
	}

	if (_eg_state == SiOPMOperator::EG_ATTACK) {
		int offset = _eg_increment_table[_eg_counter];
		if (offset > 0) {
			_eg_level -= 1 + (_eg_level >> offset);
			if (_eg_level <= 0) {
				_shift_eg_state((EGState)_eg_next_state_table[_eg_state_table_index][_eg_state]);
			}
		}
	} else {
		_eg_level += _eg_increment_table[_eg_counter];
		if (_eg_level >= _eg_state_shift_level) {
			_shift_eg_state((EGState)_eg_next_state_table[_eg_state_table_index][_eg_state]);
		}
	}

	update_eg_output();
	_eg_counter = (_eg_counter + 1) & 7;

	_eg_timer += p_timer_initial;
}

void SiOPMOperator::update_eg_output() {
	_eg_output = (_eg_level_table[_eg_level] + _eg_total_level) << 3;
}

void SiOPMOperator::update_eg_output_from(SiOPMOperator *p_other) {
	_eg_output = (p_other->_eg_level_table[p_other->_eg_level] + _eg_total_level) << 3;
}

// Pipes.

void SiOPMOperator::set_pipes(SinglyLinkedList<int> *p_out_pipe, SinglyLinkedList<int> *p_in_pipe, bool p_final) {
	_final = p_final;
	_fm_shift  = 15;

	_out_pipe = p_out_pipe;
	_in_pipe = p_in_pipe ? p_in_pipe : _sound_chip->get_zero_buffer();
	_base_pipe = (p_out_pipe != p_in_pipe) ? p_out_pipe : _sound_chip->get_zero_buffer();
}

//

void SiOPMOperator::set_operator_params(const Ref<SiOPMOperatorParams> &p_params) {
	// Some code here is duplicated from respective setters to avoid calling them
	// and triggering side effects. Modify with care.

	set_pulse_generator_type(p_params->get_pulse_generator_type());
	set_pitch_table_type(p_params->get_pitch_table_type());

	set_key_on_phase(p_params->get_initial_phase());

	set_attack_rate(p_params->get_attack_rate());
	set_decay_rate(p_params->get_decay_rate());
	set_sustain_rate(p_params->get_sustain_rate());
	set_release_rate(p_params->get_release_rate());

	set_key_scaling_rate(p_params->get_key_scaling_rate());
	set_key_scaling_level(p_params->get_key_scaling_level(), true);

	set_amplitude_modulation_shift(p_params->get_amplitude_modulation_shift());

	_multiple = p_params->get_fine_multiple();
	_fm_shift = (p_params->get_frequency_modulation_level() & 7) + 10;
	_dt1 = p_params->get_detune1() & 7;
	_pitch_index_shift = p_params->get_detune();

	_mute = p_params->is_mute() ? SiOPMRefTable::ENV_BOTTOM : 0;
	_ssg_type_envelope_control = p_params->get_ssg_type_envelope_control();
	_envelope_reset_on_attack = p_params->is_envelope_reset_on_attack();

	if (p_params->get_fixed_pitch() > 0) {
		_pitch_index = p_params->get_fixed_pitch();

		_update_key_code(_table->note_number_to_key_code[(_pitch_index >> 6) & 127]);
		_pitch_fixed = true;
	} else {
		_pitch_fixed = false;
	}

	set_sustain_level(p_params->get_sustain_level() & 15);
	set_total_level(p_params->get_total_level());

	_update_pitch();
}

void SiOPMOperator::get_operator_params(const Ref<SiOPMOperatorParams> &r_params) {
	r_params->set_pulse_generator_type(_pg_type);
	r_params->set_pitch_table_type(_pt_type);

	r_params->set_attack_rate(_attack_rate);
	r_params->set_decay_rate(_decay_rate);
	r_params->set_sustain_rate(_sustain_rate);
	r_params->set_release_rate(_release_rate);
	r_params->set_sustain_level(_sustain_level);
	r_params->set_total_level(_total_level);

	r_params->set_key_scaling_rate(get_key_scaling_rate());
	r_params->set_key_scaling_level(_key_scaling_level);
	r_params->set_fine_multiple(get_fine_multiple());
	r_params->set_detune1(_dt1);
	r_params->set_detune(get_detune());
	r_params->set_amplitude_modulation_shift(get_amplitude_modulation_shift());

	r_params->set_ssg_type_envelope_control(_ssg_type_envelope_control);
	r_params->set_envelope_reset_on_attack(is_envelope_reset_on_attack());

	r_params->set_initial_phase(get_key_on_phase());
	r_params->set_frequency_modulation_level(get_fm_level());
}

void SiOPMOperator::set_wave_table(const Ref<SiOPMWaveTable> &p_wave_table) {
	_pg_type = SiONPulseGeneratorType::PULSE_USER_CUSTOM;
	_pt_type = p_wave_table->get_default_pitch_table_type();

	_wave_table = p_wave_table->get_wavelet();
	_wave_fixed_bits = p_wave_table->get_fixed_bits();
}

void SiOPMOperator::set_pcm_data(const Ref<SiOPMWavePCMData> &p_pcm_data) {
	if (p_pcm_data.is_valid() && !p_pcm_data->get_wavelet().is_empty()) {
		_pg_type = SiONPulseGeneratorType::PULSE_USER_PCM;
		_pt_type = SiONPitchTableType::PITCH_TABLE_PCM;

		_wave_table = p_pcm_data->get_wavelet();
		_wave_fixed_bits = PCM_WAVE_FIXED_BITS;

		_pcm_channel_num = p_pcm_data->get_channel_count();
		_pcm_start_point = p_pcm_data->get_start_point();
		_pcm_end_point = p_pcm_data->get_end_point();
		_pcm_loop_point = p_pcm_data->get_loop_point();

		_key_on_phase = _pcm_start_point << PCM_WAVE_FIXED_BITS;
	} else {
		// Quick initialization for SiOPMChannelPCM.
		_pcm_end_point = _pcm_loop_point = 0;
		_pcm_loop_point = -1;
	}
}

void SiOPMOperator::note_on() {
	if (_key_on_phase >= 0) {
		_phase = _key_on_phase;
	} else if (_key_on_phase == -1) {
		Ref<RandomNumberGenerator> rng;
		rng.instantiate();

		_phase = int(rng->randi_range(0, SiOPMRefTable::PHASE_MAX));
	}

	_eg_ssgec_state = -1;
	_shift_eg_state(EG_ATTACK);
	update_eg_output();
}

void SiOPMOperator::note_off() {
	_shift_eg_state(EG_RELEASE);
	update_eg_output();
}

//

void SiOPMOperator::initialize() {
	// Reset operator connections.
	_final = true;
	_in_pipe   = _sound_chip->get_zero_buffer();
	_base_pipe = _sound_chip->get_zero_buffer();
	_feed_pipe->get()->value = 0;

	// Reset all parameters.
	set_operator_params(_sound_chip->get_init_operator_params());

	// Reset some other parameters.

	_eg_tl_offset  = 0;
	_pitch_index_shift2 = 0;

	_pcm_channel_num = 0;
	_pcm_start_point = 0;
	_pcm_end_point = 0;
	_pcm_loop_point = -1;

	// Reset PG and EG states.
	reset();
}

void SiOPMOperator::reset() {
	_shift_eg_state(EG_OFF);
	update_eg_output();
	_eg_timer = SiOPMRefTable::ENV_TIMER_INITIAL;
	_eg_counter = 0;
	_eg_ssgec_state = 0;

	_phase = 0;
}

String SiOPMOperator::_to_string() const {
	String params = "";

	params += "pg=" + itos(_pg_type) + ", ";
	params += "pt=" + itos(_pt_type) + ", ";

	params += "ar=" + itos(_attack_rate) + ", ";
	params += "dr=" + itos(_decay_rate) + ", ";
	params += "sr=" + itos(_sustain_rate) + ", ";
	params += "rr=" + itos(_release_rate) + ", ";
	params += "sl=" + itos(_sustain_level) + ", ";
	params += "tl=" + itos(_total_level) + ", ";

	params += "keyscale=(" + itos(get_key_scaling_rate()) + ", " + itos(get_key_scaling_level()) + "), ";
	params += "fmul=" + itos(get_fine_multiple()) + ", ";
	params += "detune=(" + itos(get_dt1()) + ", " + itos(get_detune()) + "), ";

	params += "amp=" + itos(get_amplitude_modulation_shift()) + ", ";
	params += "phase=" + itos(get_key_on_phase()) + ", ";
	params += "note=" + String(is_pitch_fixed() ? "yes" : "no") + ", ";

	params += "ssg=" + itos(_ssg_type_envelope_control) + ", ";
	params += "mute=" + itos(_mute) + ", ";
	params += "reset=" + String(_envelope_reset_on_attack ? "yes" : "no");

	return "SiOPMOperator: " + params;
}

SiOPMOperator::SiOPMOperator(SiOPMSoundChip *p_chip) {
	_table = SiOPMRefTable::get_instance();
	_sound_chip = p_chip;

	_feed_pipe = memnew(SinglyLinkedList<int>(1, 0, true));
	_eg_increment_table = make_vector<int>(_table->eg_increment_tables[17]);
	_eg_level_table = make_vector<int>(_table->eg_level_tables[0]);
}

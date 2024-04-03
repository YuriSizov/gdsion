/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_fm.h"

#include <godot_cpp/core/class_db.hpp>
#include "processor/channels/siopm_operator.h"
#include "processor/siopm_channel_params.h"
#include "processor/siopm_module.h"
#include "processor/wave/siopm_wave_pcm_data.h"
#include "processor/wave/siopm_wave_pcm_table.h"
#include "processor/wave/siopm_wave_table.h"

List<SiOPMOperator *> SiOPMChannelFM::_free_operators;

SiOPMOperator *SiOPMChannelFM::_alloc_fm_operator() {
	if (_free_operators.size() > 0) {
		SiOPMOperator *op = _free_operators.back()->get();
		_free_operators.pop_back();

		return op;
	} else {
		return memnew(SiOPMOperator(_chip));
	}
}

void SiOPMChannelFM::_free_fm_operator(SiOPMOperator *p_operator) {
	_free_operators.push_back(p_operator);
}

//

void SiOPMChannelFM::_update_process_function() {
	_process_function = _process_function_list[_lfo_on][_process_function_type];
}

void SiOPMChannelFM::_update_operator_count(int p_count) {
	if (_operator_count < p_count) {
		for (int i = _operator_count; i < p_count; i++) {
			_operators.write[i] = _alloc_fm_operator();
			_operators[i]->initialize();
		}
	} else if (_operator_count > p_count) {
		for (int i = p_count; i < _operator_count; i++) {
			_free_fm_operator(_operators[i]);
			_operators.write[i] = nullptr;
		}
	}

	_operator_count = p_count;
	_process_function_type = (ProcessType)(p_count - 1);
	_update_process_function();

	_active_operator = _operators[_operator_count - 1];

	if (_input_mode == INPUT_FEEDBACK) {
		set_feedback(0, 0);
	}
}

//

void SiOPMChannelFM::get_channel_params(SiOPMChannelParams *r_params) const {
	r_params->set_operator_count(_operator_count);

	r_params->set_algorithm(_algorithm);
	r_params->set_envelope_frequency_ratio(_frequency_ratio);

	r_params->set_feedback(0);
	r_params->set_feedback_connection(0);
	for (int i = 0; i < _operator_count; i++) {
		if (_in_pipe == _operators[i]->get_feed_pipe()) {
			r_params->set_feedback(_input_level - 6);
			r_params->set_feedback_connection(i);
			break;
		}
	}

	r_params->set_lfo_wave_shape(_lfo_wave_shape);
	r_params->set_lfo_frequency_step(_lfo_timer_step_buffer);

	r_params->set_amplitude_modulation_depth(_amplitude_modulation_depth);
	r_params->set_pitch_modulation_depth(_pitch_modulation_depth);

	for (int i = 0; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		r_params->set_master_volume(i, _volumes[i]);
	}
	r_params->set_pan(_pan);

	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->get_operator_params(r_params->get_operator_params(i));
	}
}

void SiOPMChannelFM::set_channel_params(SiOPMChannelParams *p_params, bool p_with_volume, bool p_with_modulation) {
	if (p_params->get_operator_count() == 0) {
		return;
	}

	set_algorithm(p_params->get_operator_count(), p_params->get_algorithm());
	set_frequency_ratio(p_params->get_envelope_frequency_ratio());
	set_feedback(p_params->get_feedback(), p_params->get_feedback_connection());

	if (p_with_modulation) {
		initialize_lfo(p_params->get_lfo_wave_shape());
		_set_lfo_timer(p_params->get_lfo_frequency_step());

		set_amplitude_modulation(p_params->get_amplitude_modulation_depth());
		set_pitch_modulation(p_params->get_pitch_modulation_depth());
	}

	if (p_with_volume) {
		for (int i = 0; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
			_volumes.write[i] = p_params->get_master_volume(i);
		}

		_has_effect_send = false;
		for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
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

	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->set_operator_params(p_params->get_operator_params(i));
	}
}

void SiOPMChannelFM::set_params_by_value(int p_ar, int p_dr, int p_sr, int p_rr, int p_sl, int p_tl, int p_ksr, int p_ksl, int p_mul, int p_dt1, int p_detune, int p_ams, int p_phase, int p_fix_note) {
#define SET_OP_PARAM(m_setter, m_value)      \
	if (m_value != INT32_MIN) {              \
		_active_operator->m_setter(m_value); \
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
	SET_OP_PARAM(set_fixed_pitch_index,          p_fix_note << 6);

#undef SET_OP_PARAM
}

void SiOPMChannelFM::set_wave_data(SiOPMWaveBase *p_wave_data) {
	SiOPMWavePCMData *pcm_data = Object::cast_to<SiOPMWavePCMData>(p_wave_data);
	SiOPMWavePCMTable *pcm_table = Object::cast_to<SiOPMWavePCMTable>(p_wave_data);
	if (pcm_table) {
		pcm_data = pcm_table->get_note_data(60);
	}

	if (pcm_data && !pcm_data->get_wavelet().is_empty()) {
		_update_operator_count(1);
		_process_function_type = PROCESS_PCM;
		_update_process_function();
		_active_operator->set_pcm_data(pcm_data);
		set_envelope_reset(true);

		return;
	}

	SiOPMWaveTable *wave_table = Object::cast_to<SiOPMWaveTable>(p_wave_data);
	if (wave_table && !wave_table->get_wavelet().is_empty()) {
		_operators[0]->set_wave_table(wave_table);
		if (_operators[1]) {
			_operators[1]->set_wave_table(wave_table);
		}
		if (_operators[2]) {
			_operators[2]->set_wave_table(wave_table);
		}
		if (_operators[3]) {
			_operators[3]->set_wave_table(wave_table);
		}

		return;
	}
}

void SiOPMChannelFM::set_channel_number(int p_value) {
	_register_map_channel = p_value;
}

void SiOPMChannelFM::_set_by_opm_register(int p_address, int p_data) {
	static int _pmd = 0;
	static int _amd = 0;

	if (p_address < 0x20) { // Module parameter
		switch (p_address) {
			case 15: { // NOIZE:7 FREQ:4-0 for channel#7
				if (_register_map_channel == 7 && _operator_count == 4 && (p_data & 128)) {
					_operators[3]->set_pulse_generator_type(SiOPMTable::PG_NOISE_PULSE);
					_operators[3]->set_pitch_table_type(SiOPMTable::PT_OPM_NOISE);
					_operators[3]->set_pitch_index(((p_data & 31) << 6) + 2048);
				}
			} break;

			case 24: { // LFO FREQ:7-0 for all 8 channels
				_set_lfo_timer(_table->lfo_timer_steps[p_data]);
			} break;

			case 25: { // A(0)/P(1):7 DEPTH:6-0 for all 8 channels
				// NOTE: Original code has pitch and amp the other way around, which is inconsistent with the rest of the code.
				if (p_data & 128) {
					_pmd = p_data & 127;
				} else {
					_amd = p_data & 127;
				}
			} break;

			case 27: { // LFO WS:10 for all 8 channels
				initialize_lfo(p_data & 3);
			} break;
		}
	} else if (_register_map_channel == (p_address & 7)) {
		if (p_address < 0x40) { // Channel parameter
			switch ((p_address - 0x20) >> 3) {
				case 0: { // L:7 R:6 FB:5-3 ALG:2-0
					set_algorithm(4, p_data & 7);
					set_feedback((p_data >> 3) & 7, 0);

					int value = p_data >> 6;
					_volumes.write[0] = (value != 0 ? 0.5 : 0);
					_pan = (value == 1 ? 128 : (value == 2 ? 0 : 64));
				} break;

				case 1: { // KC:6-0
					for (int i = 0; i < 4; i++) {
						_operators[i]->set_key_code(p_data & 127);
					}
				} break;

				case 2: { // KF:6-0
					for (int i = 0; i < 4; i++) {
						_operators[i]->set_key_fraction(p_data & 127);
					}
				} break;

				case 3: { // PMS:6-4 AMS:10
					int pitch_mod_shift = (p_data >> 4) & 7;
					int amplitude_mod_shift = p_data & 3;

					if (p_data & 128) {
						set_pitch_modulation(pitch_mod_shift < 6 ? (_pmd >> (6 - pitch_mod_shift)) : (_pmd << (pitch_mod_shift - 5)));
					} else {
						set_amplitude_modulation(amplitude_mod_shift > 0 ? (_amd << (amplitude_mod_shift - 1)) : 0);
					}
				} break;
			}
		} else { // Operator parameter
			int ops[4] = { 0, 2, 1, 3 }; // NOTE: This is the opposite of SiOPMChannelParams.
			int op_index = ops[(p_address >> 3) & 3];
			SiOPMOperator *op = _operators[op_index];

			switch ((p_address - 0x40) >> 5) {
				case 0: { // DT1:6-4 MUL:3-0
					op->set_dt1((p_data >> 4) & 7);
					op->set_multiple(p_data & 15);
				} break;
				case 1: { // TL:6-0
					op->set_total_level(p_data & 127);
				} break;
				case 2: { // KS:76 AR:4-0
					op->set_key_scaling_rate((p_data >> 6) & 3);
					op->set_attack_rate((p_data & 31) << 1);
				} break;
				case 3: { // AMS:7 DR:4-0
					op->set_amplitude_modulation_shift(((p_data >> 7) & 1) << 1);
					op->set_decay_rate((p_data & 31) << 1);
				} break;
				case 4: { // DT2:76 SR:4-0
					int options[4] = { 0, 384, 500, 608 };
					op->set_detune(options[(p_data >> 6) & 3]);
					op->set_sustain_rate((p_data & 31) << 1);
				} break;
				case 5: { // SL:7-4 RR:3-0
					op->set_sustain_level((p_data >> 4) & 15);
					op->set_release_rate((p_data & 15) << 2);
				} break;
			}
		}
	}
}

void SiOPMChannelFM::set_register(int p_address, int p_data) {
	switch (_register_map_type) {
		case REGISTER_OPM: {
			_set_by_opm_register(p_address, p_data);
		} break;

		default: break;
	}
}

void SiOPMChannelFM::_set_algorithm_operator1(int p_algorithm) {
	_update_operator_count(1);
	_algorithm = p_algorithm;

	_operators[0]->set_pipes(_pipe0, nullptr, true);
}

void SiOPMChannelFM::_set_algorithm_operator2(int p_algorithm) {
	_update_operator_count(2);
	_algorithm = p_algorithm;

	switch (_algorithm) {
		case 0: // OPL3/MA3:con=0, OPX:con=0, 1(fbc=1)
			// o1(o0)
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 1: // OPL3/MA3:con=1, OPX:con=2
			// o0+o1
			_operators[0]->set_pipes(_pipe0, nullptr, true);
			_operators[1]->set_pipes(_pipe0, nullptr, true);
			break;
		case 2: // OPX:con=3
			// o0+o1(o0)
			_operators[0]->set_pipes(_pipe0, nullptr,   true);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			_operators[1]->set_base_pipe(_pipe0);
			break;
		default:
			// o0+o1
			_operators[0]->set_pipes(_pipe0, nullptr, true);
			_operators[1]->set_pipes(_pipe0, nullptr, true);
			break;
	}
}

void SiOPMChannelFM::_set_algorithm_operator3(int p_algorithm) {
	_update_operator_count(3);
	_algorithm = p_algorithm;

	switch (_algorithm) {
		case 0: // OPX:con=0, 1(fbc=1)
			// o2(o1(o0))
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0);
			_operators[2]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 1: // OPX:con=2
			// o2(o0+o1)
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0);
			_operators[2]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 2: // OPX:con=3
			// o0+o2(o1)
			_operators[0]->set_pipes(_pipe0, nullptr,   true);
			_operators[1]->set_pipes(_pipe1);
			_operators[2]->set_pipes(_pipe0, _pipe1, true);
			break;
		case 3: // OPX:con=4, 5(fbc=1)
			// o1(o0)+o2
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			_operators[2]->set_pipes(_pipe0, nullptr,   true);
			break;
		case 4:
			// o1(o0)+o2(o0)
			_operators[0]->set_pipes(_pipe1);
			_operators[1]->set_pipes(_pipe0, _pipe1, true);
			_operators[2]->set_pipes(_pipe0, _pipe1, true);
			break;
		case 5: // OPX:con=6
			// o0+o1+o2
			_operators[0]->set_pipes(_pipe0, nullptr, true);
			_operators[1]->set_pipes(_pipe0, nullptr, true);
			_operators[2]->set_pipes(_pipe0, nullptr, true);
			break;
		case 6: // OPX:con=7
			// o0+o1(o0)+o2
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			_operators[1]->set_base_pipe(_pipe0);
			_operators[2]->set_pipes(_pipe0, nullptr,   true);
			break;
		default:
			// o0+o1+o2
			_operators[0]->set_pipes(_pipe0, nullptr, true);
			_operators[1]->set_pipes(_pipe0, nullptr, true);
			_operators[2]->set_pipes(_pipe0, nullptr, true);
			break;
	}
}

void SiOPMChannelFM::_set_algorithm_operator4(int p_algorithm) {
	_update_operator_count(4);
	_algorithm = p_algorithm;

	switch (_algorithm) {
		case 0: // OPL3:con=0, MA3:con=4, OPX:con=0, 1(fbc=1)
			// o3(o2(o1(o0)))
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0);
			_operators[2]->set_pipes(_pipe0, _pipe0);
			_operators[3]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 1: // OPX:con=2
			// o3(o2(o0+o1))
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0);
			_operators[2]->set_pipes(_pipe0, _pipe0);
			_operators[3]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 2: // MA3:con=3, OPX:con=3
			// o3(o0+o2(o1))
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe1);
			_operators[2]->set_pipes(_pipe0, _pipe1);
			_operators[3]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 3: // OPX:con=4, 5(fbc=1)
			// o3(o1(o0)+o2)
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0);
			_operators[2]->set_pipes(_pipe0);
			_operators[3]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 4: // OPL3:con=1, MA3:con=5, OPX:con=6, 7(fbc=1)
			// o1(o0)+o3(o2)
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			_operators[2]->set_pipes(_pipe1);
			_operators[3]->set_pipes(_pipe0, _pipe1, true);
			break;
		case 5: // OPX:con=12
			// o1(o0)+o2(o0)+o3(o0)
			_operators[0]->set_pipes(_pipe1);
			_operators[1]->set_pipes(_pipe0, _pipe1, true);
			_operators[2]->set_pipes(_pipe0, _pipe1, true);
			_operators[3]->set_pipes(_pipe0, _pipe1, true);
			break;
		case 6: // OPX:con=10, 11(fbc=1)
			// o1(o0)+o2+o3
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			_operators[2]->set_pipes(_pipe0, nullptr,   true);
			_operators[3]->set_pipes(_pipe0, nullptr,   true);
			break;
		case 7: // MA3:con=2, OPX:con=15
			// o0+o1+o2+o3
			_operators[0]->set_pipes(_pipe0, nullptr, true);
			_operators[1]->set_pipes(_pipe0, nullptr, true);
			_operators[2]->set_pipes(_pipe0, nullptr, true);
			_operators[3]->set_pipes(_pipe0, nullptr, true);
			break;
		case 8: // OPL3:con=2, MA3:con=6, OPX:con=8
			// o0+o3(o2(o1))
			_operators[0]->set_pipes(_pipe0, nullptr,   true);
			_operators[1]->set_pipes(_pipe1);
			_operators[2]->set_pipes(_pipe1, _pipe1);
			_operators[3]->set_pipes(_pipe0, _pipe1, true);
			break;
		case 9: // OPL3:con=3, MA3:con=7, OPX:con=13
			// o0+o2(o1)+o3
			_operators[0]->set_pipes(_pipe0, nullptr,   true);
			_operators[1]->set_pipes(_pipe1);
			_operators[2]->set_pipes(_pipe0, _pipe1, true);
			_operators[3]->set_pipes(_pipe0, nullptr,   true);
			break;
		case 10: // for DX7 emulation
			// o3(o0+o1+o2)
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0);
			_operators[2]->set_pipes(_pipe0);
			_operators[3]->set_pipes(_pipe0, _pipe0, true);
			break;
		case 11: // OPX:con=9
			// o0+o3(o1+o2)
			_operators[0]->set_pipes(_pipe0, nullptr,   true);
			_operators[1]->set_pipes(_pipe1);
			_operators[2]->set_pipes(_pipe1);
			_operators[3]->set_pipes(_pipe0, _pipe1, true);
			break;
		case 12: // OPX:con=14
			// o0+o1(o0)+o3(o2)
			_operators[0]->set_pipes(_pipe0);
			_operators[1]->set_pipes(_pipe0, _pipe0, true);
			_operators[1]->set_base_pipe(_pipe0);
			_operators[2]->set_pipes(_pipe1);
			_operators[3]->set_pipes(_pipe0, _pipe1, true);
			break;
		default:
			// o0+o1+o2+o3
			_operators[0]->set_pipes(_pipe0, nullptr, true);
			_operators[1]->set_pipes(_pipe0, nullptr, true);
			_operators[2]->set_pipes(_pipe0, nullptr, true);
			_operators[3]->set_pipes(_pipe0, nullptr, true);
			break;
	}
}

void SiOPMChannelFM::_set_algorithm_analog_like(int p_algorithm) {
	_update_operator_count(2);
	_operators[0]->set_pipes(_pipe0, nullptr, true);
	_operators[1]->set_pipes(_pipe0, nullptr, true);

	_algorithm = (p_algorithm >= 0 && p_algorithm <= 3) ? p_algorithm : 0;
	_process_function_type = (ProcessType)(PROCESS_ANALOG_LIKE + _algorithm);
	_update_process_function();
}

void SiOPMChannelFM::set_algorithm(int p_operator_count, int p_algorithm) {
	switch (p_operator_count) {
		case 2:
			_set_algorithm_operator2(p_algorithm);
			break;
		case 3:
			_set_algorithm_operator3(p_algorithm);
			break;
		case 4:
			_set_algorithm_operator4(p_algorithm);
			break;
		case 5:
			_set_algorithm_analog_like(p_algorithm);
			break;
		default:
			_set_algorithm_operator1(p_algorithm);
			break;
	}
}

void SiOPMChannelFM::set_feedback(int p_level, int p_connection) {
	if (p_level > 0) {
		// Connect the feedback pipe.
		if (p_connection < 0 || p_connection >= _operator_count) {
			p_connection = 0;
		}

		_in_pipe = _operators[p_connection]->get_feed_pipe();
		_in_pipe->value = 0;
		_input_level = p_level + 6;
		_input_mode = INPUT_FEEDBACK;
	} else {
		// Disable feedback.
		_in_pipe = _chip->get_zero_buffer();
		_input_level = 0;
		_input_mode = INPUT_ZERO;
	}
}

void SiOPMChannelFM::set_parameters(Vector<int> p_params) {
	set_params_by_value(
			p_params[1],  p_params[2],  p_params[3],  p_params[4],  p_params[5],
			p_params[6],  p_params[7],  p_params[8],  p_params[9],  p_params[10],
			p_params[11], p_params[12], p_params[13], p_params[14]
	);
}

void SiOPMChannelFM::set_types(int p_pg_type, int p_pt_type) {
	if (p_pg_type >= SiOPMTable::PG_PCM) {
		SiOPMWavePCMTable *pcm_table = _table->get_pcm_data(p_pg_type - SiOPMTable::PG_PCM);
		if (pcm_table) {
			set_wave_data(pcm_table);
		}
	} else {
		_active_operator->set_pulse_generator_type(p_pg_type);
		_active_operator->set_pitch_table_type(p_pt_type);
		_update_process_function();
	}
}

void SiOPMChannelFM::set_all_attack_rate(int p_value) {
	for (int i = 0; i < _operator_count; i++) {
		SiOPMOperator *op = _operators[i];
		if (op->is_final()) {
			op->set_attack_rate(p_value);
		}
	}
}

void SiOPMChannelFM::set_all_release_rate(int p_value) {
	for (int i = 0; i < _operator_count; i++) {
		SiOPMOperator *op = _operators[i];
		if (op->is_final()) {
			op->set_release_rate(p_value);
		}
	}
}

int SiOPMChannelFM::get_pitch() const {
	return _operators[_operator_count - 1]->get_pitch_index();
}

void SiOPMChannelFM::set_pitch(int p_value) {
	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->set_pitch_index(p_value);
	}
}

void SiOPMChannelFM::set_active_operator_index(int p_value) {
	int index = CLAMP(p_value, 0, _operator_count - 1);
	_active_operator = _operators[index];
}

void SiOPMChannelFM::set_release_rate(int p_value) {
	_active_operator->set_release_rate(p_value);
}

void SiOPMChannelFM::set_total_level(int p_value) {
	_active_operator->set_total_level(p_value);
}

void SiOPMChannelFM::set_fine_multiple(int p_value) {
	_active_operator->set_fine_multiple(p_value);
}

void SiOPMChannelFM::set_phase(int p_value) {
	_active_operator->set_key_on_phase(p_value);
}

void SiOPMChannelFM::set_detune(int p_value) {
	_active_operator->set_detune(p_value);
}

void SiOPMChannelFM::set_fixed_pitch(int p_value) {
	_active_operator->set_fixed_pitch_index(p_value);
}

void SiOPMChannelFM::set_ssg_envelope_control(int p_value) {
	_active_operator->set_ssg_type_envelope_control(p_value);
}

void SiOPMChannelFM::set_envelope_reset(bool p_reset) {
	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->set_envelope_reset_on_attack(p_reset);
	}
}


// Volume control.

void SiOPMChannelFM::offset_volume(int p_expression, int p_velocity) {
	int expression_index = p_expression << 1;
	int offset = _expression_table[expression_index] + _velocity_table[p_velocity];

	for (int i = 0; i < _operator_count; i++) {
		SiOPMOperator *op = _operators[i];

		if (op->is_final()) {
			op->offset_total_level(offset);
		} else {
			op->offset_total_level(0);
		}
	}
}

// LFO control.

void SiOPMChannelFM::_set_lfo_state(bool p_enabled) {
	_lfo_on = (int)p_enabled;
	_update_process_function();

	_lfo_timer_step = p_enabled ? _lfo_timer_step_buffer : 0;
}

void SiOPMChannelFM::_set_lfo_timer(int p_value) {
	_lfo_timer = (p_value > 0 ? 1 : 0);
	_lfo_timer_step = p_value;
	_lfo_timer_step_buffer = p_value;
}

void SiOPMChannelFM::set_frequency_ratio(int p_ratio) {
	_frequency_ratio = p_ratio;

	double value_coef = (p_ratio != 0) ? (100.0 / p_ratio) : 1.0;
	_eg_timer_initial = (int)(SiOPMTable::ENV_TIMER_INITIAL * value_coef);
	_lfo_timer_initial = (int)(SiOPMTable::LFO_TIMER_INITIAL * value_coef);
}

void SiOPMChannelFM::initialize_lfo(int p_waveform, Vector<int> p_custom_wave_table) {
	SiOPMChannelBase::initialize_lfo(p_waveform, p_custom_wave_table);

	_set_lfo_state(false);

	_amplitude_modulation_depth = 0;
	_pitch_modulation_depth = 0;
	_amplitude_modulation_output_level = 0;
	_pitch_modulation_output_level = 0;

	if (_operators[0]) {
		_operators[0]->set_detune2(0);
	}
	if (_operators[1]) {
		_operators[1]->set_detune2(0);
	}
	if (_operators[2]) {
		_operators[2]->set_detune2(0);
	}
	if (_operators[3]) {
		_operators[3]->set_detune2(0);
	}
}

void SiOPMChannelFM::set_amplitude_modulation(int p_depth) {
	_amplitude_modulation_depth = p_depth<<2;
	_amplitude_modulation_output_level = (_lfo_wave_table[_lfo_phase] * _amplitude_modulation_depth) >> 7 << 3;

	_set_lfo_state(_pitch_modulation_depth != 0 || _amplitude_modulation_depth > 0);
}

void SiOPMChannelFM::set_pitch_modulation(int p_depth) {
	_pitch_modulation_depth = p_depth;
	_pitch_modulation_output_level = (((_lfo_wave_table[_lfo_phase] << 1) - 255) * _pitch_modulation_depth) >> 8;

	_set_lfo_state(_pitch_modulation_depth != 0 || _amplitude_modulation_depth > 0);

	if (_pitch_modulation_depth == 0) {
		if (_operators[0]) {
			_operators[0]->set_detune2(0);
		}
		if (_operators[1]) {
			_operators[1]->set_detune2(0);
		}
		if (_operators[2]) {
			_operators[2]->set_detune2(0);
		}
		if (_operators[3]) {
			_operators[3]->set_detune2(0);
		}
	}
}

// Processing.

void SiOPMChannelFM::_update_lfo(int p_op_count) {
	_lfo_timer -= _lfo_timer_step;
	if (_lfo_timer >= 0) {
		return;
	}

	_lfo_phase = (_lfo_phase + 1) & 255;

	int value_base = _lfo_wave_table[_lfo_phase];
	_amplitude_modulation_output_level = (value_base * _amplitude_modulation_depth) >> 7 << 3;
	_pitch_modulation_output_level = (((value_base << 1) - 255) * _pitch_modulation_depth) >> 8;

	if (p_op_count > 0 && _operators[0]) {
		_operators[0]->set_detune2(_pitch_modulation_output_level);
	}
	if (p_op_count > 1 && _operators[1]) {
		_operators[1]->set_detune2(_pitch_modulation_output_level);
	}
	if (p_op_count > 2 && _operators[2]) {
		_operators[2]->set_detune2(_pitch_modulation_output_level);
	}
	if (p_op_count > 3 && _operators[3]) {
		_operators[3]->set_detune2(_pitch_modulation_output_level);
	}

	_lfo_timer += _lfo_timer_initial;
}

void SiOPMChannelFM::_process_operator1_lfo_off(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);

		// Update PG.
		{
			ope0->tick_pulse_generator();
			int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();

			int log_idx = ope0->get_wave_value(t);
			log_idx += ope0->get_eg_output();

			output = _table->log_table[log_idx];
			ope0->get_feed_pipe()->value = output;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;

			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_operator1_lfo_on(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update LFO.
		_update_lfo(1);

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);

		// Update PG.
		{
			ope0->tick_pulse_generator();
			int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();

			int log_idx = ope0->get_wave_value(t);
			log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());

			output = _table->log_table[log_idx];
			ope0->get_feed_pipe()->value = output;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_operator2(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];
	SiOPMOperator *ope1 = _operators[1];

	for (int i = 0; i < p_length; i++) {
		// Clear pipes.
		_pipe0->value = 0;

		// Update LFO.
		_update_lfo(2);

		// Operator 0.
		{
			// Update EG.
			ope0->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope0->tick_pulse_generator();
				int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();

				int log_idx = ope0->get_wave_value(t);
				log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope0->get_feed_pipe()->value = output;
				ope0->get_out_pipe()->value  = output + ope0->get_base_pipe()->value;
			}
		}

		// Operator 1.
		{
			// Update EG.
			ope1->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope1->tick_pulse_generator();
				int t = ((ope1->get_phase() + (ope1->get_in_pipe()->value << ope1->get_fm_shift())) & SiOPMTable::PHASE_FILTER) >> ope1->get_wave_fixed_bits();

				int log_idx = ope1->get_wave_value(t);
				log_idx += ope1->get_eg_output() + (_amplitude_modulation_output_level >> ope1->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope1->get_feed_pipe()->value = output;
				ope1->get_out_pipe()->value  = output + ope1->get_base_pipe()->value;
			}
		}

		// Output and increment pointers.
		{
			out_pipe->value = _pipe0->value + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;


}

void SiOPMChannelFM::_process_operator3(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];
	SiOPMOperator *ope1 = _operators[1];
	SiOPMOperator *ope2 = _operators[2];

	for (int i = 0; i < p_length; i++) {
		// Clear pipes.
		_pipe0->value = 0;
		_pipe1->value = 0;

		// Update LFO.
		_update_lfo(3);

		// Operator 0.
		{
			// Update EG.
			ope0->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope0->tick_pulse_generator();
				int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();

				int log_idx = ope0->get_wave_value(t);
				log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope0->get_feed_pipe()->value = output;
				ope0->get_out_pipe()->value  = output + ope0->get_base_pipe()->value;
			}
		}

		// Operator 1.
		{
			// Update EG.
			ope1->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope1->tick_pulse_generator();
				int t = ((ope1->get_phase() + (ope1->get_in_pipe()->value << ope1->get_fm_shift())) & SiOPMTable::PHASE_FILTER) >> ope1->get_wave_fixed_bits();

				int log_idx = ope1->get_wave_value(t);
				log_idx += ope1->get_eg_output() + (_amplitude_modulation_output_level >> ope1->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope1->get_feed_pipe()->value = output;
				ope1->get_out_pipe()->value  = output + ope1->get_base_pipe()->value;
			}
		}

		// Operator 2.
		{
			// Update EG.
			ope2->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope2->tick_pulse_generator();
				int t = ((ope2->get_phase() + (ope2->get_in_pipe()->value << ope2->get_fm_shift())) & SiOPMTable::PHASE_FILTER) >> ope2->get_wave_fixed_bits();

				int log_idx = ope2->get_wave_value(t);
				log_idx += ope2->get_eg_output() + (_amplitude_modulation_output_level >> ope2->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope2->get_feed_pipe()->value = output;
				ope2->get_out_pipe()->value  = output + ope2->get_base_pipe()->value;
			}
		}


		// Output and increment pointers.
		{
			out_pipe->value = _pipe0->value + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_operator4(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];
	SiOPMOperator *ope1 = _operators[1];
	SiOPMOperator *ope2 = _operators[2];
	SiOPMOperator *ope3 = _operators[3];

	for (int i = 0; i < p_length; i++) {
		// Clear pipes.
		_pipe0->value = 0;
		_pipe1->value = 0;

		// Update LFO.
		_update_lfo(4);

		// Operator 0.
		{
			// Update EG.
			ope0->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope0->tick_pulse_generator();
				int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();

				int log_idx = ope0->get_wave_value(t);
				log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope0->get_feed_pipe()->value = output;
				ope0->get_out_pipe()->value  = output + ope0->get_base_pipe()->value;
			}
		}

		// Operator 1.
		{
			// Update EG.
			ope1->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope1->tick_pulse_generator();
				int t = ((ope1->get_phase() + (ope1->get_in_pipe()->value << ope1->get_fm_shift())) & SiOPMTable::PHASE_FILTER) >> ope1->get_wave_fixed_bits();

				int log_idx = ope1->get_wave_value(t);
				log_idx += ope1->get_eg_output() + (_amplitude_modulation_output_level >> ope1->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope1->get_feed_pipe()->value = output;
				ope1->get_out_pipe()->value  = output + ope1->get_base_pipe()->value;
			}
		}

		// Operator 2.
		{
			// Update EG.
			ope2->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope2->tick_pulse_generator();
				int t = ((ope2->get_phase() + (ope2->get_in_pipe()->value << ope2->get_fm_shift())) & SiOPMTable::PHASE_FILTER) >> ope2->get_wave_fixed_bits();

				int log_idx = ope2->get_wave_value(t);
				log_idx += ope2->get_eg_output() + (_amplitude_modulation_output_level >> ope2->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope2->get_feed_pipe()->value = output;
				ope2->get_out_pipe()->value  = output + ope2->get_base_pipe()->value;
			}
		}

		// Operator 3.
		{
			// Update EG.
			ope3->tick_eg(_eg_timer_initial);

			// Update PG.
			{
				ope3->tick_pulse_generator();
				int t = ((ope3->get_phase() + (ope3->get_in_pipe()->value << ope3->get_fm_shift())) & SiOPMTable::PHASE_FILTER) >> ope3->get_wave_fixed_bits();

				int log_idx = ope3->get_wave_value(t);
				log_idx += ope3->get_eg_output() + (_amplitude_modulation_output_level >> ope3->get_amplitude_modulation_shift());
				int output = _table->log_table[log_idx];

				ope3->get_feed_pipe()->value = output;
				ope3->get_out_pipe()->value  = output + ope3->get_base_pipe()->value;
			}

		}

		// Output and increment pointers.
		{
			out_pipe->value = _pipe0->value + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_pcm_lfo_off(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);

		// Update PG.
		{
			ope0->tick_pulse_generator();
			int t = (ope0->get_phase() + (in_pipe->value << _input_level)) >> ope0->get_wave_fixed_bits();

			if (t >= ope0->get_pcm_end_point()) {
				if (ope0->get_pcm_loop_point() == -1) {
					ope0->set_eg_state(SiOPMOperator::EG_OFF);
					ope0->update_eg_output();

					// Fast forward.
					for (; i < p_length; i++) {
						out_pipe->value = base_pipe->value;
						in_pipe = in_pipe->next;
						base_pipe = base_pipe->next;
						out_pipe = out_pipe->next;
					}
					break;
				} else {
					t -= ope0->get_pcm_end_point() - ope0->get_pcm_loop_point();
					int phase_diff = ((ope0->get_pcm_end_point() - ope0->get_pcm_loop_point()) << ope0->get_wave_fixed_bits());
					ope0->adjust_phase(-phase_diff);
				}
			}

			int log_idx = ope0->get_wave_value(t);
			log_idx += ope0->get_eg_output();
			output = _table->log_table[log_idx];

			ope0->get_feed_pipe()->value = output;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_pcm_lfo_on(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update LFO.
		_update_lfo(1);

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);

		// Update PG.
		{
			ope0->tick_pulse_generator();
			int t = (ope0->get_phase() + (in_pipe->value<<_input_level)) >> ope0->get_wave_fixed_bits();

			if (t >= ope0->get_pcm_end_point()) {
				if (ope0->get_pcm_loop_point() == -1) {
					ope0->set_eg_state(SiOPMOperator::EG_OFF);
					ope0->update_eg_output();

					// Fast forward.
					for (; i < p_length; i++) {
						out_pipe->value = base_pipe->value;
						in_pipe = in_pipe->next;
						base_pipe = base_pipe->next;
						out_pipe = out_pipe->next;
					}
					break;
				} else {
					t -=  ope0->get_pcm_end_point() - ope0->get_pcm_loop_point();
					int phase_diff = ((ope0->get_pcm_end_point() - ope0->get_pcm_loop_point()) << ope0->get_wave_fixed_bits());
					ope0->adjust_phase(-phase_diff);
				}
			}

			int log_idx = ope0->get_wave_value(t);
			log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level>>ope0->get_amplitude_modulation_shift());
			output = _table->log_table[log_idx];

			ope0->get_feed_pipe()->value = output;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_analog_like(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];
	SiOPMOperator *ope1 = _operators[1];

	for (int i = 0; i < p_length; i++) {
		int output0 = 0;
		int output1 = 0;

		// Update LFO.
		_update_lfo(2);

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);
		ope1->update_eg_output_from(ope0);

		// Update PG.
		{
			// Operator 0.
			{
				ope0->tick_pulse_generator();
				int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();

				int log_idx = ope0->get_wave_value(t);
				log_idx += ope0->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				output0 = _table->log_table[log_idx];
			}

			// Operator 1 (w/ operator0's envelope and AMS).
			{
				ope1->tick_pulse_generator();
				int t = (ope1->get_phase() & SiOPMTable::PHASE_FILTER) >> ope1->get_wave_fixed_bits();

				int log_idx = ope1->get_wave_value(t);
				log_idx += ope1->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				output1 = _table->log_table[log_idx];
			}

			ope0->get_feed_pipe()->value = output0;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output0 + output1 + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_ring(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];
	SiOPMOperator *ope1 = _operators[1];

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update LFO.
		_update_lfo(2);

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);
		ope1->update_eg_output_from(ope0);

		// Update PG.
		{
			int log_idx = 0;

			// Operator 0.
			{
				ope0->tick_pulse_generator();
				int t = ((ope0->get_phase() + (in_pipe->value << _input_level)) & SiOPMTable::PHASE_FILTER) >> ope0->get_wave_fixed_bits();
				log_idx = ope0->get_wave_value(t);
			}

			// Operator 1 (w/ operator0's envelope and AMS).
			{
				ope1->tick_pulse_generator();
				int t = (ope1->get_phase() & SiOPMTable::PHASE_FILTER) >> ope1->get_wave_fixed_bits();

				log_idx += ope1->get_wave_value(t);
				log_idx += ope1->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				output = _table->log_table[log_idx];
			}

			ope0->get_feed_pipe()->value = output;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::_process_sync(int p_length) {
	SinglyLinkedList<int> *in_pipe   = _in_pipe;
	SinglyLinkedList<int> *base_pipe = _base_pipe;
	SinglyLinkedList<int> *out_pipe  = _out_pipe;

	SiOPMOperator *ope0 = _operators[0];
	SiOPMOperator *ope1 = _operators[1];

	for (int i = 0; i < p_length; i++) {
		int output = 0;

		// Update LFO.
		_update_lfo(2);

		// Update EG.
		ope0->tick_eg(_eg_timer_initial);
		ope1->update_eg_output_from(ope0);

		// Update PG.
		{
			// Operator 0.
			{
				ope0->tick_pulse_generator(in_pipe->value << _input_level);
				if (ope0->get_phase() & SiOPMTable::PHASE_MAX) {
					ope1->set_phase(ope1->get_key_on_phase_raw());
				}

				ope0->set_phase(ope0->get_phase() & SiOPMTable::PHASE_FILTER);
			}

			// Operator 1 (w/ operator0's envelope and AMS).
			{
				ope1->tick_pulse_generator();
				int t = (ope1->get_phase() & SiOPMTable::PHASE_FILTER) >> ope1->get_wave_fixed_bits();

				int log_idx = ope1->get_wave_value(t);
				log_idx += ope1->get_eg_output() + (_amplitude_modulation_output_level >> ope0->get_amplitude_modulation_shift());
				output = _table->log_table[log_idx];
			}

			ope0->get_feed_pipe()->value = output;
		}

		// Output and increment pointers.
		{
			out_pipe->value = output + base_pipe->value;
			in_pipe = in_pipe->next;
			base_pipe = base_pipe->next;
			out_pipe = out_pipe->next;
		}
	}

	_in_pipe = in_pipe;
	_base_pipe = base_pipe;
	_out_pipe = out_pipe;
}

void SiOPMChannelFM::note_on() {
	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->note_on();
	}

	_is_note_on = true;
	_is_idling = false;
	SiOPMChannelBase::note_on();
}

void SiOPMChannelFM::note_off() {
	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->note_off();
	}

	_is_note_on = false;
	SiOPMChannelBase::note_off();
}

void SiOPMChannelFM::reset_channel_buffer_status() {
	_buffer_index = 0;
	_is_idling = true;

	for (int i = 0; i < _operator_count; i++) {
		SiOPMOperator *op = _operators[i];

		if (op->is_final() && (op->get_eg_output() < IDLING_THRESHOLD || op->get_eg_state() == SiOPMOperator::EG_ATTACK)) {
			_is_idling = false;
			break;
		}
	}
}

//

String SiOPMChannelFM::to_string() const {
	String str = "SiOPMChannelFM : operatorCount=";

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

void SiOPMChannelFM::initialize(SiOPMChannelBase *p_prev, int p_buffer_index) {
	_update_operator_count(1);
	_operators[0]->initialize();

	_is_note_on = false;
	SiOPMChannelBase::initialize(p_prev, p_buffer_index);
}

void SiOPMChannelFM::reset() {
	for (int i = 0; i < _operator_count; i++) {
		_operators[i]->reset();
	}

	_is_note_on = false;
	_is_idling = true;
}

void SiOPMChannelFM::_bind_methods() {
	// To be used as callables.
	ClassDB::bind_method(D_METHOD("_process_operator1_lfo_off", "length"), &SiOPMChannelFM::_process_operator1_lfo_off);
	ClassDB::bind_method(D_METHOD("_process_operator1_lfo_on", "length"),  &SiOPMChannelFM::_process_operator1_lfo_on);
	ClassDB::bind_method(D_METHOD("_process_operator2", "length"),         &SiOPMChannelFM::_process_operator2);
	ClassDB::bind_method(D_METHOD("_process_operator3", "length"),         &SiOPMChannelFM::_process_operator3);
	ClassDB::bind_method(D_METHOD("_process_operator4", "length"),         &SiOPMChannelFM::_process_operator4);
	ClassDB::bind_method(D_METHOD("_process_pcm_lfo_off", "length"),       &SiOPMChannelFM::_process_pcm_lfo_off);
	ClassDB::bind_method(D_METHOD("_process_pcm_lfo_on", "length"),        &SiOPMChannelFM::_process_pcm_lfo_on);
	ClassDB::bind_method(D_METHOD("_process_analog_like", "length"),       &SiOPMChannelFM::_process_analog_like);
	ClassDB::bind_method(D_METHOD("_process_ring", "length"),              &SiOPMChannelFM::_process_ring);
	ClassDB::bind_method(D_METHOD("_process_sync", "length"),              &SiOPMChannelFM::_process_sync);
}

SiOPMChannelFM::SiOPMChannelFM(SiOPMModule *p_chip) : SiOPMChannelBase(p_chip) {
	_process_function_list = {
		{
			Callable(this, "_process_operator1_lfo_off"),
			Callable(this, "_process_operator2"),
			Callable(this, "_process_operator3"),
			Callable(this, "_process_operator4"),
			Callable(this, "_process_analog_like"),
			Callable(this, "_process_ring"),
			Callable(this, "_process_sync"),
			Callable(this, "_process_operator2"),
			Callable(this, "_process_pcm_lfo_off")
		},
		{
			Callable(this, "_process_operator1_lfo_on"),
			Callable(this, "_process_operator2"),
			Callable(this, "_process_operator3"),
			Callable(this, "_process_operator4"),
			Callable(this, "_process_analog_like"),
			Callable(this, "_process_ring"),
			Callable(this, "_process_sync"),
			Callable(this, "_process_operator2"),
			Callable(this, "_process_pcm_lfo_on")
		}
	};

	_operators.resize_zeroed(4);
	_operators.write[0] = _alloc_fm_operator();
	_operators.write[1] = nullptr;
	_operators.write[2] = nullptr;
	_operators.write[3] = nullptr;

	_active_operator = _operators[0];
	_operator_count = 1;

	_update_process_function();

	_pipe0 = SinglyLinkedList<int>::alloc_ring(1);
	_pipe1 = SinglyLinkedList<int>::alloc_ring(1);

	initialize(nullptr, 0);
}

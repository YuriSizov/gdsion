/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "translator_util.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "chip/siopm_channel_params.h"
#include "chip/siopm_operator_params.h"
#include "chip/siopm_ref_table.h"
#include "sequencer/simml_ref_table.h"
#include "utils/godot_util.h"

// Channel params.

Vector<int> TranslatorUtil::_split_data_string(SiOPMChannelParams *r_params, String p_data_string, int p_channel_param_count, int p_operator_param_count, const String &p_command) {
	if (p_data_string.is_empty()) {
		r_params->operator_count = 0;
		return Vector<int>();
	}

	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "gms". Behavioral implications require investigation.
	Ref<RegEx> re_command = RegEx::create_from_string("/\\*.*?\\*/|//.*?[\\r\\n]+");
	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "g". Behavioral implications require investigation.
	Ref<RegEx> re_cleanup = RegEx::create_from_string("^[^\\d\\-.]+|[^\\d\\-.]+$");

	String sanitized_string = re_command->sub(p_data_string, "", true);
	sanitized_string = re_cleanup->sub(sanitized_string, "", true);
	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "gm". Behavioral implications require investigation.
	PackedStringArray string_data = split_string_by_regex(sanitized_string, "[^\\d\\-.]+");

	for (int i = 1; i < 5; i++) {
		if (string_data.size() == (p_channel_param_count + p_operator_param_count * i)) {
			r_params->operator_count = i;

			Vector<int> data;
			data.resize_zeroed(string_data.size());
			for (int i = 0; i < string_data.size(); i++) {
				data.write[i] = string_data[i].to_int();
			}
			return data;
		}
	}

	ERR_FAIL_V_MSG(Vector<int>(), vformat("Translator: Invalid parameter count in '%s' (channel: %d, each operator: %d).", p_command, p_channel_param_count, p_operator_param_count));
}

void TranslatorUtil::_check_operator_count(SiOPMChannelParams *r_params, int p_data_length, int p_channel_param_count, int p_operator_param_count, const String &p_command) {
	int op_count = (p_data_length - p_channel_param_count) / p_operator_param_count;
	// FIXME: The second condition is seemingly impossible. Worth double checking.
	ERR_FAIL_COND_MSG((op_count > 4 || (op_count * p_operator_param_count + p_channel_param_count) != p_data_length), vformat("Translator: Invalid parameter count in '%s' (channel: %d, each operator: %d).", p_command, p_channel_param_count, p_operator_param_count));

	r_params->operator_count = op_count;
}

int TranslatorUtil::_get_params_algorithm(int (&p_algorithms)[4][16], int p_operator_count, int p_data_value, const String &p_command) {
	int alg_index = p_operator_count - 1;
	ERR_FAIL_INDEX_V_MSG(alg_index, 4, -1, vformat("Translator: Invalid algorithm parameter %d in '%s'.", p_data_value, p_command));

	int alg_data = p_data_value & 15;
	ERR_FAIL_INDEX_V_MSG(alg_data, 16, -1, vformat("Translator: Invalid algorithm parameter %d in '%s'.", p_data_value, p_command));

	int algorithm = p_algorithms[alg_index][alg_data];
	ERR_FAIL_COND_V_MSG(algorithm == -1, -1, vformat("Translator: Invalid algorithm parameter %d in '%s'.", p_data_value, p_command));

	return algorithm;
}

// #@
// alg[0-15], fb[0-7], fbc[0-3],
// (ws[0-511], ar[0-63], dr[0-63], sr[0-63], rr[0-63], sl[0-15], tl[0-127], ksr[0-3], ksl[0-3], mul[], dt1[0-7], detune[], ams[0-3], phase[-1-255], fixedNote[0-127]) x operator_count
void TranslatorUtil::_set_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	if (r_params->operator_count == 0) {
		return;
	}

	r_params->algorithm = p_data[0];
	r_params->feedback = p_data[1];
	r_params->feedback_connection = p_data[2];

	int data_index = 3;
	for (int op_index = 0; op_index < r_params->operator_count; op_index++) {
		SiOPMOperatorParams *op_params = r_params->operator_params[op_index];

		op_params->set_pulse_generator_type    (p_data[data_index++] & 511);       // 1
		op_params->attack_rate                = p_data[data_index++] & 63;         // 2
		op_params->decay_rate                 = p_data[data_index++] & 63;         // 3
		op_params->sustain_rate               = p_data[data_index++] & 63;         // 4
		op_params->release_rate               = p_data[data_index++] & 63;         // 5
		op_params->sustain_level              = p_data[data_index++] & 15;         // 6
		op_params->total_level                = p_data[data_index++] & 127;        // 7
		op_params->key_scaling_rate           = p_data[data_index++] & 3;          // 8
		op_params->key_scaling_level          = p_data[data_index++] & 3;          // 9

		// Note: Original code briefly converts this value to a Number type, which is
		// equivalent to double. However, it's unclear if this is intentional or not.
		// Fine multiple (fmul) is stored and set as an int in the class definition.
		int n = p_data[data_index++];
		op_params->fine_multiple = (n == 0) ? 64 : n * 128;                         // 10
		op_params->detune1                    = p_data[data_index++] & 7;           // 11
		op_params->detune                     = p_data[data_index++];               // 12
		op_params->amplitude_modulation_shift = p_data[data_index++] & 3;           // 13

		int i = p_data[data_index++];
		op_params->initial_phase = (i == -1) ? i : (i & 255);                       // 14
		op_params->fixed_pitch                = (p_data[data_index++] & 127) << 6;  // 15
	}
}

// #OPL@
// alg[0-5], fb[0-7],
// (ws[0-7], ar[0-15], dr[0-15], rr[0-15], egt[0,1], sl[0-15], tl[0-63], ksr[0,1], ksl[0-3], mul[0-15], ams[0-3]) x operator_count
void TranslatorUtil::_set_opl_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	if (r_params->operator_count == 0) {
		return;
	}

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opl, r_params->operator_count, p_data[0], "#OPL@");
	if (algorithm == -1) {
		return;
	}

	r_params->envelope_frequency_ratio = 133;
	r_params->algorithm = algorithm;
	r_params->feedback = p_data[1];

	int data_index = 2;
	for (int op_index = 0; op_index < r_params->operator_count; op_index++) {
		SiOPMOperatorParams *op_params = r_params->operator_params[op_index];

		op_params->set_pulse_generator_type(PG_MA3_WAVE + (p_data[data_index++] & 31));      // 1
		op_params->attack_rate                                      = (p_data[data_index++] << 2) & 63;  // 2
		op_params->decay_rate                                       = (p_data[data_index++] << 2) & 63;  // 3
		op_params->release_rate                                     = (p_data[data_index++] << 2) & 63;  // 4

		// egt=0;decaying tone / egt=1;holding tone
		int n = p_data[data_index++];
		op_params->sustain_rate = (n != 0) ? 0 : op_params->release_rate;                                // 5
		op_params->sustain_level                                     = p_data[data_index++] & 15;        // 6
		op_params->total_level                                       = p_data[data_index++] & 63;        // 7
		op_params->key_scaling_rate                                 = (p_data[data_index++] << 1) & 3;   // 8
		op_params->key_scaling_level                                 = p_data[data_index++] & 3;         // 9

		int i = p_data[data_index++] & 15;
		op_params->set_multiple((i == 11 || i == 13) ? (i - 1) : (i == 14) ? (i + 1) : i);               // 10
		op_params->amplitude_modulation_shift                        = p_data[data_index++] & 3;         // 11
	}
}

// #OPM@
// alg[0-7], fb[0-7],
// (ar[0-31], dr[0-31], sr[0-31], rr[0-15], sl[0-15], tl[0-127], ks[0-3], mul[0-15], dt1[0-7], dt2[0-3], ams[0-3]) x operator_count
void TranslatorUtil::_set_opm_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	if (r_params->operator_count == 0) {
		return;
	}

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opm, r_params->operator_count, p_data[0], "#OPM@");
	if (algorithm == -1) {
		return;
	}

	r_params->algorithm = algorithm;
	r_params->feedback = p_data[1];

	int data_index = 2;
	for (int op_index = 0; op_index < r_params->operator_count; op_index++) {
		SiOPMOperatorParams *op_params = r_params->operator_params[op_index];

		op_params->attack_rate               = (p_data[data_index++] << 1) & 63;       // 1
		op_params->decay_rate                = (p_data[data_index++] << 1) & 63;       // 2
		op_params->sustain_rate              = (p_data[data_index++] << 1) & 63;       // 3
		op_params->release_rate             = ((p_data[data_index++] << 2) + 2) & 63;  // 4
		op_params->sustain_level              = p_data[data_index++] & 15;             // 5
		op_params->total_level                = p_data[data_index++] & 127;            // 6
		op_params->key_scaling_rate           = p_data[data_index++] & 3;              // 7
		op_params->set_multiple                (p_data[data_index++] & 15);            // 8
		op_params->detune1                    = p_data[data_index++] & 7;              // 9

		int n = p_data[data_index++] & 3;
		op_params->detune = SiOPMRefTable::get_instance()->dt2_table[n];                  // 10
		op_params->amplitude_modulation_shift = p_data[data_index++] & 3;              // 11
	}
}

// #OPN@
// alg[0-7], fb[0-7],
// (ar[0-31], dr[0-31], sr[0-31], rr[0-15], sl[0-15], tl[0-127], ks[0-3], mul[0-15], dt1[0-7], ams[0-3]) x operator_count
void TranslatorUtil::_set_opn_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	if (r_params->operator_count == 0) {
		return;
	}

	// Note: OPM and OPN share the algo list.
	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opm, r_params->operator_count, p_data[0], "#OPN@");
	if (algorithm == -1) {
		return;
	}

	r_params->algorithm = algorithm;
	r_params->feedback = p_data[1];

	int data_index = 2;
	for (int op_index = 0; op_index < r_params->operator_count; op_index++) {
		SiOPMOperatorParams *op_params = r_params->operator_params[op_index];

		op_params->attack_rate               = (p_data[data_index++] << 1) & 63;       // 1
		op_params->decay_rate                = (p_data[data_index++] << 1) & 63;       // 2
		op_params->sustain_rate              = (p_data[data_index++] << 1) & 63;       // 3
		op_params->release_rate             = ((p_data[data_index++] << 2) + 2) & 63;  // 4
		op_params->sustain_level              = p_data[data_index++] & 15;             // 5
		op_params->total_level                = p_data[data_index++] & 127;            // 6
		op_params->key_scaling_rate           = p_data[data_index++] & 3;              // 7
		op_params->set_multiple                (p_data[data_index++] & 15);            // 8
		op_params->detune1                    = p_data[data_index++] & 7;              // 9
		op_params->amplitude_modulation_shift = p_data[data_index++] & 3;              // 10

	}
}

// #OPX@
// alg[0-15], fb[0-7],
// (ws[0-7], ar[0-31], dr[0-31], sr[0-31], rr[0-15], sl[0-15], tl[0-127], ks[0-3], mul[0-15], dt1[0-7], detune[], ams[0-3]) x operator_count
void TranslatorUtil::_set_opx_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	if (r_params->operator_count == 0) {
		return;
	}

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opx, r_params->operator_count, p_data[0], "#OPX@");
	if (algorithm == -1) {
		return;
	}

	r_params->algorithm = (algorithm & 15);
	r_params->feedback = p_data[1];
	r_params->feedback_connection = (algorithm & 16) ? 1 : 0;

	int data_index = 2;
	for (int op_index = 0; op_index < r_params->operator_count; op_index++) {
		SiOPMOperatorParams *op_params = r_params->operator_params[op_index];

		int i = p_data[data_index++];
		int i1 = PG_MA3_WAVE + (i & 7);
		int i2 = PG_CUSTOM + (i - 7);
		op_params->set_pulse_generator_type((i < 7) ? i1 : i2);                        // 1
		op_params->attack_rate               = (p_data[data_index++] << 1) & 63;       // 2
		op_params->decay_rate                = (p_data[data_index++] << 1) & 63;       // 3
		op_params->sustain_rate              = (p_data[data_index++] << 1) & 63;       // 4
		op_params->release_rate             = ((p_data[data_index++] << 2) + 2) & 63;  // 5
		op_params->sustain_level              = p_data[data_index++] & 15;             // 6
		op_params->total_level                = p_data[data_index++] & 127;            // 7
		op_params->key_scaling_rate           = p_data[data_index++] & 3;              // 8
		op_params->set_multiple                (p_data[data_index++] & 15);            // 9
		op_params->detune1                    = p_data[data_index++] & 7;              // 10
		op_params->detune                     = p_data[data_index++];                  // 11
		op_params->amplitude_modulation_shift = p_data[data_index++] & 3;              // 12
	}
}

// #MA@
// alg[0-15], fb[0-7],
// (ws[0-31], ar[0-15], dr[0-15], sr[0-15], rr[0-15], sl[0-15], tl[0-63], ksr[0,1], ksl[0-3], mul[0-15], dt1[0-7], ams[0-3]) x operator_count
void TranslatorUtil::_set_ma3_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	if (r_params->operator_count == 0) {
		return;
	}

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_ma3, r_params->operator_count, p_data[0], "#MA@");
	if (algorithm == -1) {
		return;
	}

	r_params->envelope_frequency_ratio = 133;
	r_params->algorithm = algorithm;
	r_params->feedback = p_data[1];

	int data_index = 2;
	for (int op_index = 0; op_index < r_params->operator_count; op_index++) {
		SiOPMOperatorParams *op_params = r_params->operator_params[op_index];

		int n = p_data[data_index++] & 31;
		op_params->set_pulse_generator_type(PG_MA3_WAVE + n);                   // 1
		op_params->attack_rate                       = (p_data[data_index++] << 2) & 63;    // 2
		op_params->decay_rate                        = (p_data[data_index++] << 2) & 63;    // 3
		op_params->sustain_rate                      = (p_data[data_index++] << 2) & 63;    // 4
		op_params->release_rate                      = (p_data[data_index++] << 2) & 63;    // 5
		op_params->sustain_level                      = p_data[data_index++] & 15;          // 6
		op_params->total_level                        = p_data[data_index++] & 63;          // 7
		op_params->key_scaling_rate                  = (p_data[data_index++]<<1) & 3;       // 8
		op_params->key_scaling_level                  = p_data[data_index++] & 3;           // 9

		int i = p_data[data_index++] & 15;
		op_params->set_multiple((i == 11 || i == 13) ? (i - 1) : (i == 14) ? (i + 1) : i);  // 10
		op_params->detune1                            = p_data[data_index++] & 7;           // 11
		op_params->amplitude_modulation_shift         = p_data[data_index++] & 3;           // 12
	}
}

// #AL@
// con[0-2], ws1[0-511], ws2[0-511], balance[-64-+64], vco2pitch[]
// ar[0-63], dr[0-63], sl[0-15], rr[0-63]
void TranslatorUtil::_set_al_params_by_array(SiOPMChannelParams *r_params, Vector<int> p_data) {
	r_params->operator_count = 5;

	int connection_type = p_data[0];
	r_params->algorithm = (connection_type >= 0 && connection_type <= 2) ? connection_type : 0;

	SiOPMOperatorParams *op_params0 = r_params->operator_params[0];
	SiOPMOperatorParams *op_params1 = r_params->operator_params[1];

	op_params0->set_pulse_generator_type(p_data[1]);
	op_params1->set_pulse_generator_type(p_data[2]);

	int balance = CLAMP(p_data[3], -64, 64);
	int (&tl_table)[129] = SiOPMRefTable::get_instance()->eg_linear_to_total_level_table;
	op_params0->total_level = tl_table[64 - balance];
	op_params1->total_level = tl_table[balance + 64];

	op_params0->detune = 0;
	op_params1->detune = p_data[4];

	op_params0->attack_rate   = p_data[5] & 63;
	op_params0->decay_rate    = p_data[6] & 63;
	op_params0->sustain_rate  = 0;
	op_params0->release_rate  = p_data[8] & 15;
	op_params0->sustain_level = p_data[7] & 63;
}

void TranslatorUtil::parse_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_params_by_array(r_params, _split_data_string(r_params, p_data_string, 3, 15, "#@"));
}

void TranslatorUtil::parse_opl_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_opl_params_by_array(r_params, _split_data_string(r_params, p_data_string, 2, 11, "#OPL@"));
}

void TranslatorUtil::parse_opm_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_opm_params_by_array(r_params, _split_data_string(r_params, p_data_string, 2, 11, "#OPM@"));
}

void TranslatorUtil::parse_opn_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_opn_params_by_array(r_params, _split_data_string(r_params, p_data_string, 2, 10, "#OPN@"));
}

void TranslatorUtil::parse_opx_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_opx_params_by_array(r_params, _split_data_string(r_params, p_data_string, 2, 12, "#OPX@"));
}

void TranslatorUtil::parse_ma3_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_ma3_params_by_array(r_params, _split_data_string(r_params, p_data_string, 2, 12, "#MA@"));
}

void TranslatorUtil::parse_al_params(SiOPMChannelParams *r_params, const String &p_data_string) {
	return _set_al_params_by_array(r_params, _split_data_string(r_params, p_data_string, 9, 0, "#AL@"));
}

void TranslatorUtil::set_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	_check_operator_count(r_params, p_data.size(), 3, 15, "#@");
	return _set_params_by_array(r_params, p_data);
}

void TranslatorUtil::set_opl_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	_check_operator_count(r_params, p_data.size(), 2, 11, "#OPL@");
	return _set_opl_params_by_array(r_params, p_data);
}

void TranslatorUtil::set_opm_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	_check_operator_count(r_params, p_data.size(), 2, 11, "#OPM@");
	return _set_opm_params_by_array(r_params, p_data);
}

void TranslatorUtil::set_opn_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	_check_operator_count(r_params, p_data.size(), 2, 10, "#OPN@");
	return _set_opn_params_by_array(r_params, p_data);
}

void TranslatorUtil::set_opx_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	_check_operator_count(r_params, p_data.size(), 2, 12, "#OPX@");
	return _set_opx_params_by_array(r_params, p_data);
}

void TranslatorUtil::set_ma3_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	_check_operator_count(r_params, p_data.size(), 2, 12, "#MA@");
	return _set_ma3_params_by_array(r_params, p_data);
}

void TranslatorUtil::set_al_params(SiOPMChannelParams *r_params, Vector<int> p_data) {
	ERR_FAIL_COND_MSG(p_data.size() != 9, vformat("Translator: Invalid parameter count in '%s' (channel: %d, each operator: %d).", "#AL@", 9, 0));

	return _set_al_params_by_array(r_params, p_data);
}

int TranslatorUtil::_get_algorithm_index(int p_operator_count, int p_algorithm, int (&p_table)[4][16], const String &p_command) {
	int alg_index = p_operator_count - 1;
	ERR_FAIL_INDEX_V_MSG(alg_index, 4, -1, vformat("Translator: Invalid operator count in the algorithm parameter 'opc%d/alg%d' in '%s'.", p_operator_count, p_algorithm, p_command));

	for (int i = 0; i < 16; i++) {
		if (p_algorithm == p_table[alg_index][i]) {
			return i;
		}
	}

	ERR_FAIL_V_MSG(-1, vformat("Translator: Invalid algorithm parameter 'opc%d/alg%d' in '%s'.", p_operator_count, p_algorithm, p_command));
}

int TranslatorUtil::_get_ma3_from_pg_type(int p_pulse_generator_type, const String &p_command) {
	int wave_shape = p_pulse_generator_type - PG_MA3_WAVE;
	if (wave_shape >= 0 && wave_shape <= 31) {
		return wave_shape;
	}

	switch (p_pulse_generator_type) {
		case 0:                               return 0;   // Sin
		case 1: case 2:   case 128: case 255: return 24;  // Saw
		case 4: case 192: case 191:           return 16;  // Triangle
		case 5: case 72:                      return 6;   // Square
	}


	ERR_FAIL_V_MSG(-1, vformat("Translator: Invalid parameter 'ws%d' in '%s'.", p_pulse_generator_type, p_command));
}

int TranslatorUtil::_get_nearest_dt2(int p_detune) {
	if (p_detune <= 100) {
		return 0;                  // 0
	} else if (p_detune <= 420) {
		return 1;                  // 384
	} else if (p_detune <= 550) {
		return 2;                  // 500
	}

	return 3;                      // 608
}

int TranslatorUtil::_balance_total_levels(int p_level0, int p_level1) {
	if (p_level0 == p_level1) {
		return 0;
	}
	if (p_level0 == 0) {
		return -64;
	}
	if (p_level1 == 0) {
		return 64;
	}

	int (&tl_table)[129] = SiOPMRefTable::get_instance()->eg_linear_to_total_level_table;
	for (int i = 1; i < 128; i++) {
		if (p_level0 >= tl_table[i]) {
			return i - 64;
		}
	}

	return 64;
}

Vector<int> TranslatorUtil::get_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	Vector<int> res = { p_params->algorithm, p_params->feedback, p_params->feedback_connection };

	for (int i = 0; i < p_params->operator_count; i++) {
		SiOPMOperatorParams *op_params = p_params->operator_params[i];
		res.append_array({
			op_params->pulse_generator_type,
			op_params->attack_rate,
			op_params->decay_rate,
			op_params->sustain_rate,
			op_params->release_rate,
			op_params->sustain_level,
			op_params->total_level,
			op_params->key_scaling_rate,
			op_params->key_scaling_level,
			op_params->get_multiple(),
			op_params->detune1,
			op_params->detune,
			op_params->amplitude_modulation_shift,
			op_params->initial_phase,
			op_params->fixed_pitch >> 6
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_opl_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opl, "#OPL@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		SiOPMOperatorParams *op_params = p_params->operator_params[i];

		int wave_shape = _get_ma3_from_pg_type(op_params->pulse_generator_type, "#OPL@");
		if (wave_shape == -1) {
			return Vector<int>();
		}

		int egt = op_params->sustain_rate == 0 ? 1 : 0; // Envelope generator t?..
		int total_level = op_params->total_level < 63 ? op_params->total_level : 63;

		res.append_array({
			wave_shape,
			op_params->attack_rate >> 2,
			op_params->decay_rate >> 2,
			op_params->release_rate >> 2,
			egt,
			op_params->sustain_level,
			total_level,
			op_params->key_scaling_rate >> 1,
			op_params->key_scaling_level,
			op_params->get_multiple(),
			op_params->amplitude_modulation_shift
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_opm_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opm, "#OPM@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		SiOPMOperatorParams *op_params = p_params->operator_params[i];

		int	detune2 = _get_nearest_dt2(op_params->detune);

		res.append_array({
			op_params->attack_rate >> 1,
			op_params->decay_rate >> 1,
			op_params->sustain_rate >> 1,
			op_params->release_rate >> 2,
			op_params->sustain_level,
			op_params->total_level,
			op_params->key_scaling_rate,
			op_params->get_multiple(),
			op_params->detune1,
			detune2,
			op_params->amplitude_modulation_shift
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_opn_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	// Note: OPM and OPN share the algo list.
	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opm, "#OPN@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		SiOPMOperatorParams *op_params = p_params->operator_params[i];

		res.append_array({
			op_params->attack_rate >> 1,
			op_params->decay_rate >> 1,
			op_params->sustain_rate >> 1,
			op_params->release_rate >> 2,
			op_params->sustain_level,
			op_params->total_level,
			op_params->key_scaling_rate,
			op_params->get_multiple(),
			op_params->detune1,
			op_params->amplitude_modulation_shift
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_opx_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opx, "#OPX@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		SiOPMOperatorParams *op_params = p_params->operator_params[i];

		int wave_shape = _get_ma3_from_pg_type(op_params->pulse_generator_type, "#OPX@");
		if (wave_shape == -1) {
			return Vector<int>();
		}

		res.append_array({
			wave_shape,
			op_params->attack_rate >> 1,
			op_params->decay_rate >> 1,
			op_params->sustain_rate >> 1,
			op_params->release_rate >> 2,
			op_params->sustain_level,
			op_params->total_level,
			op_params->key_scaling_rate,
			op_params->get_multiple(),
			op_params->detune1,
			op_params->detune,
			op_params->amplitude_modulation_shift
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_ma3_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_ma3, "#MA@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		SiOPMOperatorParams *op_params = p_params->operator_params[i];

		int wave_shape = _get_ma3_from_pg_type(op_params->pulse_generator_type, "#MA@");
		if (wave_shape == -1) {
			return Vector<int>();
		}

		int total_level = op_params->total_level < 63 ? op_params->total_level : 63;

		res.append_array({
			wave_shape,
			op_params->attack_rate >> 2,
			op_params->decay_rate >> 2,
			op_params->sustain_rate >> 2,
			op_params->release_rate >> 2,
			op_params->sustain_level,
			total_level,
			op_params->key_scaling_rate >> 1,
			op_params->key_scaling_level,
			op_params->get_multiple(),
			op_params->detune1,
			op_params->amplitude_modulation_shift
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_al_params(SiOPMChannelParams *p_params) {
	if (p_params->operator_count != 5) {
		return Vector<int>();
	}

	SiOPMOperatorParams *op_params0 = p_params->operator_params[0];
	SiOPMOperatorParams *op_params1 = p_params->operator_params[1];

	int level_balance = _balance_total_levels(op_params0->total_level, op_params1->total_level);

	Vector<int> res = {
		p_params->algorithm,
		op_params0->pulse_generator_type,
		op_params1->pulse_generator_type,
		level_balance,
		op_params1->detune,
		op_params0->attack_rate,
		op_params0->decay_rate,
		op_params0->sustain_level,
		op_params0->release_rate
	};

	return res;
}

String TranslatorUtil::mml_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

String TranslatorUtil::mml_opl_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

String TranslatorUtil::mml_opm_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

String TranslatorUtil::mml_opn_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

String TranslatorUtil::mml_opx_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

String TranslatorUtil::mml_ma3_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

String TranslatorUtil::mml_al_params(SiOPMChannelParams *p_params, String p_separator, String p_line_end, String p_comment) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

void TranslatorUtil::parse_voice_setting(const Ref<SiMMLVoice> &p_voice, String p_mml, Vector<SiMMLEnvelopeTable *> p_envelopes) {
	// FIXME: NOT IMPLEMENTED
}

String TranslatorUtil::mml_voice_setting(const Ref<SiMMLVoice> &p_voice) {
	// FIXME: NOT IMPLEMENTED
	return "";
}

//

TranslatorUtil::MMLTableNumbers TranslatorUtil::parse_table_numbers(String p_table_numbers, String p_postfix, int p_max_index) {
	MMLTableNumbers parsed_table;

	SinglyLinkedList<int> *head = SinglyLinkedList<int>::alloc(0);
	SinglyLinkedList<int> *last = head;
	SinglyLinkedList<int> *repeat = nullptr;

	// Magnification.
	Ref<RegEx> re_postfix = RegEx::create_from_string("(\\d+)?(\\*(-?[\\d.]+))?([+-][\\d.]+)?");
	Ref<RegExMatch> res = re_postfix->search(p_postfix);
	ERR_FAIL_COND_V(res.is_null(), parsed_table);

	int postfix_size = 1;
	double postfix_coef = 1;
	double postfix_offset = 0;

	if (!res->get_string(1).is_empty()) {
		postfix_size = res->get_string(1).to_int();
	}
	if (!res->get_string(2).is_empty()) {
		postfix_coef = res->get_string(3).to_float();
	}
	if (!res->get_string(4).is_empty()) {
		postfix_offset = res->get_string(4).to_float();
	}

	// res[1];(n..),m {res[2];n.., res[3];m} / res[4];n / res[5];|[] / res[6]; ]n
	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "gm". Behavioral implications require investigation.
	Ref<RegEx> re_table = RegEx::create_from_string("(\\(\\s*([,\\-\\d\\s]+)\\)[,\\s]*(\\d+))|(-?\\d+)|(\\||\\[|\\](\\d*))");
	TypedArray<RegExMatch> numbers = re_table->search_all(p_table_numbers);

	int index = 0;
	for (int n = 0; n < numbers.size() && index < p_max_index; n++) {
		Ref<RegExMatch> parsed_number = numbers[n];

		if (!parsed_number->get_string(1).is_empty()) {
			// Interpolation: "(res[2]..),res[3]"

			PackedStringArray arr = split_string_by_regex(parsed_number->get_string(2), "[,\\s]+");
			int inter_size = parsed_number->get_string(3).to_int();
			ERR_FAIL_COND_V_MSG((inter_size < 2 || arr.size() < 1), parsed_table, "Translator: Failed to parse provided MML table, interpolation data is invalid.");

			Vector<int> inter_data;
			inter_data.resize_zeroed(arr.size());
			for (int i = 0; i < inter_data.size(); i++) {
				inter_data.write[i] = arr[i].to_int();
			}

			if (inter_data.size() > 1) {
				double t = 0;
				double s = ((double)inter_data.size() - 1.0) / inter_size;

				for (int i = 0; i < inter_size && index < p_max_index; i++) {
					int ti0 = (int)t;
					int ti1 = ti0 + 1;
					double tr = t - (double)ti0;

					int value = (int)(inter_data[ti0] * (1 - tr) + inter_data[ti1] * tr + 0.5);
					value = (int)(value * postfix_coef + postfix_offset + 0.5);

					for (int j = 0; j < postfix_size; j++) {
						last->next = SinglyLinkedList<int>::alloc(value);
						last = last->next;
					}
					index += postfix_size;

					t += s;
				}
			} else {
				int value = (int)(inter_data[0] * postfix_coef + postfix_offset + 0.5);

				for (int i = 0; i < inter_size && index < p_max_index; i++) {
					for (int j = 0; j < postfix_size; j++) {
						last->next = SinglyLinkedList<int>::alloc(value);
						last = last->next;
					}
					index += postfix_size;
				}
			}
		} else if (!parsed_number->get_string(4).is_empty()) {
			// Single number.
			int value = parsed_number->get_string(4).to_int();
			value = (int)(value * postfix_coef + postfix_offset + 0.5);

			for (int j = 0; j < postfix_size; j++) {
				last->next = SinglyLinkedList<int>::alloc(value);
				last = last->next;
			}
			index++;
		} else if (!parsed_number->get_string(5).is_empty()) {
			List<SinglyLinkedList<int> *> loop_stack;
			SinglyLinkedList<int> *loop_head = nullptr;
			SinglyLinkedList<int> *loop_tail = nullptr;

			String token = parsed_number->get_string(5);
			if (token == "|") {        // Repeat point.
				repeat = last;
			} else if (token == "[") { // Being loop.
				loop_stack.push_back(last);
			} else {                   // End loop.
				ERR_FAIL_COND_V_MSG(loop_stack.is_empty(), parsed_table, "Translator: Failed to parse provided MML table, loop data is invalid.");

				loop_head = loop_stack.back()->get()->next;
				loop_stack.pop_back();
				ERR_FAIL_COND_V_MSG(!loop_head, parsed_table, "Translator: Failed to parse provided MML table, loop data is invalid.");

				loop_tail = last;
				int loop_count = 2;
				if (!parsed_number->get_string(6).is_empty()) {
					loop_count = parsed_number->get_string(6).to_int();
				}
				for (int j = loop_count; j > 0; j--) {
					for (SinglyLinkedList<int> *l = loop_head; l != loop_tail->next; l = l->next) {
						last->next = SinglyLinkedList<int>::alloc(l->value);
						last = last->next;
					}
				}
			}
		} else {
			ERR_FAIL_V_MSG(parsed_table, "Translator: Failed to parse provided MML table, structure is invalid.");
		}
	}

	if (repeat) {
		last->next = repeat->next;
	}

	parsed_table.head = head->next;
	parsed_table.tail = last;
	parsed_table.length = index;
	parsed_table.repeated = (repeat != nullptr);
	return parsed_table;
}

Vector<double> TranslatorUtil::parse_wav(String p_table_numbers, String p_postfix) {
	// FIXME: NOT IMPLEMENTED
	return Vector<double>();
}

Vector<double> TranslatorUtil::parse_wavb(String p_hex) {
	// FIXME: NOT IMPLEMENTED
	return Vector<double>();
}

bool TranslatorUtil::parse_sampler_wave(SiOPMWaveSamplerTable *p_table, int p_note_number, String p_mml, HashMap<String, Variant> p_sound_ref_table) {
	// FIXME: NOT IMPLEMENTED
	return false;
}

bool TranslatorUtil::parse_pcm_wave(SiOPMWavePCMTable *p_table, String p_mml, HashMap<String, Variant> p_sound_ref_table) {
	// FIXME: NOT IMPLEMENTED
	return false;
}

bool TranslatorUtil::parse_pcm_voice(const Ref<SiMMLVoice> &p_voice, String p_mml, String p_postfix, Vector<SiMMLEnvelopeTable *> p_envelopes) {
	// FIXME: NOT IMPLEMENTED
	return false;
}

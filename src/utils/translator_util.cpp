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
#include "chip/wave/siopm_wave_pcm_data.h"
#include "chip/wave/siopm_wave_pcm_table.h"
#include "chip/wave/siopm_wave_sampler_data.h"
#include "chip/wave/siopm_wave_sampler_table.h"
#include "sequencer/simml_ref_table.h"
#include "utils/godot_util.h"

// Channel params.

Vector<int> TranslatorUtil::_split_data_string(const Ref<SiOPMChannelParams> &p_params, String p_data_string, int p_channel_param_count, int p_operator_param_count, const String &p_command) {
	if (p_data_string.is_empty()) {
		p_params->set_operator_count(0);
		return Vector<int>();
	}

	// Godot's RegEx implementation doesn't support passing global flags, but PCRE2 allows local flags, which we can abuse.
	// (?s) enables single line mode (dot matches newline) for the entire expression.
	Ref<RegEx> re_comments = RegEx::create_from_string("(?s)/\\*.*?\\*/|//.*?[\\r\\n]+");
	Ref<RegEx> re_cleanup = RegEx::create_from_string("^[^\\d\\-.]+|[^\\d\\-.]+$");

	String sanitized_string = re_comments->sub(p_data_string, "", true);
	sanitized_string = re_cleanup->sub(sanitized_string, "", true);
	PackedStringArray string_data = split_string_by_regex(sanitized_string, "[^\\d\\-.]+");

	for (int i = 1; i <= SiOPMChannelParams::MAX_OPERATORS; i++) {
		if (string_data.size() == (p_channel_param_count + p_operator_param_count * i)) {
			p_params->set_operator_count(i);

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

void TranslatorUtil::_check_operator_count(const Ref<SiOPMChannelParams> &p_params, int p_data_length, int p_channel_param_count, int p_operator_param_count, const String &p_command) {
	int op_count = (p_data_length - p_channel_param_count) / p_operator_param_count;
	ERR_FAIL_COND_MSG(op_count > SiOPMChannelParams::MAX_OPERATORS, vformat("Translator: Invalid operator count in '%s' (parameters for: %d, max: %d).", p_command, op_count, SiOPMChannelParams::MAX_OPERATORS));
	ERR_FAIL_COND_MSG((op_count * p_operator_param_count + p_channel_param_count) != p_data_length, vformat("Translator: Invalid parameter count in '%s' (total: %d, channel: %d, each operator: %d).", p_command, p_data_length, p_channel_param_count, p_operator_param_count));

	p_params->set_operator_count(op_count);
}

int TranslatorUtil::_sanitize_param_loop(int p_value, int p_min, int p_max, const String &p_label) {
	if (unlikely(p_value < p_min || p_value > p_max)) {
		ERR_PRINT(vformat("Translator: Parameter '%s' value (%d) is outside of valid range (%d : %d). Value will be looped.", p_label, p_value, p_min, p_max));
	}

	// Special case when -1 is allowed. Other negative values still loop, which is ehhh...
	// But that's how the original is, so why not. We still report all invalid values here.
	if (p_min == -1 && p_value == -1) {
		return p_value;
	}

	// WARN: Max value must be a bitmask, e.g. 0xFF. In other words, it's power-of-2 minus 1 (1, 3, 7, 15, 31, 63, 127, 255, 511).
	return p_value & p_max;
}

int TranslatorUtil::_sanitize_param_clamp(int p_value, int p_min, int p_max, const String &p_label) {
	if (unlikely(p_value < p_min || p_value > p_max)) {
		ERR_PRINT(vformat("Translator: Parameter '%s' value (%d) is outside of valid range (%d : %d). Value will be clamped.", p_label, p_value, p_min, p_max));
	}

	return CLAMP(p_value, p_min, p_max);
}

int TranslatorUtil::_get_params_algorithm(int (&p_algorithms)[4][16], int p_operator_count, int p_data_value, int p_max_value, const String &p_command) {
	int alg_index = p_operator_count - 1;
	ERR_FAIL_INDEX_V_MSG(alg_index, 4, -1, vformat("Translator: Invalid operator count (%d) for the algorithm in '%s'.", p_operator_count, p_command));

	// WARN: Max value must be a bitmask, e.g. 0xFF. In other words, it's power-of-2 minus 1 (1, 3, 7, 15, 31, 63, 127, 255, 511).
	int alg_data = _sanitize_param_loop(p_data_value, 0, p_max_value, "AL");
	ERR_FAIL_INDEX_V_MSG(alg_data, 16, -1, vformat("Translator: Invalid algorithm parameter %d in '%s'.", p_data_value, p_command));

	int algorithm = p_algorithms[alg_index][alg_data];
	ERR_FAIL_COND_V_MSG(algorithm == -1, -1, vformat("Translator: Unsupported algorithm parameter %d in '%s'.", p_data_value, p_command));

	return algorithm;
}

void TranslatorUtil::_set_siopm_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	if (p_params->operator_count == 0) {
		return;
	}

	// #@ (SiOPM) signature:
	// AL[0-15], FB[0-7], FC[0-3],
	// (WS[0-511], AR[0-63], DR[0-63], SR[0-63], RR[0-63], SL[0-15], TL[0-127], KR[0-3], KL[0-3], ML[0-15], D1[0-7], D2[], AM[0-3], PH[-1-255], FN[0-127]) x operator_count

	p_params->algorithm           = _sanitize_param_loop(p_data[0], 0, 15, "AL");
	p_params->feedback            = _sanitize_param_loop(p_data[1], 0, 7,  "FB");
	p_params->feedback_connection = _sanitize_param_loop(p_data[2], 0, 3,  "FC");

	int data_index = 3;
	for (int op_index = 0; op_index < p_params->operator_count; op_index++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[op_index];

		op_params->set_pulse_generator_type    (_sanitize_param_loop(p_data[data_index++], 0, 511,  "WS"));       // 1
		op_params->attack_rate                = _sanitize_param_loop(p_data[data_index++], 0, 63,   "AR");        // 2
		op_params->decay_rate                 = _sanitize_param_loop(p_data[data_index++], 0, 63,   "DR");        // 3
		op_params->sustain_rate               = _sanitize_param_loop(p_data[data_index++], 0, 63,   "SR");        // 4
		op_params->release_rate               = _sanitize_param_loop(p_data[data_index++], 0, 63,   "RR");        // 5
		op_params->sustain_level              = _sanitize_param_loop(p_data[data_index++], 0, 15,   "SL");        // 6
		op_params->total_level                = _sanitize_param_loop(p_data[data_index++], 0, 127,  "TL");        // 7
		op_params->key_scaling_rate           = _sanitize_param_loop(p_data[data_index++], 0, 3,    "KR");        // 8
		op_params->key_scaling_level          = _sanitize_param_loop(p_data[data_index++], 0, 3,    "KL");        // 9
		op_params->set_multiple                (_sanitize_param_loop(p_data[data_index++], 0, 15,   "ML"));       // 10
		op_params->detune1                    = _sanitize_param_loop(p_data[data_index++], 0, 7,    "D1");        // 11
		op_params->detune2                    = p_data[data_index++];                                             // 12
		op_params->amplitude_modulation_shift = _sanitize_param_loop(p_data[data_index++], 0, 3,    "D2");        // 13
		op_params->initial_phase              = _sanitize_param_loop(p_data[data_index++], -1, 255, "PH");        // 14
		op_params->fixed_pitch                = (_sanitize_param_loop(p_data[data_index++], 0, 127, "FN")) << 6;  // 15
	}
}

void TranslatorUtil::_set_opl_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	if (p_params->operator_count == 0) {
		return;
	}

	// #OPL@ signature:
	// AL[0-3], FB[0-7],
	// (WS[0-31], AR[0-15], DR[0-15], RR[0-15], ET[0,1], SL[0-15], TL[0-63], KR[0,1], KL[0-3], ML[0-15], AM[0-3]) x operator_count

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opl, p_params->operator_count, p_data[0], 3, "#OPL@");
	if (algorithm == -1) {
		return;
	}

	p_params->envelope_frequency_ratio = 133;
	p_params->algorithm = algorithm;
	p_params->feedback = _sanitize_param_loop(p_data[1], 0, 7, "FB");

	int data_index = 2;
	for (int op_index = 0; op_index < p_params->operator_count; op_index++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[op_index];

		int pg_type = SiONPulseGeneratorType::PULSE_MA3_SINE + _sanitize_param_loop(p_data[data_index++], 0, 31, "WS");
		op_params->set_pulse_generator_type(pg_type);                                                                                  // 1

		op_params->attack_rate                                      = (_sanitize_param_loop(p_data[data_index++], 0, 15, "AR") << 2);  // 2
		op_params->decay_rate                                       = (_sanitize_param_loop(p_data[data_index++], 0, 15, "DR") << 2);  // 3
		op_params->release_rate                                     = (_sanitize_param_loop(p_data[data_index++], 0, 15, "RR") << 2);  // 4

		// If envelope waveform type is 0 — decaying sound, if it is 1 — sustained sound.
		int n = _sanitize_param_loop(p_data[data_index++], 0, 1, "ET");
		op_params->sustain_rate = (n != 0) ? 0 : op_params->release_rate;                                                              // 5
		op_params->sustain_level                                    = _sanitize_param_loop(p_data[data_index++], 0, 15, "SL");         // 6
		op_params->total_level                                      = _sanitize_param_loop(p_data[data_index++], 0, 63, "TL");         // 7
		op_params->key_scaling_rate                                 = (_sanitize_param_loop(p_data[data_index++], 0, 1, "KR") << 1);   // 8
		op_params->key_scaling_level                                = _sanitize_param_loop(p_data[data_index++], 0, 3, "KL");          // 9

		int i = _sanitize_param_loop(p_data[data_index++], 0, 15, "ML");
		op_params->set_multiple((i == 11 || i == 13) ? (i - 1) : (i == 14) ? (i + 1) : i);                                             // 10
		op_params->amplitude_modulation_shift                       = _sanitize_param_loop(p_data[data_index++], 0, 3, "AM");          // 11
	}
}

void TranslatorUtil::_set_opm_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	if (p_params->operator_count == 0) {
		return;
	}

	// #OPM@ signature:
	// AL[0-7], FB[0-7],
	// (AR[0-31], DR[0-31], SR[0-31], RR[0-15], SL[0-15], TL[0-127], KR[0-3], ML[0-15], D1[0-7], D2[0-3], AM[0-3]) x operator_count

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opm, p_params->operator_count, p_data[0], 7, "#OPM@");
	if (algorithm == -1) {
		return;
	}

	p_params->algorithm = algorithm;
	p_params->feedback = _sanitize_param_loop(p_data[1], 0, 7, "FB");

	int data_index = 2;
	for (int op_index = 0; op_index < p_params->operator_count; op_index++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[op_index];

		op_params->attack_rate               = (_sanitize_param_loop(p_data[data_index++], 0, 31, "AR") << 1);       // 1
		op_params->decay_rate                = (_sanitize_param_loop(p_data[data_index++], 0, 31, "DR") << 1);       // 2
		op_params->sustain_rate              = (_sanitize_param_loop(p_data[data_index++], 0, 31, "SR") << 1);       // 3
		op_params->release_rate             = ((_sanitize_param_loop(p_data[data_index++], 0, 15, "RR") << 2) + 2);  // 4
		op_params->sustain_level             = _sanitize_param_loop(p_data[data_index++], 0, 15,  "SL");             // 5
		op_params->total_level               = _sanitize_param_loop(p_data[data_index++], 0, 127, "TL");             // 6
		op_params->key_scaling_rate          = _sanitize_param_loop(p_data[data_index++], 0, 3,   "KR");             // 7
		op_params->set_multiple               (_sanitize_param_loop(p_data[data_index++], 0, 15,  "ML"));            // 8
		op_params->detune1                   = _sanitize_param_loop(p_data[data_index++], 0, 7,   "D1");             // 9

		int n = _sanitize_param_loop(p_data[data_index++], 0, 3, "D2");
		op_params->detune2 = SiOPMRefTable::get_instance()->dt2_table[n];                                            // 10
		op_params->amplitude_modulation_shift = _sanitize_param_loop(p_data[data_index++], 0, 3, "AM");              // 11
	}
}

void TranslatorUtil::_set_opn_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	if (p_params->operator_count == 0) {
		return;
	}

	// #OPN@ signature:
	// AL[0-7], FB[0-7],
	// (AR[0-31], DR[0-31], SR[0-31], RR[0-15], SL[0-15], TL[0-127], KR[0-3], ML[0-15], D1[0-7], AM[0-3]) x operator_count

	// Note: OPM and OPN share the algo list.
	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opm, p_params->operator_count, p_data[0], 7, "#OPN@");
	if (algorithm == -1) {
		return;
	}

	p_params->algorithm = algorithm;
	p_params->feedback = _sanitize_param_loop(p_data[1], 0, 7, "FB");

	int data_index = 2;
	for (int op_index = 0; op_index < p_params->operator_count; op_index++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[op_index];

		op_params->attack_rate                = (_sanitize_param_loop(p_data[data_index++], 0, 31, "AR") << 1);       // 1
		op_params->decay_rate                 = (_sanitize_param_loop(p_data[data_index++], 0, 31, "DR") << 1);       // 2
		op_params->sustain_rate               = (_sanitize_param_loop(p_data[data_index++], 0, 31, "SR") << 1);       // 3
		op_params->release_rate              = ((_sanitize_param_loop(p_data[data_index++], 0, 15, "RR") << 2) + 2);  // 4
		op_params->sustain_level              = _sanitize_param_loop(p_data[data_index++], 0, 15,  "SL");             // 5
		op_params->total_level                = _sanitize_param_loop(p_data[data_index++], 0, 127, "TL");             // 6
		op_params->key_scaling_rate           = _sanitize_param_loop(p_data[data_index++], 0, 3,   "KR");             // 7
		op_params->set_multiple                (_sanitize_param_loop(p_data[data_index++], 0, 15,  "ML"));            // 8
		op_params->detune1                    = _sanitize_param_loop(p_data[data_index++], 0, 7,   "D1");             // 9
		op_params->amplitude_modulation_shift = _sanitize_param_loop(p_data[data_index++], 0, 3,   "AM");             // 10
	}
}

void TranslatorUtil::_set_opx_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	if (p_params->operator_count == 0) {
		return;
	}

	// #OPX@ signature:
	// AL[0-15], FB[0-7],
	// (WS[0-7?], AR[0-31], DR[0-31], SR[0-31], RR[0-15], SL[0-15], TL[0-127], KR[0-3], ML[0-15], D1[0-7], D2[], AM[0-3]) x operator_count

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_opx, p_params->operator_count, p_data[0], 15, "#OPX@");
	if (algorithm == -1) {
		return;
	}

	// LSB is the flag of feedback connection.
	p_params->algorithm = (algorithm & 15);
	p_params->feedback = _sanitize_param_loop(p_data[1], 0, 7, "FB");
	p_params->feedback_connection = (algorithm & 16) ? 1 : 0;

	int data_index = 2;
	for (int op_index = 0; op_index < p_params->operator_count; op_index++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[op_index];

		// Standard supported values are in the [0-7] range. Values beyond that are supported for custom waves.
		int wave_shape = p_data[data_index++];
		if (wave_shape < 8) {
			int pg_type = SiONPulseGeneratorType::PULSE_MA3_SINE + _sanitize_param_loop(wave_shape, 0, 7, "WS");
			op_params->set_pulse_generator_type(pg_type);                                                             // 1
		} else {
			int pg_type = SiONPulseGeneratorType::PULSE_CUSTOM + _sanitize_param_clamp(wave_shape - 8, 0, 127, "WS");
			op_params->set_pulse_generator_type(pg_type);                                                             // 1
		}

		op_params->attack_rate                = (_sanitize_param_loop(p_data[data_index++], 0, 31, "AR") << 1);       // 2
		op_params->decay_rate                 = (_sanitize_param_loop(p_data[data_index++], 0, 31, "DR") << 1);       // 3
		op_params->sustain_rate               = (_sanitize_param_loop(p_data[data_index++], 0, 31, "SR") << 1);       // 4
		op_params->release_rate              = ((_sanitize_param_loop(p_data[data_index++], 0, 15, "RR") << 2) + 2);  // 5
		op_params->sustain_level              = _sanitize_param_loop(p_data[data_index++], 0, 15,  "SL");             // 6
		op_params->total_level                = _sanitize_param_loop(p_data[data_index++], 0, 127, "TL");             // 7
		op_params->key_scaling_rate           = _sanitize_param_loop(p_data[data_index++], 0, 3,   "KR");             // 8
		op_params->set_multiple                (_sanitize_param_loop(p_data[data_index++], 0, 15,  "ML"));            // 9
		op_params->detune1                    = _sanitize_param_loop(p_data[data_index++], 0, 7,   "D1");             // 10
		op_params->detune2                    = p_data[data_index++];                                                 // 11
		op_params->amplitude_modulation_shift = _sanitize_param_loop(p_data[data_index++], 0, 3,   "AM");             // 12
	}
}

void TranslatorUtil::_set_ma3_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	if (p_params->operator_count == 0) {
		return;
	}

	// #MA@ signature:
	// AL[0-7], FB[0-7],
	// (WS[0-31], AR[0-15], DR[0-15], SR[0-15], RR[0-15], SL[0-15], TL[0-63], KR[0,1], KL[0-3], ML[0-15], D1[0-7], AM[0-3]) x operator_count

	int algorithm = _get_params_algorithm(SiMMLRefTable::get_instance()->algorithm_ma3, p_params->operator_count, p_data[0], 7, "#MA@");
	if (algorithm == -1) {
		return;
	}

	p_params->envelope_frequency_ratio = 133;
	p_params->algorithm = algorithm;
	p_params->feedback = _sanitize_param_loop(p_data[1], 0, 7, "FB");

	int data_index = 2;
	for (int op_index = 0; op_index < p_params->operator_count; op_index++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[op_index];

		int pg_type = SiONPulseGeneratorType::PULSE_MA3_SINE + _sanitize_param_loop(p_data[data_index++], 0, 31, "WS");
		op_params->set_pulse_generator_type(pg_type);                                                                     // 1

		op_params->attack_rate                       = (_sanitize_param_loop(p_data[data_index++], 0, 15, "AR") << 2);    // 2
		op_params->decay_rate                        = (_sanitize_param_loop(p_data[data_index++], 0, 15, "DR") << 2);    // 3
		op_params->sustain_rate                      = (_sanitize_param_loop(p_data[data_index++], 0, 15, "SR") << 2);    // 4
		op_params->release_rate                      = (_sanitize_param_loop(p_data[data_index++], 0, 15, "RR") << 2);    // 5
		op_params->sustain_level                     = _sanitize_param_loop(p_data[data_index++], 0, 15, "SL");           // 6
		op_params->total_level                       = _sanitize_param_loop(p_data[data_index++], 0, 63, "TL");           // 7
		op_params->key_scaling_rate                  = (_sanitize_param_loop(p_data[data_index++], 0, 1, "KR") << 1);     // 8
		op_params->key_scaling_level                 = _sanitize_param_loop(p_data[data_index++], 0, 3, "KL");            // 9

		int i = _sanitize_param_loop(p_data[data_index++], 0, 15, "ML");
		op_params->set_multiple((i == 11 || i == 13) ? (i - 1) : (i == 14) ? (i + 1) : i);                                // 10
		op_params->detune1                           = _sanitize_param_loop(p_data[data_index++], 0, 7, "D1");            // 11
		op_params->amplitude_modulation_shift        = _sanitize_param_loop(p_data[data_index++], 0, 3, "AM");            // 12
	}
}

void TranslatorUtil::_set_al_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	p_params->set_operator_count(2);
	p_params->set_analog_like(true);

	// #AL@ signature:
	// CN[0-2], W1[0-511], W2[0-511], BL[-64-+64], DT[]
	// AR[0-63], DR[0-63], SL[0-15], RR[0-63]

	// Can't use _sanitize_param_loop here because 2 is not a valid number for the max value. So hack something ad-hoc together instead.
	if (unlikely(p_data[0] < 0 || p_data[0] > 2)) {
		ERR_PRINT(vformat("Translator: Parameter '%s' value (%d) is outside of valid range (%d : %d).", "CN", p_data[0], 0, 2));
		p_params->algorithm = 0;
	} else {
		p_params->algorithm = p_data[0];
	}

	Ref<SiOPMOperatorParams> op_params0 = p_params->operator_params[0];
	Ref<SiOPMOperatorParams> op_params1 = p_params->operator_params[1];

	op_params0->set_pulse_generator_type(_sanitize_param_loop(p_data[1], 0, 511, "W1"));
	op_params1->set_pulse_generator_type(_sanitize_param_loop(p_data[2], 0, 511, "W2"));

	int balance = _sanitize_param_clamp(p_data[3], -64, 64, "BL");
	int (&tl_table)[129] = SiOPMRefTable::get_instance()->eg_linear_to_total_level_table;
	op_params0->total_level = tl_table[64 - balance];
	op_params1->total_level = tl_table[balance + 64];

	op_params0->detune2 = 0;
	op_params1->detune2 = p_data[4];

	op_params0->attack_rate   = _sanitize_param_loop(p_data[5], 0, 63, "AR");
	op_params0->decay_rate    = _sanitize_param_loop(p_data[6], 0, 63, "DR");
	op_params0->sustain_rate  = 0;
	op_params0->release_rate  = _sanitize_param_loop(p_data[8], 0, 15, "RR");
	op_params0->sustain_level = _sanitize_param_loop(p_data[7], 0, 63, "SL");
}

void TranslatorUtil::parse_siopm_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_siopm_params_by_array(p_params, _split_data_string(p_params, p_data_string, 3, 15, "#@"));
}

void TranslatorUtil::parse_opl_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_opl_params_by_array(p_params, _split_data_string(p_params, p_data_string, 2, 11, "#OPL@"));
}

void TranslatorUtil::parse_opm_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_opm_params_by_array(p_params, _split_data_string(p_params, p_data_string, 2, 11, "#OPM@"));
}

void TranslatorUtil::parse_opn_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_opn_params_by_array(p_params, _split_data_string(p_params, p_data_string, 2, 10, "#OPN@"));
}

void TranslatorUtil::parse_opx_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_opx_params_by_array(p_params, _split_data_string(p_params, p_data_string, 2, 12, "#OPX@"));
}

void TranslatorUtil::parse_ma3_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_ma3_params_by_array(p_params, _split_data_string(p_params, p_data_string, 2, 12, "#MA@"));
}

void TranslatorUtil::parse_al_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string) {
	return _set_al_params_by_array(p_params, _split_data_string(p_params, p_data_string, 9, 0, "#AL@"));
}

void TranslatorUtil::set_siopm_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	_check_operator_count(p_params, p_data.size(), 3, 15, "#@");
	return _set_siopm_params_by_array(p_params, p_data);
}

void TranslatorUtil::set_opl_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	_check_operator_count(p_params, p_data.size(), 2, 11, "#OPL@");
	return _set_opl_params_by_array(p_params, p_data);
}

void TranslatorUtil::set_opm_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	_check_operator_count(p_params, p_data.size(), 2, 11, "#OPM@");
	return _set_opm_params_by_array(p_params, p_data);
}

void TranslatorUtil::set_opn_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	_check_operator_count(p_params, p_data.size(), 2, 10, "#OPN@");
	return _set_opn_params_by_array(p_params, p_data);
}

void TranslatorUtil::set_opx_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	_check_operator_count(p_params, p_data.size(), 2, 12, "#OPX@");
	return _set_opx_params_by_array(p_params, p_data);
}

void TranslatorUtil::set_ma3_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	_check_operator_count(p_params, p_data.size(), 2, 12, "#MA@");
	return _set_ma3_params_by_array(p_params, p_data);
}

void TranslatorUtil::set_al_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data) {
	ERR_FAIL_COND_MSG(p_data.size() != 9, vformat("Translator: Invalid parameter count in '%s' (channel: %d, each operator: %d).", "#AL@", 9, 0));

	return _set_al_params_by_array(p_params, p_data);
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
	// Standard wave types.
	int wave_shape = p_pulse_generator_type - SiONPulseGeneratorType::PULSE_MA3_SINE;
	if (wave_shape >= 0 && wave_shape <= 31) {
		return wave_shape;
	}

	// Custom wave types.
	int custom_type = p_pulse_generator_type - SiONPulseGeneratorType::PULSE_CUSTOM;
	if (custom_type >= 0 && custom_type <= 127) {
		return custom_type;
	}

	// Known PG types compatible with wave types.
	switch (p_pulse_generator_type) {
		case 0:                               return 0;   // Sine
		case 1: case 2:   case 128: case 255: return 24;  // Saw
		case 4: case 191: case 192:           return 16;  // Triangle
		case 5: case 72:                      return 6;   // Square
	}

	ERR_FAIL_V_MSG(-1, vformat("Translator: Cannot convert pulse generator type (%d) into a wave shape in '%s'.", p_pulse_generator_type, p_command));
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

Vector<int> TranslatorUtil::get_siopm_params(const Ref<SiOPMChannelParams> &p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	Vector<int> res = { p_params->algorithm, p_params->feedback, p_params->feedback_connection };

	for (int i = 0; i < p_params->operator_count; i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];
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
			op_params->detune2,
			op_params->amplitude_modulation_shift,
			op_params->initial_phase,
			op_params->fixed_pitch >> 6
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_opl_params(const Ref<SiOPMChannelParams> &p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opl, "#OPL@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];

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

Vector<int> TranslatorUtil::get_opm_params(const Ref<SiOPMChannelParams> &p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opm, "#OPM@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];

		int	detune2 = _get_nearest_dt2(op_params->detune2);

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

Vector<int> TranslatorUtil::get_opn_params(const Ref<SiOPMChannelParams> &p_params) {
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
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];

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

Vector<int> TranslatorUtil::get_opx_params(const Ref<SiOPMChannelParams> &p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opx, "#OPX@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];

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
			op_params->detune2,
			op_params->amplitude_modulation_shift
		});
	}

	return res;
}

Vector<int> TranslatorUtil::get_ma3_params(const Ref<SiOPMChannelParams> &p_params) {
	if (p_params->operator_count == 0) {
		return Vector<int>();
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_ma3, "#MA@");
	if (alg_index == -1) {
		return Vector<int>();
	}

	Vector<int> res = { alg_index, p_params->feedback };

	for (int i = 0; i < p_params->operator_count; i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];

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

Vector<int> TranslatorUtil::get_al_params(const Ref<SiOPMChannelParams> &p_params) {
	if (p_params->operator_count != 5) {
		return Vector<int>();
	}

	Ref<SiOPMOperatorParams> op_params0 = p_params->operator_params[0];
	Ref<SiOPMOperatorParams> op_params1 = p_params->operator_params[1];

	int level_balance = _balance_total_levels(op_params0->total_level, op_params1->total_level);

	Vector<int> res = {
		p_params->algorithm,
		op_params0->pulse_generator_type,
		op_params1->pulse_generator_type,
		level_balance,
		op_params1->detune2,
		op_params0->attack_rate,
		op_params0->decay_rate,
		op_params0->sustain_level,
		op_params0->release_rate
	};

	return res;
}

String TranslatorUtil::_format_mml_comment(const String &p_comment, const String &p_line_end) {
	if (p_comment.is_empty()) {
		return "";
	}

	if (p_line_end == "\n") {
		return vformat(" // %s", p_comment);
	} else {
		return vformat("/* %s */", p_comment);
	}
}

String TranslatorUtil::_format_mml_digit(int p_value, int p_padded) {
	if (p_padded <= 0) {
		return itos(p_value);
	}

	int padded_length = p_padded;
	if (p_value < 0) {
		padded_length -= 1; // Accounts for the minus sign.
	}

	return itos(p_value).pad_zeros(padded_length);
}

TranslatorUtil::OperatorParamsSizes TranslatorUtil::_get_operator_params_sizes(const Ref<SiOPMChannelParams> &p_params) {
	OperatorParamsSizes sizes;

#define MAX_PARAM_SIZE(m_key, m_value)              \
	{                                               \
		String value_string = itos(m_value);        \
		if (value_string.length() > sizes.m_key) {  \
			sizes.m_key = value_string.length();    \
		}                                           \
	}

	for (int i = 0; i < p_params->operator_count; i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->operator_params[i];

		MAX_PARAM_SIZE(pg_type,     op_params->pulse_generator_type)
		MAX_PARAM_SIZE(total_level, op_params->total_level)
		MAX_PARAM_SIZE(detune2,     op_params->detune2)
		MAX_PARAM_SIZE(phase,       op_params->initial_phase)
		MAX_PARAM_SIZE(fixed_pitch, op_params->fixed_pitch >> 6)
	}

#undef MAX_PARAM_SIZE

	return sizes;
}

String TranslatorUtil::get_siopm_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() == 0) {
		return "";
	}

	// Open MML string.
	String mml = "{";

	// Channel parameters.

	mml += _format_mml_digit(p_params->algorithm) + p_separator;
	mml += _format_mml_digit(p_params->feedback)  + p_separator;
	mml += _format_mml_digit(p_params->feedback_connection);

	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Operator parameters.

	OperatorParamsSizes sizes = _get_operator_params_sizes(p_params);

	for (int i = 0; i < p_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->get_operator_params(i);

		mml += p_line_end;

		mml += _format_mml_digit(op_params->pulse_generator_type, sizes.pg_type)  + p_separator;
		mml += _format_mml_digit(op_params->attack_rate, 2)                       + p_separator;
		mml += _format_mml_digit(op_params->decay_rate, 2)                        + p_separator;
		mml += _format_mml_digit(op_params->sustain_rate, 2)                      + p_separator;
		mml += _format_mml_digit(op_params->release_rate, 2)                      + p_separator;
		mml += _format_mml_digit(op_params->sustain_level, 2)                     + p_separator;
		mml += _format_mml_digit(op_params->total_level, sizes.total_level)       + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_rate)                     + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_level)                    + p_separator;
		mml += _format_mml_digit(op_params->get_multiple(), 2)                    + p_separator;
		mml += _format_mml_digit(op_params->detune1)                              + p_separator;
		mml += _format_mml_digit(op_params->detune2, sizes.detune2)               + p_separator;
		mml += _format_mml_digit(op_params->amplitude_modulation_shift)           + p_separator;
		mml += _format_mml_digit(op_params->initial_phase, sizes.phase)           + p_separator;
		mml += _format_mml_digit(op_params->fixed_pitch >> 6, sizes.fixed_pitch);
	}

	// Close MML string.
	mml += "}";

	return mml;
}

String TranslatorUtil::get_opl_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() == 0) {
		return "";
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opl, "#OPL@");
	if (alg_index == -1) {
		return "";
	}

	// Open MML string.
	String mml = "{";

	// Channel parameters.

	mml += _format_mml_digit(alg_index) + p_separator;
	mml += _format_mml_digit(p_params->feedback);


	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Operator parameters.

	for (int i = 0; i < p_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->get_operator_params(i);

		int wave_shape = _get_ma3_from_pg_type(op_params->pulse_generator_type, "#OPL@");
		if (wave_shape == -1) {
			return "";
		}

		mml += p_line_end;

		mml += _format_mml_digit(wave_shape)                       + p_separator;
		mml += _format_mml_digit(op_params->attack_rate >> 2, 2)   + p_separator;
		mml += _format_mml_digit(op_params->decay_rate >> 2, 2)    + p_separator;
		mml += _format_mml_digit(op_params->release_rate >> 2, 2)  + p_separator;
		mml += (op_params->sustain_rate == 0 ? "1" : "0")          + p_separator;
		mml += _format_mml_digit(op_params->sustain_level, 2)      + p_separator;

		int total_level = op_params->total_level < 63 ? op_params->total_level : 63;
		mml += _format_mml_digit(total_level, 2)                   + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_rate >> 1) + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_level)     + p_separator;
		mml += _format_mml_digit(op_params->get_multiple(), 2)     + p_separator;
		mml += _format_mml_digit(op_params->amplitude_modulation_shift);
	}

	// Close MML string.
	mml += "}";

	return mml;
}

String TranslatorUtil::get_opm_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() == 0) {
		return "";
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opm, "#OPM@");
	if (alg_index == -1) {
		return "";
	}

	// Open MML string.
	String mml = "{";

	// Channel parameters.

	mml += _format_mml_digit(alg_index) + p_separator;
	mml += _format_mml_digit(p_params->feedback);


	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Operator parameters.

	OperatorParamsSizes sizes = _get_operator_params_sizes(p_params);

	for (int i = 0; i < p_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->get_operator_params(i);

		mml += p_line_end;

		mml += _format_mml_digit(op_params->attack_rate >> 1, 2)            + p_separator;
		mml += _format_mml_digit(op_params->decay_rate >> 1, 2)             + p_separator;
		mml += _format_mml_digit(op_params->sustain_rate >> 1, 2)           + p_separator;
		mml += _format_mml_digit(op_params->release_rate >> 2, 2)           + p_separator;
		mml += _format_mml_digit(op_params->sustain_level, 2)               + p_separator;
		mml += _format_mml_digit(op_params->total_level, sizes.total_level) + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_level)              + p_separator;
		mml += _format_mml_digit(op_params->get_multiple(), 2)              + p_separator;
		mml += _format_mml_digit(op_params->detune1)                        + p_separator;

		int	detune2 = _get_nearest_dt2(op_params->detune2);
		mml += _format_mml_digit(detune2)                                   + p_separator;
		mml += _format_mml_digit(op_params->amplitude_modulation_shift);
	}

	// Close MML string.
	mml += "}";

	return mml;
}

String TranslatorUtil::get_opn_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() == 0) {
		return "";
	}

	// Note: OPM and OPN share the algo list.
	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opm, "#OPN@");
	if (alg_index == -1) {
		return "";
	}

	// Open MML string.
	String mml = "{";

	// Channel parameters.

	mml += _format_mml_digit(alg_index) + p_separator;
	mml += _format_mml_digit(p_params->feedback);

	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Operator parameters.

	OperatorParamsSizes sizes = _get_operator_params_sizes(p_params);

	for (int i = 0; i < p_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->get_operator_params(i);

		mml += p_line_end;

		mml += _format_mml_digit(op_params->attack_rate >> 1, 2)            + p_separator;
		mml += _format_mml_digit(op_params->decay_rate >> 1, 2)             + p_separator;
		mml += _format_mml_digit(op_params->sustain_rate >> 1, 2)           + p_separator;
		mml += _format_mml_digit(op_params->release_rate >> 2, 2)           + p_separator;
		mml += _format_mml_digit(op_params->sustain_level, 2)               + p_separator;
		mml += _format_mml_digit(op_params->total_level, sizes.total_level) + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_level)              + p_separator;
		mml += _format_mml_digit(op_params->get_multiple(), 2)              + p_separator;
		mml += _format_mml_digit(op_params->detune1)                        + p_separator;
		mml += _format_mml_digit(op_params->amplitude_modulation_shift);
	}

	// Close MML string.
	mml += "}";

	return mml;
}

String TranslatorUtil::get_opx_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() == 0) {
		return "";
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_opx, "#OPX@");
	if (alg_index == -1) {
		return "";
	}

	// Open MML string.
	String mml = "{";

	// Channel parameters.

	mml += _format_mml_digit(alg_index) + p_separator;
	mml += _format_mml_digit(p_params->feedback);

	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Operator parameters.

	OperatorParamsSizes sizes = _get_operator_params_sizes(p_params);

	for (int i = 0; i < p_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->get_operator_params(i);

		int wave_shape = _get_ma3_from_pg_type(op_params->pulse_generator_type, "#OPX@");
		if (wave_shape == -1) {
			return "";
		}

		mml += p_line_end;

		mml += _format_mml_digit(wave_shape)                                + p_separator;
		mml += _format_mml_digit(op_params->attack_rate >> 1, 2)            + p_separator;
		mml += _format_mml_digit(op_params->decay_rate >> 1, 2)             + p_separator;
		mml += _format_mml_digit(op_params->sustain_rate >> 1, 2)           + p_separator;
		mml += _format_mml_digit(op_params->release_rate >> 2, 2)           + p_separator;
		mml += _format_mml_digit(op_params->sustain_level, 2)               + p_separator;
		mml += _format_mml_digit(op_params->total_level, sizes.total_level) + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_level)              + p_separator;
		mml += _format_mml_digit(op_params->get_multiple(), 2)              + p_separator;
		mml += _format_mml_digit(op_params->detune1)                        + p_separator;
		mml += _format_mml_digit(op_params->detune2, sizes.detune2)         + p_separator;
		mml += _format_mml_digit(op_params->amplitude_modulation_shift);
	}

	// Close MML string.
	mml += "}";

	return mml;
}

String TranslatorUtil::get_ma3_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() == 0) {
		return "";
	}

	int alg_index = _get_algorithm_index(p_params->operator_count, p_params->algorithm, SiMMLRefTable::get_instance()->algorithm_ma3, "#MA@");
	if (alg_index == -1) {
		return "";
	}

	// Open MML string.
	String mml = "{";

	// Channel parameters.

	mml += _format_mml_digit(alg_index) + p_separator;
	mml += _format_mml_digit(p_params->feedback);

	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Operator parameters.

	for (int i = 0; i < p_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = p_params->get_operator_params(i);

		int wave_shape = _get_ma3_from_pg_type(op_params->pulse_generator_type, "#MA@");
		if (wave_shape == -1) {
			return "";
		}

		mml += p_line_end;

		mml += _format_mml_digit(wave_shape, 2)                    + p_separator;
		mml += _format_mml_digit(op_params->attack_rate >> 2, 2)   + p_separator;
		mml += _format_mml_digit(op_params->decay_rate >> 2, 2)    + p_separator;
		mml += _format_mml_digit(op_params->sustain_rate >> 2, 2)  + p_separator;
		mml += _format_mml_digit(op_params->release_rate >> 2, 2)  + p_separator;
		mml += _format_mml_digit(op_params->sustain_level, 2)      + p_separator;

		int total_level = op_params->total_level < 63 ? op_params->total_level : 63;
		mml += _format_mml_digit(total_level, 2)                   + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_rate >> 1) + p_separator;
		mml += _format_mml_digit(op_params->key_scaling_level)     + p_separator;
		mml += _format_mml_digit(op_params->get_multiple(), 2)     + p_separator;
		mml += _format_mml_digit(op_params->detune1)               + p_separator;
		mml += _format_mml_digit(op_params->amplitude_modulation_shift);
	}

	// Close MML string.
	mml += "}";

	return mml;
}

String TranslatorUtil::get_al_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator, String p_line_end, String p_comment) {
	if (p_params->get_operator_count() != 5) {
		return "";
	}

	Ref<SiOPMOperatorParams> op_params0 = p_params->operator_params[0];
	Ref<SiOPMOperatorParams> op_params1 = p_params->operator_params[1];

	// Open MML string.
	String mml = "{";

	// Leading parameters.

	mml += _format_mml_digit(p_params->algorithm)              + p_separator;
	mml += _format_mml_digit(op_params0->pulse_generator_type) + p_separator;
	mml += _format_mml_digit(op_params1->pulse_generator_type) + p_separator;

	int balanced_levels = _balance_total_levels(op_params0->total_level, op_params1->total_level);
	mml += _format_mml_digit(balanced_levels)                  + p_separator;
	mml += _format_mml_digit(op_params1->detune2)              + p_separator;

	// Custom comment message.

	mml += _format_mml_comment(p_comment, p_line_end);

	// Trailing parameters.

	mml += p_line_end;

	mml += _format_mml_digit(op_params0->attack_rate)   + p_separator;
	mml += _format_mml_digit(op_params0->decay_rate)    + p_separator;
	mml += _format_mml_digit(op_params0->sustain_level) + p_separator;
	mml += _format_mml_digit(op_params0->release_rate);

	// Close MML string.
	mml += "}";

	return mml;
}

void TranslatorUtil::parse_voice_setting(const Ref<SiMMLVoice> &p_voice, String p_mml, Vector<Ref<SiMMLEnvelopeTable>> p_envelopes) {
	Ref<SiOPMChannelParams> params = p_voice->channel_params;

	String base_re_exp = "(%[fvx]|@[fpqv]|@er|@lfo|kt?|m[ap]|_?@@|_?n[aptf]|po|p|q|s|x|v)";
	String args_re_exp = "(-?\\d*)" + String("(\\s*,\\s*(-?\\d*))?").repeat(10); // One mandatory and 10 optional arguments supported.

	Ref<RegEx> re_setting = RegEx::create_from_string(base_re_exp + args_re_exp);
	TypedArray<RegExMatch> settings = re_setting->search_all(p_mml);

	// For convenience macros take the ordered index of the argument, and convert it to the index of a capture group.
	// Capture group indices for arguments start at 2 and then continue with every even number up to 22 (11 arguments).

#define EXTRACT_ARGUMENT(m_index, m_default)                                                                                     \
	(!parsed_setting->get_string(2 + m_index * 2).is_empty() ? parsed_setting->get_string(2 + m_index * 2).to_int() : m_default)

#define EXTRACT_ARGUMENT_MOD(m_index, m_mod, m_default)                                                                                  \
	(!parsed_setting->get_string(2 + m_index * 2).is_empty() ? parsed_setting->get_string(2 + m_index * 2).to_int() * m_mod : m_default)

#define EXTRACT_ARGUMENT_POS(m_index, m_default)                                                                                  \
	(parsed_setting->get_string(2 + m_index * 2).to_int() > 0 ? parsed_setting->get_string(2 + m_index * 2).to_int() : m_default)

	for (int i = 0; i < settings.size(); i++) {
		Ref<RegExMatch> parsed_setting = settings[i];
		const String command = parsed_setting->get_string(1);

		if (command == "@f") {
			params->filter_cutoff         = EXTRACT_ARGUMENT(0, 128);
			params->filter_resonance      = EXTRACT_ARGUMENT(1, 0);
			params->filter_attack_rate    = EXTRACT_ARGUMENT(2, 0);
			params->filter_decay_rate1    = EXTRACT_ARGUMENT(3, 0);
			params->filter_decay_rate2    = EXTRACT_ARGUMENT(4, 0);
			params->filter_release_rate   = EXTRACT_ARGUMENT(5, 0);
			params->filter_decay_offset1  = EXTRACT_ARGUMENT(6, 128);
			params->filter_decay_offset2  = EXTRACT_ARGUMENT(7, 64);
			params->filter_sustain_offset = EXTRACT_ARGUMENT(8, 32);
			params->filter_release_offset = EXTRACT_ARGUMENT(9, 128);

		} else if (command == "@lfo") {
			params->set_lfo_frame(EXTRACT_ARGUMENT(0, 30));
			params->lfo_wave_shape = EXTRACT_ARGUMENT(1, SiOPMRefTable::LFO_WAVE_TRIANGLE);

		} else if (command == "ma") {
			p_voice->amplitude_modulation_depth     = EXTRACT_ARGUMENT(0, 0);
			p_voice->amplitude_modulation_depth_end = EXTRACT_ARGUMENT(1, 0);
			p_voice->amplitude_modulation_delay     = EXTRACT_ARGUMENT(2, 0);
			p_voice->amplitude_modulation_term      = EXTRACT_ARGUMENT(3, 0);

			params->amplitude_modulation_depth = p_voice->amplitude_modulation_depth;

		} else if (command == "mp") {
			p_voice->pitch_modulation_depth     = EXTRACT_ARGUMENT(0, 0);
			p_voice->pitch_modulation_depth_end = EXTRACT_ARGUMENT(1, 0);
			p_voice->pitch_modulation_delay     = EXTRACT_ARGUMENT(2, 0);
			p_voice->pitch_modulation_term      = EXTRACT_ARGUMENT(3, 0);

			params->pitch_modulation_depth = p_voice->pitch_modulation_depth;

		} else if (command == "po") {
			p_voice->portament = EXTRACT_ARGUMENT(0, 30);

		} else if (command == "q") {
			p_voice->default_gate_time = EXTRACT_ARGUMENT_MOD(0, 0.125, NAN);

		} else if (command == "s") {
			//[release rate] = EXTRACT_ARGUMENT(0, 0); // Is not implemented in the original code.
			p_voice->release_sweep = EXTRACT_ARGUMENT(2, 0);

		} else if (command == "%f") {
			params->filter_type = EXTRACT_ARGUMENT(0, 0);

		} else if (command == "@er") {
			for (int j = 0; j < 4; j++) {
				params->operator_params[j]->envelope_reset_on_attack = (parsed_setting->get_string(2) != "1");
			}

		} else if (command == "k") {
			p_voice->pitch_shift = EXTRACT_ARGUMENT(0, 0);

		} else if (command == "kt") {
			p_voice->note_shift = EXTRACT_ARGUMENT(0, 0);

		} else if (command == "@v") {
			params->master_volumes.write[0] = EXTRACT_ARGUMENT_MOD(0, 0.0078125, 0.5);
			params->master_volumes.write[1] = EXTRACT_ARGUMENT_MOD(1, 0.0078125, 0);
			params->master_volumes.write[2] = EXTRACT_ARGUMENT_MOD(2, 0.0078125, 0);
			params->master_volumes.write[3] = EXTRACT_ARGUMENT_MOD(3, 0.0078125, 0);
			params->master_volumes.write[4] = EXTRACT_ARGUMENT_MOD(4, 0.0078125, 0);
			params->master_volumes.write[5] = EXTRACT_ARGUMENT_MOD(5, 0.0078125, 0);
			params->master_volumes.write[6] = EXTRACT_ARGUMENT_MOD(6, 0.0078125, 0);
			params->master_volumes.write[7] = EXTRACT_ARGUMENT_MOD(7, 0.0078125, 0);

		} else if (command == "p") {
			params->pan = EXTRACT_ARGUMENT_MOD(0, 16, 64);

		} else if (command == "@p") {
			params->pan = EXTRACT_ARGUMENT(0, 64);

		} else if (command == "v") {
			if (!parsed_setting->get_string(2).is_empty()) {
				p_voice->velocity = parsed_setting->get_string(2).to_int() << p_voice->velocity_shift;
			} else {
				p_voice->velocity = 256;
			}

		} else if (command == "x") {
			p_voice->expression = EXTRACT_ARGUMENT(0, 128);

		} else if (command == "%v") {
			p_voice->velocity_mode  = EXTRACT_ARGUMENT(0, 0);
			p_voice->velocity_shift = EXTRACT_ARGUMENT(1, 4);

		} else if (command == "%x") {
			p_voice->expression_mode = EXTRACT_ARGUMENT(0, 0);

		} else if (command == "@q") {
			p_voice->default_gate_ticks         = EXTRACT_ARGUMENT(0, 0);
			p_voice->default_key_on_delay_ticks = EXTRACT_ARGUMENT(1, 0);

		} else if (command == "@@") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_on_tone_envelope = p_envelopes[i];
				p_voice->note_on_tone_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "na") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_on_amplitude_envelope = p_envelopes[i];
				p_voice->note_on_amplitude_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "np") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_on_pitch_envelope = p_envelopes[i];
				p_voice->note_on_pitch_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "nt") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_on_note_envelope = p_envelopes[i];
				p_voice->note_on_note_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "nf") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_on_filter_envelope = p_envelopes[i];
				p_voice->note_on_filter_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "_@@") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_off_tone_envelope = p_envelopes[i];
				p_voice->note_off_tone_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "_na") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_off_amplitude_envelope = p_envelopes[i];
				p_voice->note_off_amplitude_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "_np") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_off_pitch_envelope = p_envelopes[i];
				p_voice->note_off_pitch_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "_nt") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_off_note_envelope = p_envelopes[i];
				p_voice->note_off_note_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}

		} else if (command == "_nf") {
			int value = EXTRACT_ARGUMENT(0, 0);
			if (!p_envelopes.is_empty() && value >= 0 && value < 255) {
				p_voice->note_off_filter_envelope = p_envelopes[i];
				p_voice->note_off_filter_envelope_step = EXTRACT_ARGUMENT_POS(1, 1);
			}
		}
	}

#undef EXTRACT_ARGUMENT
#undef EXTRACT_ARGUMENT_MOD
#undef EXTRACT_ARGUMENT_POS
}

String TranslatorUtil::get_voice_setting_as_mml(const Ref<SiMMLVoice> &p_voice) {
	Ref<SiOPMChannelParams> params = p_voice->channel_params;
	String mml;

	if (params->filter_type > 0) {
		mml += "%f" + itos(params->filter_type);
	}
	if (params->has_filter() || params->has_filter_advanced()) {
		mml += vformat("@f%d,%d", params->filter_cutoff, params->filter_resonance);

		if (params->has_filter_advanced()) {
			mml += vformat(
				",%d,%d,%d,%d,%d,%d,%d,%d",
				params->filter_attack_rate,
				params->filter_decay_rate1,
				params->filter_decay_rate2,
				params->filter_release_rate,
				params->filter_decay_offset1,
				params->filter_decay_offset2,
				params->filter_sustain_offset,
				params->filter_release_offset
			);
		}
	}

	if (p_voice->has_amplitude_modulation() || params->has_amplitude_modulation() || p_voice->has_pitch_modulation() || params->has_pitch_modulation()) {
		if (params->get_lfo_frame() != 30 || params->lfo_wave_shape != SiOPMRefTable::LFO_WAVE_TRIANGLE) {
			mml += "@lfo" + itos(params->get_lfo_frame());

			if (params->lfo_wave_shape != SiOPMRefTable::LFO_WAVE_TRIANGLE) {
				mml += "," + itos(params->lfo_wave_shape);
			}
		}

		if (p_voice->has_amplitude_modulation()) {
			mml += vformat(
				"ma%d,%d,%d,%d",
				p_voice->amplitude_modulation_depth,
				p_voice->amplitude_modulation_depth_end,
				p_voice->amplitude_modulation_delay,
				p_voice->amplitude_modulation_term
			);
		} else if (params->has_amplitude_modulation()) {
			mml += "ma" + itos(params->amplitude_modulation_depth);
		}

		if (p_voice->has_pitch_modulation()) {
			mml += vformat(
				"mp%d,%d,%d,%d",
				p_voice->pitch_modulation_depth,
				p_voice->pitch_modulation_depth_end,
				p_voice->pitch_modulation_delay,
				p_voice->pitch_modulation_term
			);
		} else if (params->has_pitch_modulation()) {
			mml += "mp" + itos(params->pitch_modulation_depth);
		}
	}

	if (p_voice->velocity_mode != 0 || p_voice->velocity_shift != 4) {
		mml += vformat("%%v%d,%d", p_voice->velocity_mode, p_voice->velocity_shift);
	}

	if (p_voice->expression_mode != 0) {
		mml += "%x" + itos(p_voice->expression_mode);
	}

	if (p_voice->portament > 0) {
		mml += "po" + itos(p_voice->portament);
	}

	if (!Math::is_nan(p_voice->default_gate_time)) {
		mml += "q" + itos(p_voice->default_gate_time * 8);
	}

	if (p_voice->default_gate_ticks > 0 || p_voice->default_key_on_delay_ticks > 0) {
		mml += vformat("@q%d,%d", p_voice->default_gate_ticks, p_voice->default_key_on_delay_ticks);
	}

	if (p_voice->release_sweep > 0) {
		// First argument, release rate, is not implemented.
		mml += "s," + itos(p_voice->release_sweep);
	}

	if (params->operator_params[0]->envelope_reset_on_attack) {
		mml += "@er1";
	}

	if (p_voice->pitch_shift != 0) {
		mml += "k" + itos(p_voice->pitch_shift);
	}
	if (p_voice->note_shift != 0) {
		mml += "kt" + itos(p_voice->note_shift);
	}

	if (p_voice->update_volumes) {
		int volumes_count = (params->master_volumes[0] == 0.5 ? 0 : 1);
		for (int i = 1; i < 8; i++) {
			if (params->master_volumes[i] != 0) {
				volumes_count = i + 1;
			}
		}

		if (volumes_count > 0) {
			mml += "@v" + itos(params->master_volumes[0] * 128);
			for (int i = 1; i < volumes_count; i++) {
				mml += "," + itos(params->master_volumes[i] * 128);
			}
		}

		if (params->pan != 64) {
			if (params->pan & 15) {
				mml += "@p" + itos(params->pan - 64);
			} else {
				mml += "p" + itos(params->pan >> 4);
			}
		}

		if (p_voice->velocity != 256) {
			mml += "v" + itos(p_voice->velocity >> p_voice->velocity_shift);
		}
		if (p_voice->expression != 128) {
			mml += "x" + itos(p_voice->expression);
		}
	}

	return mml;
}

//

TranslatorUtil::MMLTableNumbers TranslatorUtil::parse_table_numbers(String p_table_numbers, String p_postfix, int p_max_index) {
	MMLTableNumbers parsed_table;
	parsed_table.data = memnew(SinglyLinkedList<int>);

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

	// match[1];(n..),m {match[2];n.., match[3];m} / match[4];n / match[5];|[] / match[6]; ]n
	Ref<RegEx> re_table = RegEx::create_from_string("(\\(\\s*([,\\-\\d\\s]+)\\)[,\\s]*(\\d+))|(-?\\d+)|(\\||\\[|\\](\\d*))");
	TypedArray<RegExMatch> numbers = re_table->search_all(p_table_numbers);

	SinglyLinkedList<int>::Element *repeat = nullptr;
	List<SinglyLinkedList<int>::Element *> loop_stack;

	int index = 0;
	for (int n = 0; n < numbers.size() && index < p_max_index; n++) {
		Ref<RegExMatch> parsed_number = numbers[n];

		// Interpolation: "(match[2]..),match[3]"
		if (!parsed_number->get_string(1).is_empty()) {
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
						parsed_table.data->append(value);
					}
					index += postfix_size;

					t += s;
				}
			} else {
				int value = (int)(inter_data[0] * postfix_coef + postfix_offset + 0.5);

				for (int i = 0; i < inter_size && index < p_max_index; i++) {
					for (int j = 0; j < postfix_size; j++) {
						parsed_table.data->append(value);
					}
					index += postfix_size;
				}
			}

		// Single number.
		} else if (!parsed_number->get_string(4).is_empty()) {
			int value = parsed_number->get_string(4).to_int();
			value = (int)(value * postfix_coef + postfix_offset + 0.5);

			for (int j = 0; j < postfix_size; j++) {
				parsed_table.data->append(value);
			}
			index++;

		// Loop control characters.
		} else if (!parsed_number->get_string(5).is_empty()) {
			const String token = parsed_number->get_string(5);

			// Loop repeat point.
			if (token == "|") {
				repeat = parsed_table.data->get();

			// Loop start.
			} else if (token == "[") {
				loop_stack.push_back(parsed_table.data->get());

			// Loop end.
			} else {
				ERR_FAIL_COND_V_MSG(loop_stack.is_empty(), parsed_table, "Translator: Failed to parse provided MML table, loop data is invalid.");

				SinglyLinkedList<int>::Element *loop_tail = parsed_table.data->get();
				SinglyLinkedList<int>::Element *loop_head = (loop_stack.back()->get())->next();
				loop_stack.pop_back();
				ERR_FAIL_COND_V_MSG(!loop_head, parsed_table, "Translator: Failed to parse provided MML table, loop data is invalid.");

				int loop_count = 2;
				if (!parsed_number->get_string(6).is_empty()) {
					loop_count = parsed_number->get_string(6).to_int();
				}

				for (int j = loop_count; j > 0; j--) {
					for (SinglyLinkedList<int>::Element *l = loop_head; l != loop_tail->next(); l = l->next()) {
						parsed_table.data->append(l->value);
					}
				}
			}
		} else {
			ERR_FAIL_V_MSG(parsed_table, "Translator: Failed to parse provided MML table, structure is invalid.");
		}
	}

	if (repeat) {
		parsed_table.data->loop(repeat->next());
	}

	parsed_table.length = index;
	parsed_table.repeated = (repeat != nullptr);
	return parsed_table;
}

void TranslatorUtil::parse_wav(String p_table_numbers, String p_postfix, Vector<double> *r_data) {
	MMLTableNumbers table = parse_table_numbers(p_table_numbers, p_postfix, 1024);

	int data_length = 2;
	while (data_length < 1024 && data_length < table.length) {
		data_length <<= 1;
	}
	r_data->resize_zeroed(data_length);

	int i = 0;
	table.data->front();
	for (; i < data_length && table.data->get(); i++) {
		double value = (table.data->get()->value + 0.5) * 0.0078125;
		r_data->write[i] = CLAMP(value, -1, 1);

		table.data->next();
	}

	for (; i < data_length; i++) {
		r_data->write[i] = 0;
	}
}

void TranslatorUtil::parse_wavb(String p_hex, Vector<double> *r_data) {
	Ref<RegEx> re_spaces = RegEx::create_from_string("\\s+");
	String hex = re_spaces->sub(p_hex, "", true);

	int data_length = hex.length() >> 1;
	r_data->resize_zeroed(data_length);

	for (int i = 0; i < data_length; i++) {
		int value = hex.substr(i << 1, 2).hex_to_int();
		if (value < 128) {
			r_data->write[i] = value * 0.0078125;
		} else {
			r_data->write[i] = (value - 256) * 0.0078125;
		}
	}
}

#define PARSE_ARGUMENT(m_index, m_default)                                                     \
	((m_index < args.size() && !args[m_index].is_empty()) ? args[m_index].to_int() : m_default)

bool TranslatorUtil::parse_sampler_wave(const Ref<SiOPMWaveSamplerTable> &p_table, int p_note_number, String p_mml, HashMap<String, Variant> p_sound_ref_table) {
	PackedStringArray args = split_string_by_regex(p_mml, "\\s*,\\s*");
	ERR_FAIL_COND_V(args.size() == 0, false);

	String wave_id = args[0];
	if (!p_sound_ref_table.has(wave_id)) {
		return false;
	}

	bool ignore_note_off = (bool)PARSE_ARGUMENT(1, 0);
	int pan              = PARSE_ARGUMENT(2, 0);
	int channel_count    = PARSE_ARGUMENT(3, 2);
	int start_point      = PARSE_ARGUMENT(4, -1);
	int end_point        = PARSE_ARGUMENT(5, -1);
	int loop_point       = PARSE_ARGUMENT(6, -1);

	Ref<SiOPMWaveSamplerData> sampler_data = memnew(SiOPMWaveSamplerData(p_sound_ref_table[wave_id], ignore_note_off, pan, 2, channel_count));
	sampler_data->slice(start_point, end_point, loop_point);
	p_table->set_sample(sampler_data, p_note_number);

	return true;
}

bool TranslatorUtil::parse_pcm_wave(const Ref<SiOPMWavePCMTable> &p_table, String p_mml, HashMap<String, Variant> p_sound_ref_table) {
	PackedStringArray args = split_string_by_regex(p_mml, "\\s*,\\s*");
	ERR_FAIL_COND_V(args.size() == 0, false);

	String wave_id = args[0];
	if (!p_sound_ref_table.has(wave_id)) {
		return false;
	}

	int sampling_pitch = PARSE_ARGUMENT(1, 69) * 64;
	int key_range_from = PARSE_ARGUMENT(2, 0);
	int key_range_to   = PARSE_ARGUMENT(3, 127);
	int channel_count  = PARSE_ARGUMENT(4, 2);
	int start_point    = PARSE_ARGUMENT(5, -1);
	int end_point      = PARSE_ARGUMENT(6, -1);
	int loop_point     = PARSE_ARGUMENT(7, -1);

	Ref<SiOPMWavePCMData> pcm_data = memnew(SiOPMWavePCMData(p_sound_ref_table[wave_id], sampling_pitch, 2, channel_count));
	pcm_data->slice(start_point, end_point, loop_point);
	p_table->set_key_range_data(pcm_data, key_range_from, key_range_to);

	return true;
}

bool TranslatorUtil::parse_pcm_voice(const Ref<SiMMLVoice> &p_voice, String p_mml, String p_postfix, Vector<Ref<SiMMLEnvelopeTable>> p_envelopes) {
	Ref<SiOPMWavePCMTable> table = p_voice->get_wave_data();
	if (table.is_null()) {
		return false;
	}

	PackedStringArray args = split_string_by_regex(p_mml, "\\s*,\\s*");

	int volume_note_number = PARSE_ARGUMENT(0, 64);
	int volume_key_range   = PARSE_ARGUMENT(1, 0);
	int volume_range       = PARSE_ARGUMENT(2, 0);
	int pan_note_number    = PARSE_ARGUMENT(3, 64);
	int pan_key_range      = PARSE_ARGUMENT(4, 0);
	int pan_width          = PARSE_ARGUMENT(5, 0);
	int attack_rate        = PARSE_ARGUMENT(6, 63);
	int decay_rate         = PARSE_ARGUMENT(7, 0);
	int sustain_rate       = PARSE_ARGUMENT(8, 0);
	int release_rate       = PARSE_ARGUMENT(9, 63);
	int sustain_level      = PARSE_ARGUMENT(10, 0);

	Ref<SiOPMOperatorParams> op_params = p_voice->get_channel_params()->operator_params[0];
	op_params->attack_rate = attack_rate;
	op_params->decay_rate = decay_rate;
	op_params->sustain_rate = sustain_rate;
	op_params->release_rate = release_rate;
	op_params->sustain_level = sustain_level;

	table->set_key_scale_volume(volume_note_number, volume_key_range, volume_range);
	table->set_key_scale_pan(pan_note_number, pan_key_range, pan_width);
	parse_voice_setting(p_voice, p_postfix, p_envelopes);

	return true;
}

#undef PARSE_ARGUMENT

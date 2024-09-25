/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_TRANSLATOR_UTIL_H
#define SION_TRANSLATOR_UTIL_H

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include "sequencer/base/mml_system_command.h"
#include "templates/singly_linked_list.h"

using namespace godot;

class SiMMLEnvelopeTable;
class SiMMLVoice;
class SiONVoice;
class SiOPMChannelParams;
class SiOPMWavePCMTable;
class SiOPMWaveSamplerTable;

class TranslatorUtil {

	static Vector<int> _split_data_string(const Ref<SiOPMChannelParams> &p_params, String p_data_string, int p_channel_param_count, int p_operator_param_count, const String &p_command);
	static void _check_operator_count(const Ref<SiOPMChannelParams> &p_params, int p_data_length, int p_channel_param_count, int p_operator_param_count, const String &p_command);

	static int _get_params_algorithm(int (&p_algorithms)[4][16], int p_operator_count, int p_data_value, const String &p_command);

	static void _set_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void _set_opl_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void _set_opm_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void _set_opn_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void _set_opx_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void _set_ma3_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void _set_al_params_by_array(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);

	static int _get_algorithm_index(int p_operator_count, int p_algorithm, int (&p_table)[4][16], const String &p_command);
	static int _get_ma3_from_pg_type(int p_pulse_generator_type, const String &p_command);
	static int _get_nearest_dt2(int p_detune);
	static int _balance_total_levels(int p_level0, int p_level1);

	struct OperatorParamsSizes {
		int pg_type = 1;
		int total_level = 2;
		int detune = 1;
		int phase = 1;
		int fixed_pitch = 1;
	};

	static String _format_mml_comment(const String &p_comment, const String &p_line_end);
	static String _format_mml_digit(int p_value, int p_padded = 0);
	static OperatorParamsSizes _get_operator_params_sizes(const Ref<SiOPMChannelParams> &p_params);

public:
	// Channel params.

	static void parse_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);
	static void parse_opl_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);
	static void parse_opm_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);
	static void parse_opn_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);
	static void parse_opx_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);
	static void parse_ma3_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);
	static void parse_al_params(const Ref<SiOPMChannelParams> &p_params, const String &p_data_string);

	static void set_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void set_opl_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void set_opm_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void set_opn_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void set_opx_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void set_ma3_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);
	static void set_al_params(const Ref<SiOPMChannelParams> &p_params, Vector<int> p_data);

	static Vector<int> get_params(const Ref<SiOPMChannelParams> &p_params);
	static Vector<int> get_opl_params(const Ref<SiOPMChannelParams> &p_params);
	static Vector<int> get_opm_params(const Ref<SiOPMChannelParams> &p_params);
	static Vector<int> get_opn_params(const Ref<SiOPMChannelParams> &p_params);
	static Vector<int> get_opx_params(const Ref<SiOPMChannelParams> &p_params);
	static Vector<int> get_ma3_params(const Ref<SiOPMChannelParams> &p_params);
	static Vector<int> get_al_params(const Ref<SiOPMChannelParams> &p_params);

	static String get_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());
	static String get_opl_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());
	static String get_opm_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());
	static String get_opn_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());
	static String get_opx_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());
	static String get_ma3_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());
	static String get_al_params_as_mml(const Ref<SiOPMChannelParams> &p_params, String p_separator = " ", String p_line_end = "\n", String p_comment = String());

	static void parse_voice_setting(const Ref<SiMMLVoice> &p_voice, String p_mml, Vector<Ref<SiMMLEnvelopeTable>> p_envelopes = Vector<Ref<SiMMLEnvelopeTable>>());
	static String get_voice_setting_as_mml(const Ref<SiMMLVoice> &p_voice);

	//

	static List<Ref<MMLSystemCommand>> extract_system_command(String p_mml);

	struct MMLTableNumbers {
		SinglyLinkedList<int> *head =  nullptr;
		SinglyLinkedList<int> *tail =  nullptr;
		int length = 0;
		bool repeated = false;
	};

	static MMLTableNumbers parse_table_numbers(String p_table_numbers, String p_postfix, int p_max_index = 65536);

	static void parse_wav(String p_table_numbers, String p_postfix, Vector<double> *r_data);
	static void parse_wavb(String p_hex, Vector<double> *r_data);

	// TODO: The sound reference table is mostly needed for passing a map of Flash Sound objects. Some code changes may be needed in places that utilize that.
	static bool parse_sampler_wave(const Ref<SiOPMWaveSamplerTable> &p_table, int p_note_number, String p_mml, HashMap<String, Variant> p_sound_ref_table);
	static bool parse_pcm_wave(const Ref<SiOPMWavePCMTable> &p_table, String p_mml, HashMap<String, Variant> p_sound_ref_table);
	static bool parse_pcm_voice(const Ref<SiMMLVoice> &p_voice, String p_mml, String p_postfix, Vector<Ref<SiMMLEnvelopeTable>> p_envelopes = Vector<Ref<SiMMLEnvelopeTable>>());
};

#endif // SION_TRANSLATOR_UTIL_H

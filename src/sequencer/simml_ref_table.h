/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_REF_TABLE_H
#define SIMML_REF_TABLE_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "sequencer/simml_voice.h"

using namespace godot;

class SiMMLChannelSettings;
class SiMMLEnvelopeTable;
enum SiONModuleType : unsigned int;

// Reference data object for the sequencer and related operations.
class SiMMLRefTable {

	static SiMMLRefTable *_instance;

	Vector<SiMMLEnvelopeTable *> _master_envelopes;
	Vector<Ref<SiMMLVoice>> _master_voices;
	Vector<SiMMLEnvelopeTable *> _stencil_envelopes;
	Vector<Ref<SiMMLVoice>> _stencil_voices;

	void _fill_tss_log_table(String (&r_table)[256], int p_start, int p_step, int p_v0, int p_v255);

	template <size_t S>
	Vector<Ref<SiMMLVoice>> _setup_ym2413_default_voices(uint32_t (&p_register_map)[S]);
	void _dump_ym2413_register(const Ref<SiMMLVoice> &p_voice, uint32_t p_u0, uint32_t p_u1);

public:
	static const int ENVELOPE_TABLE_MAX = 512;
	static const int VOICE_MAX = 256;

	// All reference properties are made public to simplify code.
	// These are not expected to be written to externally. But that might happen.

	HashMap<SiONModuleType, SiMMLChannelSettings *> channel_settings_map;

	// Mapping from tsscp @s command to OPM attack rate.
	String tss_scmd_to_attack_rate[256];
	// Mapping from tsscp @s command to OPM decay rate.
	String tss_scmd_to_decay_rate[256];
	// Mapping from tsscp @s command to OPM sustain rate.
	String tss_scmd_to_sustain_rate[256];
	// Mapping from tsscp @s command to OPM release rate.
	String tss_scmd_to_release_rate[256];

	// Table of OPLL preset voices (from virturenes)
	uint32_t preset_register_ym2413[32] = {
		0x00000000, 0x00000000, 0x61611e17, 0xf07f0717, 0x13410f0d, 0xced24313, 0x03019904, 0xffc30373,
		0x21611b07, 0xaf634028, 0x22211e06, 0xf0760828, 0x31221605, 0x90710018, 0x21611d07, 0x82811017,
		0x23212d16, 0xc0700707, 0x61211b06, 0x64651818, 0x61610c18, 0x85a07907, 0x23218711, 0xf0a400f7,
		0x97e12807, 0xfff302f8, 0x61100c05, 0xf2c440c8, 0x01015603, 0xb4b22358, 0x61418903, 0xf1f4f013
	};
	// Table of VRC7 preset voices (from virturenes)
	uint32_t preset_register_vrc7[32] = {
		0x00000000, 0x00000000, 0x3301090e, 0x94904001, 0x13410f0d, 0xced34313, 0x01121b06, 0xffd20032,
		0x61611b07, 0xaf632028, 0x22211e06, 0xf0760828, 0x66211500, 0x939420f8, 0x21611c07, 0x82811017,
		0x2321201f, 0xc0710747, 0x25312605, 0x644118f8, 0x17212807, 0xff8302f8, 0x97812507, 0xcfc80214,
		0x2121540f, 0x807f0707, 0x01015603, 0xd3b24358, 0x31210c03, 0x82c04007, 0x21010c03, 0xd4d34084
	};
	// Table of VRC7/OPLL preset drums (from virturenes)
	uint32_t preset_register_vrc7_drums[6] = {
		0x04212800, 0xdff8fff8, 0x23220000, 0xd8f8f8f8, 0x25180000, 0xf8daf855
	};
	// Preset voice set of OPLL
	Vector<Ref<SiMMLVoice>> preset_voice_ym2413;
	// Preset voice set of VRC7
	Vector<Ref<SiMMLVoice>> preset_voice_vrc7;
	// Preset voice set of VRC7/OPLL drum
	Vector<Ref<SiMMLVoice>> preset_voice_vrc7_drums;

	// Algorithm table for OPM/OPN.
	int algorithm_opm[4][16] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 1, 1, 1, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 1, 2, 3, 3, 4, 3, 5,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 1, 2, 3, 4, 5, 6, 7,-1,-1,-1,-1,-1,-1,-1,-1}
	};
	// Algorithm table for OPL3.
	int algorithm_opl[4][16] = {
		{ 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 3, 2, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 4, 8, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}
	};
	// Algorithm table for MA3.
	int algorithm_ma3[4][16] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0, 1, 1, 1, 0, 1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1},
		{-1,-1, 5, 2, 0, 3, 2, 2,-1,-1,-1,-1,-1,-1,-1,-1},
		{-1,-1, 7, 2, 0, 4, 8, 9,-1,-1,-1,-1,-1,-1,-1,-1}
	};
	// Algorithm table for OPX. LSB4 is the flag of feedback connection.
	int algorithm_opx[4][16] = {
		{ 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0,16, 1, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0,16, 1, 2, 3,19, 5, 6,-1,-1,-1,-1,-1,-1,-1,-1},
		{ 0,16, 1, 2, 3,19, 4,20, 8,11, 6,22, 5, 9,12, 7}
	};
	// Initial connection.
	int algorithm_init[4] = { 0,1,5,7 };

	//

	static SiMMLRefTable *get_instance() { return _instance; }
	static void initialize();
	static void finalize();

	//

	void reset_all_user_tables();
	void register_master_envelope_table(int p_index, SiMMLEnvelopeTable *p_table);
	void register_master_voice(int p_index, const Ref<SiMMLVoice> &p_voice);

	void set_stencil_envelopes(Vector<SiMMLEnvelopeTable *> p_tables) { _stencil_envelopes = p_tables; }
	void clear_stencil_envelopes() { _stencil_envelopes = Vector<SiMMLEnvelopeTable *>(); }
	void set_stencil_voices(Vector<Ref<SiMMLVoice>> p_tables) { _stencil_voices = p_tables; }
	void clear_stencil_voices() { _stencil_voices = Vector<Ref<SiMMLVoice>>(); }

	SiMMLEnvelopeTable *get_envelope_table(int p_index);
	Ref<SiMMLVoice> get_voice(int p_index);

	int get_pulse_generator_type(SiONModuleType p_module_type, int p_channel_num, int p_tone_num = -1);
	bool is_suitable_for_fm_voice(SiONModuleType p_module_type);

	//

	SiMMLRefTable();
	~SiMMLRefTable();
};

#endif // SIMML_REF_TABLE_H

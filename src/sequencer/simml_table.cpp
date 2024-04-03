/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_table.h"

#include <godot_cpp/core/memory.hpp>
#include "sion_voice.h"
#include "processor/channels/siopm_channel_manager.h"
#include "processor/siopm_channel_params.h"
#include "processor/siopm_operator_params.h"
#include "processor/siopm_table.h"
#include "sequencer/simml_channel_settings.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simulator/simml_simulator_base.h"
#include "sequencer/simml_voice.h"

#include "sequencer/simulator/simml_simulator_apu.h"
#include "sequencer/simulator/simml_simulator_fm_ma3.h"
#include "sequencer/simulator/simml_simulator_fm_opl3.h"
#include "sequencer/simulator/simml_simulator_fm_opll.h"
#include "sequencer/simulator/simml_simulator_fm_opm.h"
#include "sequencer/simulator/simml_simulator_fm_opn.h"
#include "sequencer/simulator/simml_simulator_fm_opna.h"
#include "sequencer/simulator/simml_simulator_fm_siopm.h"
#include "sequencer/simulator/simml_simulator_gb.h"
#include "sequencer/simulator/simml_simulator_ks.h"
#include "sequencer/simulator/simml_simulator_ma3_wave_table.h"
#include "sequencer/simulator/simml_simulator_noise.h"
#include "sequencer/simulator/simml_simulator_pcm.h"
#include "sequencer/simulator/simml_simulator_psg.h"
#include "sequencer/simulator/simml_simulator_pulse.h"
#include "sequencer/simulator/simml_simulator_ramp.h"
#include "sequencer/simulator/simml_simulator_sampler.h"
#include "sequencer/simulator/simml_simulator_sid.h"
#include "sequencer/simulator/simml_simulator_siopm.h"
#include "sequencer/simulator/simml_simulator_vrc6.h"
#include "sequencer/simulator/simml_simulator_wt.h"

using namespace godot;

SiMMLTable *SiMMLTable::_instance = nullptr;

void SiMMLTable::initialize() {
	if (_instance) {
		return;
	}

	// Sets the instance internally.
	memnew(SiMMLTable);
}

//

void SiMMLTable::reset_all_user_tables() {
	for (int i = 0; i < ENVELOPE_TABLE_MAX; i++) {
		if (_master_envelopes[i]) {
			_master_envelopes[i]->free();
			_master_envelopes.write[i] = nullptr;
		}
	}

	for (int i = 0; i < VOICE_MAX; i++) {
		// FIXME: Memory leak?
		_master_voices.write[i] = nullptr;
	}
}

void SiMMLTable::register_master_envelope_table(int p_index, SiMMLEnvelopeTable *p_table) {
	ERR_FAIL_INDEX(p_index, ENVELOPE_TABLE_MAX);
	_master_envelopes.write[p_index] = p_table;
}

void SiMMLTable::register_master_voice(int p_index, SiMMLVoice *p_voice) {
	ERR_FAIL_INDEX(p_index, VOICE_MAX);
	_master_voices.write[p_index] = p_voice;
}

SiMMLEnvelopeTable *SiMMLTable::get_envelope_table(int p_index) {
	ERR_FAIL_INDEX_V(p_index, ENVELOPE_TABLE_MAX, nullptr);

	if (p_index < _stencil_envelopes.size() && _stencil_envelopes[p_index]) {
		return _stencil_envelopes[p_index];
	}
	return _master_envelopes[p_index];
}

SiMMLVoice *SiMMLTable::get_voice(int p_index) {
	ERR_FAIL_INDEX_V(p_index, VOICE_MAX, nullptr);

	if (p_index < _stencil_voices.size() && _stencil_voices[p_index]) {
		return _stencil_voices[p_index];
	}
	return _master_voices[p_index];
}

int SiMMLTable::get_pulse_generator_type(ModuleType p_module_type, int p_channel_num, int p_tone_num) {
	SiMMLChannelSettings *channel_settings = channel_settings_map[p_module_type];
	ERR_FAIL_COND_V(!channel_settings, -1);

	if (!channel_settings->is_select_tone_type(SiMMLChannelSettings::SELECT_TONE_NORMAL)) {
		return -1;
	}

	int tone_num = p_tone_num;

	Vector<int> voice_index_table = channel_settings->get_voice_index_table();
	if (tone_num == -1 && p_channel_num >= 0 && p_channel_num < voice_index_table.size()) {
		tone_num = voice_index_table[p_channel_num];
	}

	Vector<int> pg_type_list = channel_settings->get_pg_type_list();
	if (tone_num < 0 || tone_num >= pg_type_list.size()) {
		tone_num = channel_settings->get_initial_voice_index();
	}

	return pg_type_list[tone_num];
}

bool SiMMLTable::is_suitable_for_fm_voice(ModuleType p_module_type) {
	SiMMLChannelSettings *channel_settings = channel_settings_map[p_module_type];
	ERR_FAIL_COND_V(!channel_settings, false);

	return channel_settings->is_suitable_for_fm_voice();
}

//

void SiMMLTable::_fill_tss_log_table(String (&r_table)[256], int p_start, int p_step, int p_v0, int p_v255) {
	int value = p_start << 16;
	int step = p_step << 16;

	for (int i = 1, j = 1; j <= 8; j++) {
		int row_max = 1 << j;
		for (; i < row_max; i++) {
			r_table[i] = itos(value >> 16);
			value += step;
		}

		step >>= 1;
	}

	r_table[0] = itos(p_v0);
	r_table[255] = itos(p_v255);
}

template <size_t S>
Vector<SiMMLVoice *> SiMMLTable::_setup_ym2413_default_voices(uint32_t (&p_register_map)[S]) {
	Vector<SiMMLVoice *> voices;
	voices.resize_zeroed(S >> 1);

	for (int i = 0, j = 0; i < voices.size(); i++, j += 2) {
		SiMMLVoice *voice = memnew(SiMMLVoice);
		_dump_ym2413_register(voice, p_register_map[j], p_register_map[j + 1]);
		voices.write[i] = voice;
	}

	return voices;
}

void SiMMLTable::_dump_ym2413_register(SiMMLVoice *p_voice, uint32_t p_u0, uint32_t p_u1) {
	SiOPMChannelParams *channel_params = p_voice->get_channel_params();
	SiOPMOperatorParams *op_params0 = channel_params->get_operator_params(0);
	SiOPMOperatorParams *op_params1 = channel_params->get_operator_params(1);

	p_voice->set_module_type(SiMMLTable::MT_FM);
	p_voice->set_channel_num(0);
	p_voice->set_tone_num(-1);
	p_voice->set_chip_type(SiONVoice::CHIPTYPE_OPL);

	channel_params->set_envelope_frequency_ratio(133);
	channel_params->set_operator_count(2);
	channel_params->set_algorithm(0);

	op_params0->set_amplitude_modulation_shift(((p_u0 >> 31) & 1) << 1);
	op_params1->set_amplitude_modulation_shift(((p_u0 >> 23) & 1) << 1);
	op_params0->set_key_scaling_rate(((p_u0 >> 28) & 1) << 1);
	op_params1->set_key_scaling_rate(((p_u0 >> 20) & 1) << 1);

	int i = (p_u0 >> 24) & 15;
	op_params0->set_multiple((i==11 || i==13) ? (i-1) : (i==14) ? (i+1) : i);
	i = (p_u0 >> 16) & 15;
	op_params1->set_multiple((i==11 || i==13) ? (i-1) : (i==14) ? (i+1) : i);

	op_params0->set_key_scaling_level((p_u0 >> 14) & 3);
	op_params1->set_key_scaling_level((p_u0 >> 6) & 3);

	channel_params->set_feedback((p_u0 >> 0) & 7);

	op_params0->set_pulse_generator_type(SiOPMTable::PG_MA3_WAVE + ((p_u0 >> 3)&1));
	op_params1->set_pulse_generator_type(SiOPMTable::PG_MA3_WAVE + ((p_u0 >> 4)&1));

	op_params0->set_attack_rate(((p_u1 >> 28) & 15) << 2);
	op_params1->set_attack_rate(((p_u1 >> 20) & 15) << 2);
	op_params0->set_decay_rate(((p_u1 >> 24) & 15) << 2);
	op_params1->set_decay_rate(((p_u1 >> 16) & 15) << 2);
	op_params0->set_sustain_level((p_u1 >> 12) & 15);
	op_params1->set_sustain_level((p_u1 >> 4) & 15);
	op_params0->set_release_rate(((p_u1 >> 8) & 15) << 2);
	op_params1->set_release_rate(((p_u1 >> 0) & 15) << 2);
	op_params0->set_sustain_rate((((p_u0 >> 29) & 1) != 0) ? 0 : op_params0->get_release_rate());
	op_params1->set_sustain_rate((((p_u0 >> 21) & 1) != 0) ? 0 : op_params1->get_release_rate());
	op_params0->set_total_level((p_u0 >> 8) & 63);
	op_params1->set_total_level(0);
}

SiMMLTable::SiMMLTable() {
	if (!_instance) {
		_instance = this; // Do this early so it can be self-referenced.
	}

	// Channel module settings map.
	{
		channel_settings_map[MT_PSG]    = memnew(SiMMLChannelSettings(MT_PSG,    SiOPMTable::PG_SQUARE,      3,   1, 4));   // PSG
		channel_settings_map[MT_APU]    = memnew(SiMMLChannelSettings(MT_APU,    SiOPMTable::PG_PULSE,       11,  2, 4));   // FC pAPU
		channel_settings_map[MT_NOISE]  = memnew(SiMMLChannelSettings(MT_NOISE,  SiOPMTable::PG_NOISE_WHITE, 16,  1, 16));  // noise
		channel_settings_map[MT_MA3]    = memnew(SiMMLChannelSettings(MT_MA3,    SiOPMTable::PG_MA3_WAVE,    32,  1, 32));  // MA3
		channel_settings_map[MT_CUSTOM] = memnew(SiMMLChannelSettings(MT_CUSTOM, SiOPMTable::PG_CUSTOM,      256, 1, 256)); // SCC / custom wave table
		channel_settings_map[MT_ALL]    = memnew(SiMMLChannelSettings(MT_ALL,    SiOPMTable::PG_SINE,        512, 1, 512)); // all pgTypes
		channel_settings_map[MT_FM]     = memnew(SiMMLChannelSettings(MT_FM,     SiOPMTable::PG_SINE,        1,   1, 1));   // FM sound module
		channel_settings_map[MT_PCM]    = memnew(SiMMLChannelSettings(MT_PCM,    SiOPMTable::PG_PCM,         128, 1, 128)); // PCM
		channel_settings_map[MT_PULSE]  = memnew(SiMMLChannelSettings(MT_PULSE,  SiOPMTable::PG_PULSE,       32,  1, 32));  // pulse
		channel_settings_map[MT_RAMP]   = memnew(SiMMLChannelSettings(MT_RAMP,   SiOPMTable::PG_RAMP,        128, 1, 128)); // ramp
		channel_settings_map[MT_SAMPLE] = memnew(SiMMLChannelSettings(MT_SAMPLE, 0,                          4,   1, 4));   // sampler. this is based on SiOPMChannelSampler
		channel_settings_map[MT_KS]     = memnew(SiMMLChannelSettings(MT_KS,     0,                          3,   1, 3));   // karplus strong (0-2 to choose seed generator algrithm)
		channel_settings_map[MT_GB]     = memnew(SiMMLChannelSettings(MT_GB,     SiOPMTable::PG_PULSE,       11,  2, 4));   // Gameboy
		channel_settings_map[MT_VRC6]   = memnew(SiMMLChannelSettings(MT_VRC6,   SiOPMTable::PG_PULSE,       9,   1, 3));   // VRC6
		channel_settings_map[MT_SID]    = memnew(SiMMLChannelSettings(MT_SID,    SiOPMTable::PG_PULSE,       12,  1, 3));   // SID

		// PSG settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_PSG];
			cs->set_pg_type(0, SiOPMTable::PG_SQUARE);
			cs->set_pg_type(1, SiOPMTable::PG_NOISE_PULSE);
			cs->set_pg_type(2, SiOPMTable::PG_PC_NZ_16BIT);

			cs->set_pt_type(0, SiOPMTable::PT_PSG);
			cs->set_pt_type(1, SiOPMTable::PT_PSG_NOISE);
			cs->set_pt_type(2, SiOPMTable::PT_PSG);

			cs->set_voice_index(0, 0);
			cs->set_voice_index(1, 0);
			cs->set_voice_index(2, 0);
			cs->set_voice_index(3, 1);
		}

		// APU settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_APU];
			cs->set_pg_type(8, SiOPMTable::PG_TRIANGLE_FC);
			cs->set_pg_type(9, SiOPMTable::PG_NOISE_PULSE);
			cs->set_pg_type(10, SiOPMTable::PG_NOISE_SHORT);

			for (int i = 0; i < 9;  i++) {
				cs->set_pt_type(i, SiOPMTable::PT_PSG);
			}
			for (int i = 9; i < 11; i++) {
				cs->set_pt_type(i, SiOPMTable::PT_APU_NOISE);
			}

			cs->set_initial_voice_index(1);
			cs->set_voice_index(0, 4);
			cs->set_voice_index(1, 4);
			cs->set_voice_index(2, 8);
			cs->set_voice_index(3, 9);
		}

		// GB settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_GB];
			cs->set_pg_type(8, SiOPMTable::PG_CUSTOM);
			cs->set_pg_type(9, SiOPMTable::PG_NOISE_PULSE);
			cs->set_pg_type(10, SiOPMTable::PG_NOISE_GB_SHORT);

			for (int i = 0; i < 9;  i++) {
				cs->set_pt_type(i, SiOPMTable::PT_PSG);
			}
			for (int i = 9; i < 11; i++) {
				cs->set_pt_type(i, SiOPMTable::PT_GB_NOISE);
			}

			cs->set_initial_voice_index(1);
			cs->set_voice_index(0, 4);
			cs->set_voice_index(1, 4);
			cs->set_voice_index(2, 8);
			cs->set_voice_index(3, 9);
		}

		// VRC6 settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_VRC6];
			cs->set_pg_type(8, SiOPMTable::PG_SAW_VC6);
			cs->set_pt_type(8, SiOPMTable::PT_PSG);

			cs->set_initial_voice_index(1);
			cs->set_voice_index(0, 7);
			cs->set_voice_index(1, 7);
			cs->set_voice_index(2, 8);
		}

		// SID settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_SID];
			cs->set_pg_type(8, SiOPMTable::PG_TRIANGLE);
			cs->set_pg_type(9, SiOPMTable::PG_SAW_UP);
			cs->set_pg_type(10, SiOPMTable::PG_SAW_VC6);
			cs->set_pg_type(11, SiOPMTable::PG_NOISE_PULSE);

			for (int i = 0; i < 11;  i++) {
				cs->set_pt_type(i, SiOPMTable::PT_PSG);
			}
			cs->set_pt_type(11, SiOPMTable::PT_OPM_NOISE);

			cs->set_initial_voice_index(1);
			cs->set_voice_index(0, 7);
			cs->set_voice_index(1, 7);
			cs->set_voice_index(2, 7);
		}

		// FM settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_FM];
			cs->set_select_tone_type(SiMMLChannelSettings::SELECT_TONE_FM);
			cs->set_suitable_for_fm_voice(false);
		}

		// PCM settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_PCM];
			cs->set_channel_type(SiOPMChannelManager::CT_CHANNEL_PCM);
			cs->set_suitable_for_fm_voice(false);
		}

		// Sampler settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_SAMPLE];
			cs->set_channel_type(SiOPMChannelManager::CT_CHANNEL_SAMPLER);
			cs->set_suitable_for_fm_voice(false);
		}

		// Karplus strong settings.
		{
			SiMMLChannelSettings *cs = channel_settings_map[MT_KS];
			cs->set_channel_type(SiOPMChannelManager::CT_CHANNEL_KS);
			cs->set_suitable_for_fm_voice(false);
		}
	}

	// Simulators map.
	{
		channel_simulator_map[MT_PSG]     = memnew(SiMMLSimulatorPSG);          // PSG(DCSG)
		channel_simulator_map[MT_APU]     = memnew(SiMMLSimulatorAPU);          // FC pAPU
		channel_simulator_map[MT_NOISE]   = memnew(SiMMLSimulatorNoise);        // noise wave
		channel_simulator_map[MT_MA3]     = memnew(SiMMLSimulatorMA3WaveTable); // MA3 wave form
		channel_simulator_map[MT_CUSTOM]  = memnew(SiMMLSimulatorWT);           // SCC / custom wave table
		channel_simulator_map[MT_ALL]     = memnew(SiMMLSimulatorSiOPM);        // all pgTypes
		channel_simulator_map[MT_FM]      = memnew(SiMMLSimulatorFMSiOPM);      // FM sound module
		channel_simulator_map[MT_PCM]     = memnew(SiMMLSimulatorPCM);          // PCM
		channel_simulator_map[MT_PULSE]   = memnew(SiMMLSimulatorPulse);        // pulse wave
		channel_simulator_map[MT_RAMP]    = memnew(SiMMLSimulatorRamp);         // ramp wave
		channel_simulator_map[MT_SAMPLE]  = memnew(SiMMLSimulatorSampler);      // sampler
		channel_simulator_map[MT_KS]      = memnew(SiMMLSimulatorKS);           // karplus strong
		channel_simulator_map[MT_GB]      = memnew(SiMMLSimulatorGB);           // gameboy
		channel_simulator_map[MT_VRC6]    = memnew(SiMMLSimulatorVRC6);         // vrc6
		channel_simulator_map[MT_SID]     = memnew(SiMMLSimulatorSID);          // sid
		channel_simulator_map[MT_FM_OPM]  = memnew(SiMMLSimulatorFMOPM);        // YM2151
		channel_simulator_map[MT_FM_OPN]  = memnew(SiMMLSimulatorFMOPN);        // YM2203
		channel_simulator_map[MT_FM_OPNA] = memnew(SiMMLSimulatorFMOPNA);       // YM2608
		channel_simulator_map[MT_FM_OPLL] = memnew(SiMMLSimulatorFMOPLL);       // YM2413
		channel_simulator_map[MT_FM_OPL3] = memnew(SiMMLSimulatorFMOPL3);       // YM3812
		channel_simulator_map[MT_FM_MA3]  = memnew(SiMMLSimulatorFMMA3);        // YMU762
	}

	// OPLL default voices.
	{
		preset_voice_ym2413     = _setup_ym2413_default_voices(preset_register_ym2413);
		preset_voice_vrc7       = _setup_ym2413_default_voices(preset_register_vrc7);
		preset_voice_vrc7_drums = _setup_ym2413_default_voices(preset_register_vrc7_drums);
	}

	// Master tables.
	{
		_master_envelopes.resize_zeroed(ENVELOPE_TABLE_MAX);
		_master_envelopes.fill(nullptr);
		_master_voices.resize_zeroed(VOICE_MAX);
		_master_voices.fill(nullptr);
	}

	// TSSCP maps.
	{
		// Tuned subjectively to personal preference.
		_fill_tss_log_table(tss_scmd_to_attack_rate, 41, -4, 63, 9);
		_fill_tss_log_table(tss_scmd_to_decay_rate, 52, -4,  0, 20);
		_fill_tss_log_table(tss_scmd_to_sustain_rate, 9,  5,  0, 63);
		_fill_tss_log_table(tss_scmd_to_release_rate, 12,  4, 63, 63);
	}
}

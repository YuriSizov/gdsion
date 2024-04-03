/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "sion_voice.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/core/memory.hpp>

#include "processor/siopm_channel_params.h"
#include "processor/siopm_operator_params.h"
#include "processor/siopm_table.h"
#include "processor/wave/siopm_wave_pcm_data.h"
#include "processor/wave/siopm_wave_pcm_table.h"
#include "processor/wave/siopm_wave_sampler_data.h"
#include "processor/wave/siopm_wave_sampler_table.h"
#include "processor/wave/siopm_wave_table.h"
#include "sequencer/simml_table.h"
#include "utils/translator_util.h"

const char *SiONVoice::CHIPTYPE_AUTO = "<autodetect>";
const char *SiONVoice::CHIPTYPE_SIOPM = "";
const char *SiONVoice::CHIPTYPE_OPL = "OPL";
const char *SiONVoice::CHIPTYPE_OPM = "OPM";
const char *SiONVoice::CHIPTYPE_OPN = "OPN";
const char *SiONVoice::CHIPTYPE_OPX = "OPX";
const char *SiONVoice::CHIPTYPE_MA3 = "MA3";
const char *SiONVoice::CHIPTYPE_PMS_GUITAR = "PMSGuitar";
const char *SiONVoice::CHIPTYPE_ANALOG_LIKE = "AnalogLike";

void SiONVoice::set_params(Vector<int> p_args) {
	TranslatorUtil::set_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_SIOPM;
}

void SiONVoice::set_params_opl(Vector<int> p_args) {
	TranslatorUtil::set_opl_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_OPL;
}

void SiONVoice::set_params_opm(Vector<int> p_args) {
	TranslatorUtil::set_opm_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_OPM;
}

void SiONVoice::set_params_opn(Vector<int> p_args) {
	TranslatorUtil::set_opn_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_OPN;
}

void SiONVoice::set_params_opx(Vector<int> p_args) {
	TranslatorUtil::set_opx_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_OPX;
}

void SiONVoice::set_params_ma3(Vector<int> p_args) {
	TranslatorUtil::set_ma3_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_MA3;

}

void SiONVoice::set_params_al(Vector<int> p_args) {
	TranslatorUtil::set_al_params(channel_params, p_args);
	chip_type = SiONVoice::CHIPTYPE_ANALOG_LIKE;

}

Vector<int> SiONVoice::get_params() const {
	return TranslatorUtil::get_params(channel_params);
}

Vector<int> SiONVoice::get_params_opl() const {
	return TranslatorUtil::get_opl_params(channel_params);
}

Vector<int> SiONVoice::get_params_opm() const {
	return TranslatorUtil::get_opm_params(channel_params);
}

Vector<int> SiONVoice::get_params_opn() const {
	return TranslatorUtil::get_opn_params(channel_params);
}

Vector<int> SiONVoice::get_params_opx() const {
	return TranslatorUtil::get_opx_params(channel_params);
}

Vector<int> SiONVoice::get_params_ma3() const {
	return TranslatorUtil::get_ma3_params(channel_params);
}

Vector<int> SiONVoice::get_params_al() const {
	return TranslatorUtil::get_al_params(channel_params);
}

String SiONVoice::get_mml(int p_index, String p_chip_type, bool p_append_postfix) const {
	String type = p_chip_type;
	if (type == SiONVoice::CHIPTYPE_AUTO) {
		type = chip_type;
	}

	String mml;
	if (type == SiONVoice::CHIPTYPE_OPL) {
		mml = "#OPL@" + itos(p_index) + TranslatorUtil::mml_opl_params(channel_params, " ", "\n", _name);
	} else if (type == SiONVoice::CHIPTYPE_OPM) {
		mml = "#OPM@" + itos(p_index) + TranslatorUtil::mml_opm_params(channel_params, " ", "\n", _name);
	} else if (type == SiONVoice::CHIPTYPE_OPN) {
		mml = "#OPN@" + itos(p_index) + TranslatorUtil::mml_opn_params(channel_params, " ", "\n", _name);
	} else if (type == SiONVoice::CHIPTYPE_OPX) {
		mml = "#OPX@" + itos(p_index) + TranslatorUtil::mml_opx_params(channel_params, " ", "\n", _name);
	} else if (type == SiONVoice::CHIPTYPE_MA3) {
		mml = "#MA@"  + itos(p_index) + TranslatorUtil::mml_ma3_params(channel_params, " ", "\n", _name);
	} else if (type == SiONVoice::CHIPTYPE_ANALOG_LIKE) {
		mml = "#AL@"  + itos(p_index) + TranslatorUtil::mml_al_params(channel_params, " ", "\n", _name);
	} else {
		mml = "#@"    + itos(p_index) + TranslatorUtil::mml_params(channel_params, " ", "\n", _name);
	}

	if (p_append_postfix) {
		String postfix = TranslatorUtil::mml_voice_setting(this);
		if (!postfix.is_empty()) {
			mml += "\n" + postfix;
		}
	}

	return mml + ";";
}

int SiONVoice::set_by_mml(String p_mml) {
	initialize();

	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "ms". Behavioral implications require investigation.
	Ref<RegEx> re_command = RegEx::create_from_string("(#[A-Z]*@)\\s*(\\d+)\\s*{(.*?)}(.*?);");
	Ref<RegExMatch> res = re_command->search(p_mml);
	if (res.is_null()) {
		return -1;
	}

	String command = res->get_string(1);
	String data = res->get_string(3);

	if (command == "#@") {
		TranslatorUtil::parse_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_SIOPM;
	} else if (command == "#OPL@") {
		TranslatorUtil::parse_opl_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_OPL;
	} else if (command == "#OPM@") {
		TranslatorUtil::parse_opm_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_OPM;
	} else if (command == "#OPN@") {
		TranslatorUtil::parse_opn_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_OPN;
	} else if (command == "#OPX@") {
		TranslatorUtil::parse_opx_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_OPX;
	} else if (command == "#MA@") {
		TranslatorUtil::parse_ma3_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_MA3;
	} else if (command == "#AL@") {
		TranslatorUtil::parse_al_params(channel_params, data);
		chip_type = SiONVoice::CHIPTYPE_ANALOG_LIKE;
	} else {
		return -1;
	}

	String postfix = res->get_string(4);
	int voice_index = res->get_string(2).to_int();
	TranslatorUtil::parse_voice_setting(this, postfix);

	Ref<RegEx> re_name = RegEx::create_from_string("^.*?(//\\s*(.+?))?[\\n\\r]");
	res = re_name->search(data);
	if (res.is_valid()) {
		_name = res->get_string(2);
	} else {
		_name = "";
	}

	return voice_index;
}

SiOPMWaveTable *SiONVoice::set_wave_table(Vector<double> p_data) {
	module_type = SiMMLTable::MT_CUSTOM;

	Vector<int> table;
	for (int i = 0; i < p_data.size(); i++) {
		int table_index = SiOPMTable::calculate_log_table_index(p_data[i]);
		table.append(table_index);
	}
	wave_data = SiOPMWaveTable::alloc(table);

	return (SiOPMWaveTable *)wave_data;
}

SiOPMWavePCMData *SiONVoice::set_pcm_voice(const Variant &p_data, int p_sampling_note, int p_src_channel_count, int p_channel_count) {
	module_type = SiMMLTable::MT_PCM;

	wave_data = memnew(SiOPMWavePCMData(p_data, p_sampling_note * 64, p_src_channel_count, p_channel_count));
	return (SiOPMWavePCMData *)wave_data;
}

SiOPMWaveSamplerData *SiONVoice::set_mp3_voice(Object *p_wave, bool p_ignore_note_off, int p_channel_count) {
	// FIXME: This method originally only supports Flash Sound objects. It needs to either be removed or adapted.

	module_type = SiMMLTable::MT_SAMPLE;

	wave_data = memnew(SiOPMWaveSamplerData(Variant(p_wave), p_ignore_note_off, 0, 2, p_channel_count));
	return (SiOPMWaveSamplerData *)wave_data;
}

SiOPMWavePCMData *SiONVoice::set_pcm_wave(int p_index, const Variant &p_data, int p_sampling_note, int p_key_range_from, int p_key_range_to, int p_src_channel_count, int p_channel_count) {
	if (module_type != SiMMLTable::MT_PCM || channel_num != p_index) {
		// FIXME: This is probably a leak.
		wave_data = nullptr;
	}

	module_type = SiMMLTable::MT_PCM;
	channel_num = p_index;

	SiOPMWavePCMTable *pcm_table = Object::cast_to<SiOPMWavePCMTable>(wave_data);
	if (!pcm_table) {
		pcm_table = memnew(SiOPMWavePCMTable);
	}
	SiOPMWavePCMData *pcm_data = memnew(SiOPMWavePCMData(p_data, int(p_sampling_note * 64), p_src_channel_count, p_channel_count));
	pcm_table->set_sample(pcm_data, p_key_range_from, p_key_range_to);
	wave_data = pcm_table;

	return pcm_data;
}

SiOPMWaveSamplerData *SiONVoice::set_sampler_wave(int p_index, const Variant &p_data, bool p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count) {
	module_type = SiMMLTable::MT_SAMPLE;

	SiOPMWaveSamplerTable *sampler_table = Object::cast_to<SiOPMWaveSamplerTable>(wave_data);
	if (!sampler_table) {
		sampler_table = memnew(SiOPMWaveSamplerTable);
	}
	SiOPMWaveSamplerData *sampler_data = memnew(SiOPMWaveSamplerData(p_data, p_ignore_note_off, p_pan, p_src_channel_count, p_channel_count));
	sampler_table->set_sample(sampler_data, p_index & (SiOPMTable::NOTE_TABLE_SIZE - 1));

	return sampler_data;
}

void SiONVoice::set_sampler_table(SiOPMWaveSamplerTable *p_table) {
	module_type = SiMMLTable::MT_SAMPLE;
	wave_data = p_table;
}

void SiONVoice::set_pms_guitar(int p_attack_rate, int p_decay_rate, int p_total_level, int p_fixed_pitch, int p_wave_shape, int p_tension) {
	module_type = SiMMLTable::MT_KS;
	channel_num = 1;

	Vector<int> param_args = { 1, 0, 0, p_wave_shape, p_attack_rate, p_decay_rate, 0, 63, 15, p_total_level, 0, 0, 1, 0, 0, 0, 0, p_fixed_pitch };
	set_params(param_args);
	pms_tension = p_tension;
	chip_type = SiONVoice::CHIPTYPE_PMS_GUITAR;
}

void SiONVoice::set_analog_like(int p_connection_type, int p_wave_shape1, int p_wave_shape2, int p_balance, int p_pitch_difference) {
	channel_params->set_operator_count(5);
	channel_params->set_algorithm((p_connection_type >= 0 && p_connection_type <= 3) ? p_connection_type : 0);
	channel_params->get_operator_params(0)->set_pulse_generator_type(p_wave_shape1);
	channel_params->get_operator_params(1)->set_pulse_generator_type(p_wave_shape2);

	int balance = CLAMP(p_balance, -64, 64);
	int (&level_table)[129] = SiOPMTable::get_instance()->eg_linear_to_total_level_table;
	channel_params->get_operator_params(0)->set_total_level(level_table[64 - balance]);
	channel_params->get_operator_params(1)->set_total_level(level_table[balance + 64]);

	channel_params->get_operator_params(0)->set_detune(0);
	channel_params->get_operator_params(1)->set_detune(p_pitch_difference);

	chip_type = SiONVoice::CHIPTYPE_ANALOG_LIKE;
}

void SiONVoice::set_envelope(int p_attack_rate, int p_decay_rate, int p_sustain_rate, int p_release_rate, int p_sustain_level, int p_total_level) {
	for (int i = 0; i < 4; i++) {
		SiOPMOperatorParams *op_params = channel_params->get_operator_params(i);
		op_params->set_attack_rate(p_attack_rate);
		op_params->set_decay_rate(p_decay_rate);
		op_params->set_sustain_rate(p_sustain_rate);
		op_params->set_release_rate(p_release_rate);
		op_params->set_sustain_level(p_sustain_level);
		op_params->set_total_level(p_total_level);
	}
}

void SiONVoice::set_filter_envelope(int p_filter_type, int p_cutoff, int p_resonance, int p_attack_rate, int p_decay_rate1, int p_decay_rate2, int p_release_rate, int p_decay_cutoff1, int p_decay_cutoff2, int p_sustain_cutoff, int p_release_cutoff) {
	channel_params->set_filter_type(p_filter_type);
	channel_params->set_filter_cutoff(p_cutoff);
	channel_params->set_filter_resonance(p_resonance);
	channel_params->set_filter_attack_rate(p_attack_rate);
	channel_params->set_filter_decay_rate1(p_decay_rate1);
	channel_params->set_filter_decay_rate2(p_decay_rate2);
	channel_params->set_filter_release_rate(p_release_rate);
	channel_params->set_filter_decay_offset1(p_decay_cutoff1);
	channel_params->set_filter_decay_offset2(p_decay_cutoff2);
	channel_params->set_filter_sustain_offset(p_sustain_cutoff);
	channel_params->set_filter_release_offset(p_release_cutoff);
}

void SiONVoice::set_amplitude_modulation(int p_depth, int p_end_depth, int p_delay, int p_term) {
	amplitude_modulation_depth = p_depth;
	channel_params->set_amplitude_modulation_depth(p_depth);

	amplitude_modulation_depth_end = p_end_depth;
	amplitude_modulation_delay = p_delay;
	amplitude_modulation_term = p_term;
}

void SiONVoice::set_pitch_modulation(int p_depth, int p_end_depth, int p_delay, int p_term) {
	pitch_modulation_depth = p_depth;
	channel_params->set_pitch_modulation_depth(p_depth);

	pitch_modulation_depth_end = p_end_depth;
	pitch_modulation_delay = p_delay;
	pitch_modulation_term = p_term;
}

SiONVoice *SiONVoice::clone() {
	SiONVoice *new_voice = memnew(SiONVoice);
	new_voice->copy_from(this);
	new_voice->_name = _name;

	return new_voice;
}

void SiONVoice::initialize() {
	SiMMLVoice::initialize();

	_name = "";
	set_update_track_parameters(true);
}

void SiONVoice::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_filter_envelope", "filter_type", "cutoff", "resonance", "attack_rate", "decay_rate1", "decay_rate2", "release_rate", "decay_cutoff1", "decay_cutoff2", "sustain_cutoff", "release_cutoff"), &SiONVoice::set_filter_envelope, DEFVAL(0), DEFVAL(128), DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(128), DEFVAL(64), DEFVAL(32), DEFVAL(128));
}

SiONVoice::SiONVoice(SiMMLTable::ModuleType p_module_type, int p_channel_num, int p_attack_rate, int p_release_rate, int p_pitch_shift, int p_connection_type, int p_wave_shape2, int p_pitch_shift2) {
	set_update_track_parameters(true);

	set_module_type(p_module_type, p_channel_num);
	channel_params->get_operator_params(0)->set_attack_rate(p_attack_rate);
	channel_params->get_operator_params(0)->set_release_rate(p_release_rate);
	pitch_shift = p_pitch_shift;

	if (p_connection_type >= 0) {
		channel_params->set_operator_count(5);
		channel_params->set_algorithm(p_connection_type <= 2 ? p_connection_type : 0);
		channel_params->get_operator_params(0)->set_pulse_generator_type(p_channel_num);
		channel_params->get_operator_params(1)->set_pulse_generator_type(p_wave_shape2);
		channel_params->get_operator_params(1)->set_detune(p_pitch_shift2);
	}
}

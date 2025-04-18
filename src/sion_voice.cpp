/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "sion_voice.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/core/memory.hpp>

#include "sion_enums.h"
#include "chip/siopm_channel_params.h"
#include "chip/siopm_operator_params.h"
#include "chip/siopm_ref_table.h"
#include "chip/wave/siopm_wave_pcm_data.h"
#include "chip/wave/siopm_wave_pcm_table.h"
#include "chip/wave/siopm_wave_sampler_data.h"
#include "chip/wave/siopm_wave_sampler_table.h"
#include "chip/wave/siopm_wave_table.h"
#include "utils/godot_util.h"
#include "utils/translator_util.h"

void SiONVoice::set_params(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_siopm_params(channel_params, data);
	chip_type = SiONChipType::CHIP_SIOPM;
}

void SiONVoice::set_params_opl(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_opl_params(channel_params, data);
	chip_type = SiONChipType::CHIP_OPL;
}

void SiONVoice::set_params_opm(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_opm_params(channel_params, data);
	chip_type = SiONChipType::CHIP_OPM;
}

void SiONVoice::set_params_opn(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_opn_params(channel_params, data);
	chip_type = SiONChipType::CHIP_OPN;
}

void SiONVoice::set_params_opx(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_opx_params(channel_params, data);
	chip_type = SiONChipType::CHIP_OPX;
}

void SiONVoice::set_params_ma3(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_ma3_params(channel_params, data);
	chip_type = SiONChipType::CHIP_MA3;

}

void SiONVoice::set_params_al(TypedArray<int> p_args) {
	Vector<int> data = make_vector_from_typed_array<int>(p_args);
	TranslatorUtil::set_al_params(channel_params, data);
	chip_type = SiONChipType::CHIP_ANALOG_LIKE;

}

TypedArray<int> SiONVoice::get_params() const {
	Vector<int> data = TranslatorUtil::get_siopm_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

TypedArray<int> SiONVoice::get_params_opl() const {
	Vector<int> data = TranslatorUtil::get_opl_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

TypedArray<int> SiONVoice::get_params_opm() const {
	Vector<int> data = TranslatorUtil::get_opm_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

TypedArray<int> SiONVoice::get_params_opn() const {
	Vector<int> data = TranslatorUtil::get_opn_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

TypedArray<int> SiONVoice::get_params_opx() const {
	Vector<int> data = TranslatorUtil::get_opx_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

TypedArray<int> SiONVoice::get_params_ma3() const {
	Vector<int> data = TranslatorUtil::get_ma3_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

TypedArray<int> SiONVoice::get_params_al() const {
	Vector<int> data = TranslatorUtil::get_ma3_params(channel_params);
	return make_typed_array_from_vector<int>(data);
}

String SiONVoice::get_mml(int p_index, SiONChipType p_chip_type, bool p_append_postfix) const {
	SiONChipType type = p_chip_type;
	if (type == SiONChipType::CHIP_AUTO) {
		type = chip_type;
	}

	String mml;
	switch (type) {
		case SiONChipType::CHIP_SIOPM:
			mml = "#@"    + itos(p_index) + TranslatorUtil::get_siopm_params_as_mml(channel_params, " ", "\n", _name);
			break;
		case SiONChipType::CHIP_OPL:
			mml = "#OPL@" + itos(p_index) + TranslatorUtil::get_opl_params_as_mml(channel_params, " ", "\n", _name);
			break;
		case SiONChipType::CHIP_OPM:
			mml = "#OPM@" + itos(p_index) + TranslatorUtil::get_opm_params_as_mml(channel_params, " ", "\n", _name);
			break;
		case SiONChipType::CHIP_OPN:
			mml = "#OPN@" + itos(p_index) + TranslatorUtil::get_opn_params_as_mml(channel_params, " ", "\n", _name);
			break;
		case SiONChipType::CHIP_OPX:
			mml = "#OPX@" + itos(p_index) + TranslatorUtil::get_opx_params_as_mml(channel_params, " ", "\n", _name);
			break;
		case SiONChipType::CHIP_MA3:
			mml = "#MA@"  + itos(p_index) + TranslatorUtil::get_ma3_params_as_mml(channel_params, " ", "\n", _name);
			break;
		case SiONChipType::CHIP_ANALOG_LIKE:
			mml = "#AL@"  + itos(p_index) + TranslatorUtil::get_al_params_as_mml(channel_params, " ", "\n", _name);
			break;
		default:
			ERR_FAIL_V_MSG("", vformat("SiONVoice: Chip type %d is unsupported for MML strings.", type));
	}

	if (p_append_postfix) {
		Ref<SiONVoice> this_voice = const_cast<SiONVoice *>(this);
		String postfix = TranslatorUtil::get_voice_setting_as_mml(this_voice);
		if (!postfix.is_empty()) {
			mml += "\n" + postfix;
		}
	}

	return mml + ";";
}

int SiONVoice::set_by_mml(String p_mml) {
	reset();

	// Godot's RegEx implementation doesn't support passing global flags, but PCRE2 allows local flags, which we can abuse.
	// (?s) enables single line mode (dot matches newline) for the entire expression.
	Ref<RegEx> re_command = RegEx::create_from_string("(?s)(#[A-Z]*@)\\s*(\\d+)\\s*{(.*?)}(.*?);");
	Ref<RegExMatch> res = re_command->search(p_mml);
	if (res.is_null()) {
		return -1;
	}

	String command = res->get_string(1);
	String data = res->get_string(3);

	if (command == "#@") {
		TranslatorUtil::parse_siopm_params(channel_params, data);
		chip_type = SiONChipType::CHIP_SIOPM;
	} else if (command == "#OPL@") {
		TranslatorUtil::parse_opl_params(channel_params, data);
		chip_type = SiONChipType::CHIP_OPL;
	} else if (command == "#OPM@") {
		TranslatorUtil::parse_opm_params(channel_params, data);
		chip_type = SiONChipType::CHIP_OPM;
	} else if (command == "#OPN@") {
		TranslatorUtil::parse_opn_params(channel_params, data);
		chip_type = SiONChipType::CHIP_OPN;
	} else if (command == "#OPX@") {
		TranslatorUtil::parse_opx_params(channel_params, data);
		chip_type = SiONChipType::CHIP_OPX;
	} else if (command == "#MA@") {
		TranslatorUtil::parse_ma3_params(channel_params, data);
		chip_type = SiONChipType::CHIP_MA3;
	} else if (command == "#AL@") {
		TranslatorUtil::parse_al_params(channel_params, data);
		chip_type = SiONChipType::CHIP_ANALOG_LIKE;
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

Ref<SiOPMWaveTable> SiONVoice::set_wave_table(Vector<double> *p_data) {
	module_type = SiONModuleType::MODULE_SCC;

	Vector<int> table;
	for (int i = 0; i < p_data->size(); i++) {
		int table_index = SiOPMRefTable::calculate_log_table_index((*p_data)[i]);
		table.append(table_index);
	}

	Ref<SiOPMWaveTable> wave_table = memnew(SiOPMWaveTable(table));
	wave_data = wave_table;

	return wave_data;
}

Ref<SiOPMWavePCMData> SiONVoice::set_pcm_voice(const Variant &p_data, int p_sampling_note, int p_src_channel_count, int p_channel_count) {
	module_type = SiONModuleType::MODULE_PCM;

	Ref<SiOPMWavePCMData> pcm_data = memnew(SiOPMWavePCMData(p_data, p_sampling_note * 64, p_src_channel_count, p_channel_count));
	wave_data = pcm_data;

	return wave_data;
}

Ref<SiOPMWavePCMData> SiONVoice::set_pcm_wave(int p_index, const Variant &p_data, int p_sampling_note, int p_key_range_from, int p_key_range_to, int p_src_channel_count, int p_channel_count) {
	if (module_type != SiONModuleType::MODULE_PCM || channel_num != p_index) {
		wave_data = Ref<SiOPMWaveBase>();
	}

	module_type = SiONModuleType::MODULE_PCM;
	channel_num = p_index;

	Ref<SiOPMWavePCMTable> pcm_table = wave_data;
	if (pcm_table.is_null()) {
		pcm_table = Ref<SiOPMWavePCMTable>(memnew(SiOPMWavePCMTable));
		wave_data = pcm_table;
	}

	Ref<SiOPMWavePCMData> pcm_data = memnew(SiOPMWavePCMData(p_data, int(p_sampling_note * 64), p_src_channel_count, p_channel_count));
	pcm_table->set_key_range_data(pcm_data, p_key_range_from, p_key_range_to);

	return pcm_data;
}

Ref<SiOPMWaveSamplerData> SiONVoice::set_sampler_voice(const Variant &p_data, bool p_ignore_note_off, int p_channel_count) {
	module_type = SiONModuleType::MODULE_SAMPLE;

	Ref<SiOPMWaveSamplerData> sampler_data = memnew(SiOPMWaveSamplerData(p_data, p_ignore_note_off, 0, 2, p_channel_count));
	wave_data = sampler_data;

	return wave_data;
}

Ref<SiOPMWaveSamplerData> SiONVoice::set_sampler_wave(int p_index, const Variant &p_data, bool p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count) {
	module_type = SiONModuleType::MODULE_SAMPLE;

	Ref<SiOPMWaveSamplerTable> sampler_table = wave_data;
	if (sampler_table.is_null()) {
		sampler_table = Ref<SiOPMWaveSamplerTable>(memnew(SiOPMWaveSamplerTable));
		wave_data = sampler_table;
	}

	Ref<SiOPMWaveSamplerData> sampler_data = memnew(SiOPMWaveSamplerData(p_data, p_ignore_note_off, p_pan, p_src_channel_count, p_channel_count));
	sampler_table->set_sample(sampler_data, p_index & (SiOPMRefTable::NOTE_TABLE_SIZE - 1));

	return sampler_data;
}

void SiONVoice::set_sampler_table(const Ref<SiOPMWaveSamplerTable> &p_table) {
	module_type = SiONModuleType::MODULE_SAMPLE;

	wave_data = p_table;
}

void SiONVoice::set_pms_guitar(int p_attack_rate, int p_decay_rate, int p_total_level, int p_fixed_pitch, int p_wave_shape, int p_tension) {
	module_type = SiONModuleType::MODULE_KS;
	channel_num = 1;

	TypedArray<int> param_args = make_typed_array_from_vector<int>({ 1, 0, 0, p_wave_shape, p_attack_rate, p_decay_rate, 0, 63, 15, p_total_level, 0, 0, 1, 0, 0, 0, 0, p_fixed_pitch });
	set_params(param_args);
	pms_tension = p_tension;
	chip_type = SiONChipType::CHIP_PMS_GUITAR;
}

void SiONVoice::set_analog_like(int p_connection_type, int p_wave_shape1, int p_wave_shape2, int p_balance, int p_pitch_difference) {
	channel_params->set_operator_count(2);
	channel_params->set_analog_like(true);
	channel_params->set_algorithm((p_connection_type >= 0 && p_connection_type <= 3) ? p_connection_type : 0);
	channel_params->get_operator_params(0)->set_pulse_generator_type(p_wave_shape1);
	channel_params->get_operator_params(1)->set_pulse_generator_type(p_wave_shape2);

	int balance = CLAMP(p_balance, -64, 64);
	int (&level_table)[129] = SiOPMRefTable::get_instance()->eg_linear_to_total_level_table;
	channel_params->get_operator_params(0)->set_total_level(level_table[64 - balance]);
	channel_params->get_operator_params(1)->set_total_level(level_table[balance + 64]);

	channel_params->get_operator_params(0)->set_detune2(0);
	channel_params->get_operator_params(1)->set_detune2(p_pitch_difference);

	chip_type = SiONChipType::CHIP_ANALOG_LIKE;
}

void SiONVoice::set_envelope(int p_attack_rate, int p_decay_rate, int p_sustain_rate, int p_release_rate, int p_sustain_level, int p_total_level) {
	for (int i = 0; i < channel_params->get_operator_count(); i++) {
		Ref<SiOPMOperatorParams> op_params = channel_params->get_operator_params(i);
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

Ref<SiONVoice> SiONVoice::clone() {
	Ref<SiONVoice> new_voice;
	new_voice.instantiate();
	new_voice->copy_from(this);
	new_voice->_name = _name;

	return new_voice;
}

void SiONVoice::reset() {
	SiMMLVoice::reset();

	_name = "";
	set_update_track_parameters(true);
}

Ref<SiONVoice> SiONVoice::create(SiONModuleType p_module_type, int p_channel_num, int p_attack_rate, int p_release_rate, int p_pitch_shift, int p_connection_type, int p_wave_shape2, int p_pitch_shift2) {
	return memnew(SiONVoice(p_module_type, p_channel_num, p_attack_rate, p_release_rate, p_pitch_shift, p_connection_type, p_wave_shape2, p_pitch_shift2));
}

void SiONVoice::_bind_methods() {
	// Factory.

	ClassDB::bind_static_method("SiONVoice", D_METHOD("create", "module_type", "channel_num", "attack_rate", "release_rate", "pitch_shift", "connection_type", "wave_shape2", "pitch_shift2"), &SiONVoice::create, DEFVAL(SiONModuleType::MODULE_GENERIC_PG), DEFVAL(0), DEFVAL(63), DEFVAL(63), DEFVAL(0), DEFVAL(-1), DEFVAL(0), DEFVAL(0));

	// Public API.

	ClassDB::bind_method(D_METHOD("get_name"), &SiONVoice::get_name);
	ClassDB::bind_method(D_METHOD("set_name", "value"), &SiONVoice::set_name);

	ClassDB::bind_method(D_METHOD("get_params"), &SiONVoice::get_params);
	ClassDB::bind_method(D_METHOD("get_params_opl"), &SiONVoice::get_params_opl);
	ClassDB::bind_method(D_METHOD("get_params_opm"), &SiONVoice::get_params_opm);
	ClassDB::bind_method(D_METHOD("get_params_opn"), &SiONVoice::get_params_opn);
	ClassDB::bind_method(D_METHOD("get_params_opx"), &SiONVoice::get_params_opx);
	ClassDB::bind_method(D_METHOD("get_params_ma3"), &SiONVoice::get_params_ma3);
	ClassDB::bind_method(D_METHOD("get_params_al"), &SiONVoice::get_params_al);

	ClassDB::bind_method(D_METHOD("set_params", "args"), &SiONVoice::set_params);
	ClassDB::bind_method(D_METHOD("set_params_opl", "args"), &SiONVoice::set_params_opl);
	ClassDB::bind_method(D_METHOD("set_params_opm", "args"), &SiONVoice::set_params_opm);
	ClassDB::bind_method(D_METHOD("set_params_opn", "args"), &SiONVoice::set_params_opn);
	ClassDB::bind_method(D_METHOD("set_params_opx", "args"), &SiONVoice::set_params_opx);
	ClassDB::bind_method(D_METHOD("set_params_ma3", "args"), &SiONVoice::set_params_ma3);
	ClassDB::bind_method(D_METHOD("set_params_al", "args"), &SiONVoice::set_params_al);

	ClassDB::bind_method(D_METHOD("get_mml", "index", "chip_type", "append_postfix"), &SiONVoice::get_mml, DEFVAL(SiONChipType::CHIP_AUTO), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_by_mml", "mml"), &SiONVoice::set_by_mml);

	ClassDB::bind_method(D_METHOD("set_pms_guitar", "attack_rate", "decay_rate", "total_level", "fixed_pitch", "wave_shape", "tension"), &SiONVoice::set_pms_guitar, DEFVAL(48), DEFVAL(48), DEFVAL(0), DEFVAL(69), DEFVAL(20), DEFVAL(8));
	ClassDB::bind_method(D_METHOD("set_analog_like", "connection_type", "wave_shape1", "wave_shape2", "balance", "pitch_difference"), &SiONVoice::set_analog_like, DEFVAL(1), DEFVAL(1), DEFVAL(0), DEFVAL(0));

	ClassDB::bind_method(D_METHOD("set_envelope", "attack_rate", "decay_rate", "sustain_rate", "release_rate", "sustain_level", "total_level"), &SiONVoice::set_envelope);
	ClassDB::bind_method(D_METHOD("set_filter_envelope", "filter_type", "cutoff", "resonance", "attack_rate", "decay_rate1", "decay_rate2", "release_rate", "decay_cutoff1", "decay_cutoff2", "sustain_cutoff", "release_cutoff"), &SiONVoice::set_filter_envelope, DEFVAL(0), DEFVAL(128), DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(128), DEFVAL(64), DEFVAL(32), DEFVAL(128));
	ClassDB::bind_method(D_METHOD("set_amplitude_modulation", "depth", "end_depth", "delay", "term"), &SiONVoice::set_amplitude_modulation, DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("set_pitch_modulation", "depth", "end_depth", "delay", "term"), &SiONVoice::set_pitch_modulation, DEFVAL(0), DEFVAL(0), DEFVAL(0), DEFVAL(0));

	ClassDB::add_property("SiONVoice", PropertyInfo(Variant::STRING, "name"), "set_name", "get_name");
}

SiONVoice::SiONVoice(SiONModuleType p_module_type, int p_channel_num, int p_attack_rate, int p_release_rate, int p_pitch_shift, int p_connection_type, int p_wave_shape2, int p_pitch_shift2) :
		SiMMLVoice() {
	set_update_track_parameters(true);

	set_module_type(p_module_type, p_channel_num);
	channel_params->get_operator_params(0)->set_attack_rate(p_attack_rate);
	channel_params->get_operator_params(0)->set_release_rate(p_release_rate);
	pitch_shift = p_pitch_shift;

	if (p_connection_type >= 0) {
		channel_params->set_operator_count(2);
		channel_params->set_analog_like(true);
		channel_params->set_algorithm(p_connection_type <= 2 ? p_connection_type : 0);
		channel_params->get_operator_params(0)->set_pulse_generator_type(p_channel_num);
		channel_params->get_operator_params(1)->set_pulse_generator_type(p_wave_shape2);
		channel_params->get_operator_params(1)->set_detune2(p_pitch_shift2);
	}
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_operator_params.h"

#include "chip/siopm_ref_table.h"
#include "chip/wave/siopm_wave_table.h"

void SiOPMOperatorParams::set_pulse_generator_type(int p_type) {
	pulse_generator_type = p_type & 511;
	pitch_table_type = SiOPMRefTable::get_instance()->get_wave_table(pulse_generator_type)->get_default_pitch_table_type();
}

int SiOPMOperatorParams::get_multiple() const {
	return (fine_multiple >> 7) & 15;
}

void SiOPMOperatorParams::set_multiple(int p_value) {
	fine_multiple = (p_value == 0 ? 64 : (p_value << 7));
}

void SiOPMOperatorParams::set_ssg_envelope_control(int p_value) {
	if (p_value >= SSGEnvelopeControl::SSG_MAX) {
		ssg_envelope_control = SSGEnvelopeControl::SSG_IGNORE;
	} else if (p_value < SSGEnvelopeControl::SSG_REPEAT_TO_ZERO) {
		ssg_envelope_control = SSGEnvelopeControl::SSG_DISABLED;
	} else {
		ssg_envelope_control = p_value;
	}
}

void SiOPMOperatorParams::initialize() {
	pulse_generator_type = SiONPulseGeneratorType::PULSE_SINE;
	pitch_table_type = SiONPitchTableType::PITCH_TABLE_OPM;

	attack_rate = 63;
	decay_rate = 0;
	sustain_rate = 0;
	release_rate = 63;
	sustain_level = 0;
	total_level = 0;

	key_scaling_rate = 1;
	key_scaling_level = 0;

	fine_multiple = 128;
	detune1 = 0;
	detune2 = 0;

	amplitude_modulation_shift = 0;
	initial_phase = 0;
	fixed_pitch = 0;

	mute = false;
	ssg_envelope_control = SSG_DISABLED;
	frequency_modulation_level = 5;
	envelope_reset_on_attack = false;
}

void SiOPMOperatorParams::copy_from(const Ref<SiOPMOperatorParams> &p_params) {
	pulse_generator_type = p_params->pulse_generator_type;
	pitch_table_type     = p_params->pitch_table_type;

	attack_rate   = p_params->attack_rate;
	decay_rate    = p_params->decay_rate;
	sustain_rate  = p_params->sustain_rate;
	release_rate  = p_params->release_rate;
	sustain_level = p_params->sustain_level;
	total_level   = p_params->total_level;

	key_scaling_rate  = p_params->key_scaling_rate;
	key_scaling_level = p_params->key_scaling_level;

	fine_multiple = p_params->fine_multiple;
	detune1       = p_params->detune1;
	detune2       = p_params->detune2;

	amplitude_modulation_shift = p_params->amplitude_modulation_shift;
	initial_phase              = p_params->initial_phase;
	fixed_pitch                = p_params->fixed_pitch;

	mute                       = p_params->mute;
	ssg_envelope_control       = p_params->ssg_envelope_control;
	frequency_modulation_level = p_params->frequency_modulation_level;
	envelope_reset_on_attack   = p_params->envelope_reset_on_attack;
}

String SiOPMOperatorParams::_to_string() const {
	String params = "";

	params += "pg=" + itos(pulse_generator_type) + ", ";
	params += "pt=" + itos(pitch_table_type) + ", ";

	params += "ar=" + itos(attack_rate) + ", ";
	params += "dr=" + itos(decay_rate) + ", ";
	params += "sr=" + itos(sustain_rate) + ", ";
	params += "rr=" + itos(release_rate) + ", ";
	params += "sl=" + itos(sustain_level) + ", ";
	params += "tl=" + itos(total_level) + ", ";

	params += "keyscale=(" + itos(key_scaling_rate) + ", " + itos(key_scaling_level) + "), ";
	params += "fmul=" + itos(fine_multiple) + ", ";
	params += "detune=(" + itos(detune1) + ", " + itos(detune2) + "), ";

	params += "amp=" + itos(amplitude_modulation_shift) + ", ";
	params += "phase=" + itos(initial_phase) + ", ";
	params += "note=" + itos(fixed_pitch) + ", ";

	params += "ssgec=" + itos(ssg_envelope_control) + ", ";
	params += "mute=" + String(mute ? "yes" : "no") + ", ";
	params += "reset=" + String(envelope_reset_on_attack ? "yes" : "no");

	return "SiOPMOperatorParams: " + params;
}

void SiOPMOperatorParams::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_pulse_generator_type"), &SiOPMOperatorParams::get_pulse_generator_type);
	ClassDB::bind_method(D_METHOD("set_pulse_generator_type", "type"), &SiOPMOperatorParams::set_pulse_generator_type);
	ClassDB::bind_method(D_METHOD("get_pitch_table_type"), &SiOPMOperatorParams::get_pitch_table_type);
	ClassDB::bind_method(D_METHOD("set_pitch_table_type", "type"), &SiOPMOperatorParams::set_pitch_table_type);

	ClassDB::bind_method(D_METHOD("get_attack_rate"), &SiOPMOperatorParams::get_attack_rate);
	ClassDB::bind_method(D_METHOD("set_attack_rate", "value"), &SiOPMOperatorParams::set_attack_rate);
	ClassDB::bind_method(D_METHOD("get_decay_rate"), &SiOPMOperatorParams::get_decay_rate);
	ClassDB::bind_method(D_METHOD("set_decay_rate", "value"), &SiOPMOperatorParams::set_decay_rate);
	ClassDB::bind_method(D_METHOD("get_sustain_rate"), &SiOPMOperatorParams::get_sustain_rate);
	ClassDB::bind_method(D_METHOD("set_sustain_rate", "value"), &SiOPMOperatorParams::set_sustain_rate);
	ClassDB::bind_method(D_METHOD("get_release_rate"), &SiOPMOperatorParams::get_release_rate);
	ClassDB::bind_method(D_METHOD("set_release_rate", "value"), &SiOPMOperatorParams::set_release_rate);
	ClassDB::bind_method(D_METHOD("get_sustain_level"), &SiOPMOperatorParams::get_sustain_level);
	ClassDB::bind_method(D_METHOD("set_sustain_level", "value"), &SiOPMOperatorParams::set_sustain_level);
	ClassDB::bind_method(D_METHOD("get_total_level"), &SiOPMOperatorParams::get_total_level);
	ClassDB::bind_method(D_METHOD("set_total_level", "value"), &SiOPMOperatorParams::set_total_level);

	ClassDB::bind_method(D_METHOD("get_key_scaling_rate"), &SiOPMOperatorParams::get_key_scaling_rate);
	ClassDB::bind_method(D_METHOD("set_key_scaling_rate", "value"), &SiOPMOperatorParams::set_key_scaling_rate);
	ClassDB::bind_method(D_METHOD("get_key_scaling_level"), &SiOPMOperatorParams::get_key_scaling_level);
	ClassDB::bind_method(D_METHOD("set_key_scaling_level", "value"), &SiOPMOperatorParams::set_key_scaling_level);

	ClassDB::bind_method(D_METHOD("get_fine_multiple"), &SiOPMOperatorParams::get_fine_multiple);
	ClassDB::bind_method(D_METHOD("set_fine_multiple", "value"), &SiOPMOperatorParams::set_fine_multiple);
	ClassDB::bind_method(D_METHOD("get_multiple"), &SiOPMOperatorParams::get_multiple);
	ClassDB::bind_method(D_METHOD("set_multiple", "value"), &SiOPMOperatorParams::set_multiple);
	ClassDB::bind_method(D_METHOD("get_detune1"), &SiOPMOperatorParams::get_detune1);
	ClassDB::bind_method(D_METHOD("set_detune1", "value"), &SiOPMOperatorParams::set_detune1);
	ClassDB::bind_method(D_METHOD("get_detune2"), &SiOPMOperatorParams::get_detune2);
	ClassDB::bind_method(D_METHOD("set_detune2", "value"), &SiOPMOperatorParams::set_detune2);

	ClassDB::bind_method(D_METHOD("get_amplitude_modulation_shift"), &SiOPMOperatorParams::get_amplitude_modulation_shift);
	ClassDB::bind_method(D_METHOD("set_amplitude_modulation_shift", "value"), &SiOPMOperatorParams::set_amplitude_modulation_shift);
	ClassDB::bind_method(D_METHOD("get_initial_phase"), &SiOPMOperatorParams::get_initial_phase);
	ClassDB::bind_method(D_METHOD("set_initial_phase", "value"), &SiOPMOperatorParams::set_initial_phase);
	ClassDB::bind_method(D_METHOD("get_fixed_pitch"), &SiOPMOperatorParams::get_fixed_pitch);
	ClassDB::bind_method(D_METHOD("set_fixed_pitch", "value"), &SiOPMOperatorParams::set_fixed_pitch);

	ClassDB::bind_method(D_METHOD("is_mute"), &SiOPMOperatorParams::is_mute);
	ClassDB::bind_method(D_METHOD("set_mute", "mute"), &SiOPMOperatorParams::set_mute);
	ClassDB::bind_method(D_METHOD("get_ssg_envelope_control"), &SiOPMOperatorParams::get_ssg_envelope_control);
	ClassDB::bind_method(D_METHOD("set_ssg_envelope_control", "value"), &SiOPMOperatorParams::set_ssg_envelope_control);
	ClassDB::bind_method(D_METHOD("get_frequency_modulation_level"), &SiOPMOperatorParams::get_frequency_modulation_level);
	ClassDB::bind_method(D_METHOD("set_frequency_modulation_level", "value"), &SiOPMOperatorParams::set_frequency_modulation_level);
	ClassDB::bind_method(D_METHOD("is_envelope_reset_on_attack"), &SiOPMOperatorParams::is_envelope_reset_on_attack);
	ClassDB::bind_method(D_METHOD("set_envelope_reset_on_attack", "reset"), &SiOPMOperatorParams::set_envelope_reset_on_attack);

	//

	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "pulse_generator_type"), "set_pulse_generator_type", "get_pulse_generator_type");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "pitch_table_type"), "set_pitch_table_type", "get_pitch_table_type");

	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "attack_rate"), "set_attack_rate", "get_attack_rate");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "decay_rate"), "set_decay_rate", "get_decay_rate");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "sustain_rate"), "set_sustain_rate", "get_sustain_rate");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "release_rate"), "set_release_rate", "get_release_rate");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "sustain_level"), "set_sustain_level", "get_sustain_level");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "total_level"), "set_total_level", "get_total_level");

	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "key_scaling_rate"), "set_key_scaling_rate", "get_key_scaling_rate");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "key_scaling_level"), "set_key_scaling_level", "get_key_scaling_level");

	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "fine_multiple"), "set_fine_multiple", "get_fine_multiple");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "multiple"), "set_multiple", "get_multiple");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "detune1"), "set_detune1", "get_detune1");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "detune2"), "set_detune2", "get_detune2");

	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "amplitude_modulation_shift"), "set_amplitude_modulation_shift", "get_amplitude_modulation_shift");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "initial_phase"), "set_initial_phase", "get_initial_phase");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "fixed_pitch"), "set_fixed_pitch", "get_fixed_pitch");

	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::BOOL, "mute"), "set_mute", "is_mute");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "ssg_envelope_control"), "set_ssg_envelope_control", "get_ssg_envelope_control");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::INT, "frequency_modulation_level"), "set_frequency_modulation_level", "get_frequency_modulation_level");
	ClassDB::add_property("SiOPMOperatorParams", PropertyInfo(Variant::BOOL, "envelope_reset_on_attack"), "set_envelope_reset_on_attack", "is_envelope_reset_on_attack");

	BIND_ENUM_CONSTANT(SSG_DISABLED);
	BIND_ENUM_CONSTANT(SSG_REPEAT_TO_ZERO);
	BIND_ENUM_CONSTANT(SSG_IGNORE);
	BIND_ENUM_CONSTANT(SSG_REPEAT_SHUTTLE);
	BIND_ENUM_CONSTANT(SSG_ONCE_HOLD_HIGH);
	BIND_ENUM_CONSTANT(SSG_REPEAT_TO_MAX);
	BIND_ENUM_CONSTANT(SSG_INVERSE);
	BIND_ENUM_CONSTANT(SSG_REPEAT_SHUTTLE_INVERSE);
	BIND_ENUM_CONSTANT(SSG_ONCE_HOLD_LOW);
	BIND_ENUM_CONSTANT(SSG_CONSTANT_HIGH);
	BIND_ENUM_CONSTANT(SSG_CONSTANT_LOW);
	BIND_ENUM_CONSTANT(SSG_MAX);
}

SiOPMOperatorParams::SiOPMOperatorParams() {
	initialize();
}

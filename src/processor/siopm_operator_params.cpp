/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_operator_params.h"

#include "processor/siopm_ref_table.h"
#include "processor/wave/siopm_wave_table.h"

void SiOPMOperatorParams::set_pulse_generator_type(int p_type) {
	pulse_generator_type = p_type & 511;
	pitch_table_type = SiOPMRefTable::get_instance()->get_wave_table(pulse_generator_type)->get_default_pitch_table_type();
}

int SiOPMOperatorParams::get_multiple() const {
	return (fine_multiple >> 7) & 15;
}

void SiOPMOperatorParams::set_multiple(int p_value) {
	fine_multiple = p_value ? (p_value << 7) : 64;
}

String SiOPMOperatorParams::to_string() const {
	// Class was renamed to say "params", but this may be needed for compatibility,
	// so keeping the old name here.
	String str = "SiOPMOperatorParam : ";

	str += itos(pulse_generator_type) + "(";
	str += itos(pitch_table_type) + ") : ";
	str += itos(attack_rate) + "/";
	str += itos(decay_rate) + "/";
	str += itos(sustain_rate) + "/";
	str += itos(release_rate) + "/";
	str += itos(sustain_level) + "/";
	str += itos(total_level) + " : ";
	str += itos(key_scaling_rate) + "/";
	str += itos(key_scaling_level) + " : ";
	str += itos(fine_multiple) + "/";
	str += itos(detune1) + "/";
	str += itos(detune) + " : ";
	str += itos(amplitude_modulation_shift) + "/";
	str += itos(initial_phase)  + "/";
	str += itos(fixed_pitch) + " : ";
	str += itos(ssg_type_envelope_control) + "/";

	// FIXME: These are boolean flags, which can technically be interpreted as ints and converted.
	// But original code was probably generating some strings of "true" and "false". Needs testing.
	str += itos(mute) + "/";
	str += itos(envelope_reset_on_attack);

	return str;
}

void SiOPMOperatorParams::initialize() {
	pulse_generator_type = SiOPMRefTable::PG_SINE;
	pitch_table_type = SiOPMRefTable::PT_OPM;

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
	detune = 0;

	amplitude_modulation_shift = 0;
	initial_phase = 0;
	fixed_pitch = 0;

	mute = false;
	ssg_type_envelope_control = 0;
	frequency_modulation_level = 5;
	envelope_reset_on_attack = false;
}

void SiOPMOperatorParams::copy_from(SiOPMOperatorParams *p_params) {
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
	detune        = p_params->detune;

	amplitude_modulation_shift = p_params->amplitude_modulation_shift;
	initial_phase              = p_params->initial_phase;
	fixed_pitch                = p_params->fixed_pitch;

	mute                       = p_params->mute;
	ssg_type_envelope_control  = p_params->ssg_type_envelope_control;
	frequency_modulation_level = p_params->frequency_modulation_level;
	envelope_reset_on_attack   = p_params->envelope_reset_on_attack;
}

SiOPMOperatorParams::SiOPMOperatorParams() {
	initialize();
}

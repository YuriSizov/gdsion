/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_voice.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>

#include "processor/channels/siopm_channel_base.h"
#include "processor/siopm_channel_params.h"
#include "processor/siopm_operator_params.h"
#include "processor/wave/siopm_wave_pcm_data.h"
#include "processor/wave/siopm_wave_pcm_table.h"
#include "processor/wave/siopm_wave_sampler_table.h"
#include "processor/wave/siopm_wave_table.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_table.h"
#include "sequencer/simml_track.h"

using namespace godot;

bool SiMMLVoice::is_fm_voice() const {
	return module_type == SiMMLTable::MT_FM;
}

bool SiMMLVoice::is_pcm_voice() const {
	return (Object::cast_to<SiOPMWavePCMTable>(wave_data) || Object::cast_to<SiOPMWavePCMData>(wave_data));
}

bool SiMMLVoice::is_sampler_voice() const {
	return Object::cast_to<SiOPMWaveSamplerTable>(wave_data);
}

bool SiMMLVoice::is_wave_table_voice() const {
	return Object::cast_to<SiOPMWaveTable>(wave_data);
}

bool SiMMLVoice::is_suitable_for_fm_voice() const {
	return update_track_parameters || (SiMMLTable::get_instance()->is_suitable_for_fm_voice(module_type) && wave_data == nullptr);
}

void SiMMLVoice::set_module_type(SiMMLTable::ModuleType p_module_type, int p_channel_num, int p_tone_num) {
	module_type = p_module_type;
	channel_num = p_channel_num;
	tone_num = p_tone_num;

	int pg_type = -1;
	// TODO: This is needed because this method can be indirectly called by Godot editor's docgen. But this smells...
	if (SiMMLTable::get_instance()) {
		pg_type = SiMMLTable::get_instance()->get_pulse_generator_type(module_type, channel_num, tone_num);
	}
	if (pg_type != -1) {
		channel_params->get_operator_params(0)->set_pulse_generator_type(pg_type);
	}
}

void SiMMLVoice::update_track_voice(SiMMLTrack *p_track) {
	switch (module_type) {
		case SiMMLTable::MT_FM: { // Registered FM voice (%6)
			p_track->set_channel_module_type(SiMMLTable::MT_FM, channel_num);
		} break;

		case SiMMLTable::MT_KS: { // PMS Guitar (%11)
			p_track->set_channel_module_type(SiMMLTable::MT_KS, 1);
			p_track->get_channel()->set_channel_params(channel_params, false);
			p_track->get_channel()->set_all_release_rate(pms_tension);
			if (is_pcm_voice()) {
				p_track->get_channel()->set_wave_data(wave_data);
			}
		} break;

		default: { // Other sound modules.
			if (wave_data) {
				p_track->set_channel_module_type(wave_data->get_module_type(), -1);
				p_track->get_channel()->set_channel_params(channel_params, update_volumes);
				p_track->get_channel()->set_wave_data(wave_data);
			} else {
				p_track->set_channel_module_type(module_type, channel_num, tone_num);
				p_track->get_channel()->set_channel_params(channel_params, update_volumes);
			}
		} break;
	}

	if (!Math::is_nan(default_gate_time)) {
		p_track->set_quantize_ratio(default_gate_time);
	}

	p_track->set_pitch_shift(pitch_shift);
	p_track->set_note_shift(note_shift);
	p_track->set_velocity_shift(velocity_shift);

	p_track->set_velocity_mode(velocity_mode);
	p_track->set_expression_mode(expression_mode);
	if (update_volumes) {
		p_track->set_velocity(velocity);
		p_track->set_expression(expression);
	}

	p_track->set_portament(portament);
	p_track->set_release_sweep(release_sweep);
	p_track->set_modulation_envelope(false, amplitude_modulation_depth, amplitude_modulation_depth_end, amplitude_modulation_delay, amplitude_modulation_term);
	p_track->set_modulation_envelope(true,  pitch_modulation_depth, pitch_modulation_depth_end, pitch_modulation_delay, pitch_modulation_term);
	{
		p_track->set_tone_envelope(1, note_on_tone_envelope, note_on_tone_envelope_step);
		p_track->set_amplitude_envelope(1, note_on_amplitude_envelope, note_on_amplitude_envelope_step);
		p_track->set_filter_envelope(1, note_on_filter_envelope, note_on_filter_envelope_step);
		p_track->set_pitch_envelope(1, note_on_pitch_envelope, note_on_pitch_envelope_step);
		p_track->set_note_envelope(1, note_on_note_envelope, note_on_note_envelope_step);
		p_track->set_tone_envelope(0, note_off_tone_envelope, note_off_tone_envelope_step);
		p_track->set_amplitude_envelope(0, note_off_amplitude_envelope, note_off_amplitude_envelope_step);
		p_track->set_filter_envelope(0, note_off_filter_envelope, note_off_filter_envelope_step);
		p_track->set_pitch_envelope(0, note_off_pitch_envelope, note_off_pitch_envelope_step);
		p_track->set_note_envelope(0, note_off_note_envelope, note_off_note_envelope_step);
	}
}

SiMMLVoice *SiMMLVoice::create_blank_pcm_voice(int p_channel_num) {
	SiMMLVoice *instance = memnew(SiMMLVoice);
	instance->module_type = SiMMLTable::MT_PCM;
	instance->channel_num = p_channel_num;

	SiOPMWavePCMTable *pcm_table = memnew(SiOPMWavePCMTable);
	instance->wave_data = pcm_table;

	return instance;
}

void SiMMLVoice::initialize() {
	chip_type = "";

	update_track_parameters = false;
	update_volumes = false;

	module_type = SiMMLTable::MT_ALL;
	channel_num = -1;
	tone_num = -1;
	preferable_note = -1;

	channel_params->initialize();
	// FIXME: Memory leak?
	wave_data = nullptr;
	pms_tension = 8;

	default_gate_time = NAN;
	default_gate_ticks = -1;
	default_key_on_delay_ticks = -1;
	pitch_shift = 0;
	note_shift = 0;
	portament = 0;
	release_sweep = 0;

	velocity = 256;
	expression = 128;
	velocity_mode = 0;
	velocity_shift = 4;
	expression_mode = 0;

	amplitude_modulation_depth = 0;
	amplitude_modulation_depth_end = 0;
	amplitude_modulation_delay = 0;
	amplitude_modulation_term = 0;
	pitch_modulation_depth = 0;
	pitch_modulation_depth_end = 0;
	pitch_modulation_delay = 0;
	pitch_modulation_term = 0;

	// FIXME: Memory leak?
	note_on_tone_envelope = nullptr;
	note_on_amplitude_envelope = nullptr;
	note_on_filter_envelope = nullptr;
	note_on_pitch_envelope = nullptr;
	note_on_note_envelope = nullptr;
	note_off_tone_envelope = nullptr;
	note_off_amplitude_envelope = nullptr;
	note_off_filter_envelope = nullptr;
	note_off_pitch_envelope = nullptr;
	note_off_note_envelope = nullptr;

	note_on_tone_envelope_step = 1;
	note_on_amplitude_envelope_step = 1;
	note_on_filter_envelope_step = 1;
	note_on_pitch_envelope_step = 1;
	note_on_note_envelope_step = 1;
	note_off_tone_envelope_step = 1;
	note_off_amplitude_envelope_step = 1;
	note_off_filter_envelope_step = 1;
	note_off_pitch_envelope_step = 1;
	note_off_note_envelope_step = 1;
}

void SiMMLVoice::copy_from(SiMMLVoice *p_source) {
	chip_type = p_source->chip_type;

	update_track_parameters = p_source->update_track_parameters;
	update_volumes = p_source->update_volumes;

	module_type = p_source->module_type;
	channel_num = p_source->channel_num;
	tone_num = p_source->tone_num;
	preferable_note = p_source->preferable_note;

	channel_params->copy_from(p_source->channel_params);
	wave_data = p_source->wave_data;
	pms_tension = p_source->pms_tension;

	default_gate_time = p_source->default_gate_time;
	default_gate_ticks = p_source->default_gate_ticks;
	default_key_on_delay_ticks = p_source->default_key_on_delay_ticks;
	pitch_shift = p_source->pitch_shift;
	note_shift = p_source->note_shift;
	portament = p_source->portament;
	release_sweep = p_source->release_sweep;

	velocity = p_source->velocity;
	expression = p_source->expression;
	velocity_mode = p_source->velocity_mode;
	velocity_shift = p_source->velocity_shift;
	expression_mode = p_source->expression_mode;

	amplitude_modulation_depth = p_source->amplitude_modulation_depth;
	amplitude_modulation_depth_end = p_source->amplitude_modulation_depth_end;
	amplitude_modulation_delay = p_source->amplitude_modulation_delay;
	amplitude_modulation_term = p_source->amplitude_modulation_term;
	pitch_modulation_depth = p_source->pitch_modulation_depth;
	pitch_modulation_depth_end = p_source->pitch_modulation_depth_end;
	pitch_modulation_delay = p_source->pitch_modulation_delay;
	pitch_modulation_term = p_source->pitch_modulation_term;

	// FIXME: Memory leak?
#define COPY_NOTE_ENVELOPE(m_prop)            \
	m_prop = nullptr;                         \
	if (p_source->m_prop) {                   \
		m_prop = memnew(SiMMLEnvelopeTable);  \
		m_prop->copy_from(p_source->m_prop);  \
	}

	COPY_NOTE_ENVELOPE(note_on_tone_envelope);
	COPY_NOTE_ENVELOPE(note_on_amplitude_envelope);
	COPY_NOTE_ENVELOPE(note_on_filter_envelope);
	COPY_NOTE_ENVELOPE(note_on_pitch_envelope);
	COPY_NOTE_ENVELOPE(note_on_note_envelope);
	COPY_NOTE_ENVELOPE(note_off_tone_envelope);
	COPY_NOTE_ENVELOPE(note_off_amplitude_envelope);
	COPY_NOTE_ENVELOPE(note_off_filter_envelope);
	COPY_NOTE_ENVELOPE(note_off_pitch_envelope);
	COPY_NOTE_ENVELOPE(note_off_note_envelope);
#undef COPY_NOTE_ENVELOPE

	note_on_tone_envelope_step = p_source->note_on_tone_envelope_step;
	note_on_amplitude_envelope_step = p_source->note_on_amplitude_envelope_step;
	note_on_filter_envelope_step = p_source->note_on_filter_envelope_step;
	note_on_pitch_envelope_step = p_source->note_on_pitch_envelope_step;
	note_on_note_envelope_step = p_source->note_on_note_envelope_step;
	note_off_tone_envelope_step = p_source->note_off_tone_envelope_step;
	note_off_amplitude_envelope_step = p_source->note_off_amplitude_envelope_step;
	note_off_filter_envelope_step = p_source->note_off_filter_envelope_step;
	note_off_pitch_envelope_step = p_source->note_off_pitch_envelope_step;
	note_off_note_envelope_step = p_source->note_off_note_envelope_step;
}

void SiMMLVoice::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_channel_params"), &SiMMLVoice::get_channel_params);

	ClassDB::bind_method(D_METHOD("get_velocity"), &SiMMLVoice::get_velocity);
	ClassDB::bind_method(D_METHOD("set_velocity", "value"), &SiMMLVoice::set_velocity);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::INT, "velocity"), "set_velocity", "get_velocity");
	ClassDB::bind_method(D_METHOD("get_expression"), &SiMMLVoice::get_expression);
	ClassDB::bind_method(D_METHOD("set_expression", "value"), &SiMMLVoice::set_expression);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::INT, "expression"), "set_expression", "get_expression");
	ClassDB::bind_method(D_METHOD("get_velocity_mode"), &SiMMLVoice::get_velocity_mode);
	ClassDB::bind_method(D_METHOD("set_velocity_mode", "value"), &SiMMLVoice::set_velocity_mode);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::INT, "velocity_mode"), "set_velocity_mode", "get_velocity_mode");
	ClassDB::bind_method(D_METHOD("get_velocity_shift"), &SiMMLVoice::get_velocity_shift);
	ClassDB::bind_method(D_METHOD("set_velocity_shift", "value"), &SiMMLVoice::set_velocity_shift);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::INT, "velocity_shift"), "set_velocity_shift", "get_velocity_shift");
	ClassDB::bind_method(D_METHOD("get_expression_mode"), &SiMMLVoice::get_expression_mode);
	ClassDB::bind_method(D_METHOD("set_expression_mode", "value"), &SiMMLVoice::set_expression_mode);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::INT, "expression_mode"), "set_expression_mode", "get_expression_mode");

	ClassDB::bind_method(D_METHOD("should_update_track_parameters"), &SiMMLVoice::should_update_track_parameters);
	ClassDB::bind_method(D_METHOD("set_update_track_parameters", "enabled"), &SiMMLVoice::set_update_track_parameters);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::BOOL, "update_track_parameters"), "set_update_track_parameters", "should_update_track_parameters");

	ClassDB::bind_method(D_METHOD("should_update_volumes"), &SiMMLVoice::should_update_volumes);
	ClassDB::bind_method(D_METHOD("set_update_volumes", "enabled"), &SiMMLVoice::set_update_volumes);
	ClassDB::add_property("SiMMLVoice", PropertyInfo(Variant::BOOL, "update_volumes"), "set_update_volumes", "should_update_volumes");
}

SiMMLVoice::SiMMLVoice() {
	channel_params = memnew(SiOPMChannelParams);
	initialize();
}

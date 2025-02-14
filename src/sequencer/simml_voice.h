/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_VOICE_H
#define SIMML_VOICE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include "chip/siopm_channel_params.h"
#include "chip/wave/siopm_wave_base.h"
#include "sequencer/simml_envelope_table.h"

using namespace godot;

class SiMMLTrack;
enum SiONChipType : signed int;
enum SiONModuleType : unsigned int;

// Base voice data class.
class SiMMLVoice : public RefCounted {
	GDCLASS(SiMMLVoice, RefCounted)

	friend class TranslatorUtil;

	// Set to true to update track params alongside channel params.
	bool update_track_parameters = false;
	// Set to true to update volume, velocity, expression, and panning when the voice is set.
	bool update_volumes = false;

	int tone_num = -1;
	int preferable_note = -1;

	// Setting to NAN disables the feature, tracks can still have their quantization ratios adjusted.
	double default_gate_time = NAN;
	int default_gate_ticks = -1;
	int default_key_on_delay_ticks = -1;
	int note_shift = 0;
	int portament = 0;
	int release_sweep = 0;

	int velocity = 256;
	int expression = 128;
	int velocity_mode = 0;
	int velocity_shift = 4;
	int expression_mode = 0;

	Ref<SiMMLEnvelopeTable> note_on_tone_envelope;
	Ref<SiMMLEnvelopeTable> note_on_amplitude_envelope;
	Ref<SiMMLEnvelopeTable> note_on_filter_envelope;
	Ref<SiMMLEnvelopeTable> note_on_pitch_envelope;
	Ref<SiMMLEnvelopeTable> note_on_note_envelope;
	Ref<SiMMLEnvelopeTable> note_off_tone_envelope;
	Ref<SiMMLEnvelopeTable> note_off_amplitude_envelope;
	Ref<SiMMLEnvelopeTable> note_off_filter_envelope;
	Ref<SiMMLEnvelopeTable> note_off_pitch_envelope;
	Ref<SiMMLEnvelopeTable> note_off_note_envelope;

	int note_on_tone_envelope_step = 1;
	int note_on_amplitude_envelope_step = 1;
	int note_on_filter_envelope_step = 1;
	int note_on_pitch_envelope_step = 1;
	int note_on_note_envelope_step = 1;
	int note_off_tone_envelope_step = 1;
	int note_off_amplitude_envelope_step = 1;
	int note_off_filter_envelope_step = 1;
	int note_off_pitch_envelope_step = 1;
	int note_off_note_envelope_step = 1;

protected:
	SiONChipType chip_type = (SiONChipType)0;
	SiONModuleType module_type = (SiONModuleType)5;
	int channel_num = -1;
	// PMS guitar tension.
	int pms_tension = 8;

	// Params for the FM sound channel.
	Ref<SiOPMChannelParams> channel_params;
	Ref<SiOPMWaveBase> wave_data;

	int pitch_shift = 0;

	int amplitude_modulation_depth = 0;
	int amplitude_modulation_depth_end = 0;
	int amplitude_modulation_delay = 0;
	int amplitude_modulation_term = 0;
	int pitch_modulation_depth = 0;
	int pitch_modulation_depth_end = 0;
	int pitch_modulation_delay = 0;
	int pitch_modulation_term = 0;

	static void _bind_methods();

public:
	static Ref<SiMMLVoice> create_blank_pcm_voice(int p_channel_num);

	SiONChipType get_chip_type() const { return chip_type; }
	void set_chip_type(SiONChipType p_type) { chip_type = p_type; }
	SiONModuleType get_module_type() const { return module_type; }
	void set_module_type(SiONModuleType p_module_type, int p_channel_num = 0, int p_tone_num = -1);
	int get_channel_num() const { return channel_num; }
	void set_channel_num(int p_num) { channel_num = p_num; }
	int get_tone_num() const { return tone_num; }
	void set_tone_num(int p_num) { tone_num = p_num; }

	Ref<SiOPMChannelParams> get_channel_params() const { return channel_params; }
	Ref<SiOPMWaveBase> get_wave_data() const { return wave_data; }
	void set_wave_data(const Ref<SiOPMWaveBase> &p_data) { wave_data = p_data; }

	bool is_fm_voice() const;
	bool is_pcm_voice() const;
	bool is_sampler_voice() const;
	bool is_wave_table_voice() const;
	bool is_suitable_for_fm_voice() const;

	bool should_update_track_parameters() const { return update_track_parameters; }
	void set_update_track_parameters(bool p_enable) { update_track_parameters = p_enable; }
	bool should_update_volumes() const { return update_volumes; }
	void set_update_volumes(bool p_enable) { update_volumes = p_enable; }

	double get_default_gate_time() const { return default_gate_time; }
	void set_default_gate_time(double p_gate_time) { default_gate_time = p_gate_time; }
	int get_release_sweep() const { return release_sweep; }
	void set_release_sweep(int p_sweep) { release_sweep = p_sweep; }

	int get_velocity() const { return velocity; }
	void set_velocity(int p_value) { velocity = p_value; }
	int get_expression() const { return expression; }
	void set_expression(int p_value) { expression = p_value; }
	int get_velocity_mode() const { return velocity_mode; }
	void set_velocity_mode(int p_value) { velocity_mode = p_value; }
	int get_velocity_shift() const { return velocity_shift; }
	void set_velocity_shift(int p_value) { velocity_shift = p_value; }
	int get_expression_mode() const { return expression_mode; }
	void set_expression_mode(int p_value) { expression_mode = p_value; }

	bool has_amplitude_modulation() const;
	bool has_pitch_modulation() const;

	void update_track_voice(SiMMLTrack *p_track);

	virtual void reset();
	virtual void copy_from(const Ref<SiMMLVoice> &p_source);

	SiMMLVoice();
	~SiMMLVoice() {}
};

#endif // SIMML_VOICE_H

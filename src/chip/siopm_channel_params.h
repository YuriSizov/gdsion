/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_PARAMS_H
#define SIOPM_CHANNEL_PARAMS_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class MMLSequence;
class SiOPMOperatorParams;

// Channel parameters for SiONVoice.
class SiOPMChannelParams : public RefCounted {
	GDCLASS(SiOPMChannelParams, RefCounted)

	friend class TranslatorUtil;

public:
	static const int MAX_OPERATORS = 4;

private:
	MMLSequence *init_sequence = nullptr;

	// This list is exactly MAX_OPERATORS at all times, use operator_count to read only valid values.
	List<SiOPMOperatorParams *> operator_params;
	int operator_count = 0;
	bool analog_like = false;

	// Algorithm [0,15]
	int algorithm = 0;
	// Feedback [0,7]
	int feedback = 0;
	// Feedback connection [0,3]
	int feedback_connection = 0;
	int envelope_frequency_ratio = 100;
	int lfo_wave_shape = 0;
	int lfo_frequency_step = 0;

	int amplitude_modulation_depth = 0;
	int pitch_modulation_depth = 0;
	Vector<double> master_volumes;
	int pan = 0;

	int filter_type = 0;
	int filter_cutoff = 0;
	int filter_resonance = 0;
	int filter_attack_rate = 0;
	int filter_decay_rate1 = 0;
	int filter_decay_rate2 = 0;
	int filter_release_rate = 0;
	int filter_decay_offset1 = 0;
	int filter_decay_offset2 = 0;
	int filter_sustain_offset = 0;
	int filter_release_offset = 0;

protected:
	static void _bind_methods();

public:
	MMLSequence *get_init_sequence() const { return init_sequence; }

	SiOPMOperatorParams *get_operator_params(int p_index);
	int get_operator_count() const { return operator_count; }
	void set_operator_count(int p_value);
	bool is_analog_like() const { return analog_like; }
	void set_analog_like(bool p_value) { analog_like = p_value; }

	int get_algorithm() const { return algorithm; }
	void set_algorithm(int p_value) { algorithm = p_value; }
	int get_feedback() const { return feedback; }
	void set_feedback(int p_value) { feedback = p_value; }
	int get_feedback_connection() const { return feedback_connection; }
	void set_feedback_connection(int p_value) { feedback_connection = p_value; }
	int get_envelope_frequency_ratio() const { return envelope_frequency_ratio; }
	void set_envelope_frequency_ratio(int p_value) { envelope_frequency_ratio = p_value; }

	int get_lfo_wave_shape() const { return lfo_wave_shape; }
	void set_lfo_wave_shape(int p_value) { lfo_wave_shape = p_value; }
	int get_lfo_frequency_step() const { return lfo_frequency_step; }
	void set_lfo_frequency_step(int p_value) { lfo_frequency_step = p_value; }

	int get_amplitude_modulation_depth() const { return amplitude_modulation_depth; }
	void set_amplitude_modulation_depth(int p_value) { amplitude_modulation_depth = p_value; }
	bool has_amplitude_modulation() const;
	int get_pitch_modulation_depth() const { return pitch_modulation_depth; }
	void set_pitch_modulation_depth(int p_value) { pitch_modulation_depth = p_value; }
	bool has_pitch_modulation() const;

	double get_master_volume(int p_index) const;
	void set_master_volume(int p_index, double p_value);

	int get_pan() const { return pan; }
	void set_pan(int p_value) { pan = p_value; }

	int get_filter_type() const { return filter_type; }
	void set_filter_type(int p_value) { filter_type = p_value; }
	int get_filter_cutoff() const { return filter_cutoff; }
	void set_filter_cutoff(int p_value) { filter_cutoff = p_value; }
	int get_filter_resonance() const { return filter_resonance; }
	void set_filter_resonance(int p_value) { filter_resonance = p_value; }
	int get_filter_attack_rate() const { return filter_attack_rate; }
	void set_filter_attack_rate(int p_value) { filter_attack_rate = p_value; }
	int get_filter_decay_rate1() const { return filter_decay_rate1; }
	void set_filter_decay_rate1(int p_value) { filter_decay_rate1 = p_value; }
	int get_filter_decay_rate2() const { return filter_decay_rate2; }
	void set_filter_decay_rate2(int p_value) { filter_decay_rate2 = p_value; }
	int get_filter_release_rate() const { return filter_release_rate; }
	void set_filter_release_rate(int p_value) { filter_release_rate = p_value; }
	int get_filter_decay_offset1() const { return filter_decay_offset1; }
	void set_filter_decay_offset1(int p_value) { filter_decay_offset1 = p_value; }
	int get_filter_decay_offset2() const { return filter_decay_offset2; }
	void set_filter_decay_offset2(int p_value) { filter_decay_offset2 = p_value; }
	int get_filter_sustain_offset() const { return filter_sustain_offset; }
	void set_filter_sustain_offset(int p_value) { filter_sustain_offset = p_value; }
	int get_filter_release_offset() const { return filter_release_offset; }
	void set_filter_release_offset(int p_value) { filter_release_offset = p_value; }

	bool has_filter() const;
	bool has_filter_advanced() const;

	int get_lfo_frame() const;
	void set_lfo_frame(int p_fps);

	void set_by_opm_register(int p_channel, int p_address, int p_data);

	String to_string() const;

	void initialize();
	void copy_from(const Ref<SiOPMChannelParams> &p_params);

	SiOPMChannelParams();
	~SiOPMChannelParams();
};

#endif // SIOPM_CHANNEL_PARAMS_H

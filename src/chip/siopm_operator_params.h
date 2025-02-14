/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_OPERATOR_PARAMS_H
#define SIOPM_OPERATOR_PARAMS_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include "sion_enums.h"

using namespace godot;

// Operator parameters for SiONVoice.
class SiOPMOperatorParams : public RefCounted {
	GDCLASS(SiOPMOperatorParams, RefCounted)

	friend class SiOPMChannelParams;
	friend class TranslatorUtil;

public:
	// Explanations sourced from https://mml-guide.readthedocs.io/pmd/ssgeg/.
	enum SSGEnvelopeControl {
		SSG_DISABLED = 0, // Values 0-7 mean it's disabled.

		SSG_REPEAT_TO_ZERO = 8,  // Repeats the ADSR envelope upon the volume reaching 0.
		SSG_IGNORE         = 9,  // As if SSG-EG was disabled, though it is enabled.
		SSG_REPEAT_SHUTTLE = 10, // Repeats the ADSR envelope forward and backward, reversing the direction when the volume reaches zero or maximum.
		SSG_ONCE_HOLD_HIGH = 11, // Uses the ADSR envelope, then stays at the maximum volume.

		// Same as above 4, but inverted (volume 0 is maximum, and vice versa).
		SSG_REPEAT_TO_MAX          = 12,
		SSG_INVERSE                = 13,
		SSG_REPEAT_SHUTTLE_INVERSE = 14,
		SSG_ONCE_HOLD_LOW          = 15,

		// These are not standard, not exactly sure about their purpose.
		SSG_CONSTANT_HIGH = 16,
		SSG_CONSTANT_LOW  = 17,

		SSG_MAX
	};

private:
	// Pulse generator type [0,511]
	int pulse_generator_type = SiONPulseGeneratorType::PULSE_SINE;
	// Pitch table type [0,7]
	SiONPitchTableType pitch_table_type = SiONPitchTableType::PITCH_TABLE_OPM;

	// Attack rate [0,63]
	int attack_rate = 0;
	// Decay rate [0,63]
	int decay_rate = 0;
	// Sustain rate [0,63]
	int sustain_rate = 0;
	// Release rate [0,63]
	int release_rate = 0;
	// Sustain level [0,15]
	int sustain_level = 0;
	// Total level [0,127]
	int total_level = 0;

	// Key scaling rate [0,3]
	int key_scaling_rate = 0;
	// Key scaling level [0,3]
	int key_scaling_level = 0;

	// Fine multiple [0,...]
	int fine_multiple = 0;
	// Detune 1 [0,7]
	int detune1 = 0;
	// Detune 2 [0,...]
	int detune2 = 0;

	// Amp modulation shift [0,3]
	int amplitude_modulation_shift = 0;
	// Initial phase [0,255]. 255 means no phase reset.
	int initial_phase = 0;
	// 0 means pitch is not fixed.
	int fixed_pitch = 0;

	bool mute = false;
	// SSG-type envelope control [0,17].
	int ssg_envelope_control = 0;
	// Frequency modulation level [0,7]. 5 means standard modulation.
	int frequency_modulation_level = 5;
	bool envelope_reset_on_attack = false;

protected:
	static void _bind_methods();

	String _to_string() const;

public:
	int get_pulse_generator_type() const { return pulse_generator_type; }
	void set_pulse_generator_type(int p_type);
	SiONPitchTableType get_pitch_table_type() const { return pitch_table_type; }
	void set_pitch_table_type(SiONPitchTableType p_type) { pitch_table_type = p_type; }

	int get_attack_rate() const { return attack_rate; }
	void set_attack_rate(int p_value) { attack_rate = p_value; }
	int get_decay_rate() const { return decay_rate; }
	void set_decay_rate(int p_value) { decay_rate = p_value; }
	int get_sustain_rate() const { return sustain_rate; }
	void set_sustain_rate(int p_value) { sustain_rate = p_value; }
	int get_release_rate() const { return release_rate; }
	void set_release_rate(int p_value) { release_rate = p_value; }
	int get_sustain_level() const { return sustain_level; }
	void set_sustain_level(int p_value) { sustain_level = p_value; }
	int get_total_level() const { return total_level; }
	void set_total_level(int p_value) { total_level = p_value; }

	int get_key_scaling_rate() const { return key_scaling_rate; }
	void set_key_scaling_rate(int p_value) { key_scaling_rate = p_value; }
	int get_key_scaling_level() const { return key_scaling_level; }
	void set_key_scaling_level(int p_value) { key_scaling_level = p_value; }

	int get_fine_multiple() const { return fine_multiple; }
	void set_fine_multiple(int p_value) { fine_multiple = p_value; }
	// Multiple [0,15]
	int get_multiple() const;
	void set_multiple(int p_value);
	int get_detune1() const { return detune1; }
	void set_detune1(int p_value) { detune1 = p_value; }
	int get_detune2() const { return detune2; }
	void set_detune2(int p_value) { detune2 = p_value; }

	int get_amplitude_modulation_shift() const { return amplitude_modulation_shift; }
	void set_amplitude_modulation_shift(int p_value) { amplitude_modulation_shift = p_value; }
	int get_initial_phase() const { return initial_phase; }
	void set_initial_phase(int p_value) { initial_phase = p_value; }
	int get_fixed_pitch() const { return fixed_pitch; }
	void set_fixed_pitch(int p_value) { fixed_pitch = p_value; }

	bool is_mute() const { return mute; }
	void set_mute(bool p_mute) { mute = p_mute; }
	int get_ssg_envelope_control() const { return ssg_envelope_control; }
	void set_ssg_envelope_control(int p_value);
	int get_frequency_modulation_level() const { return frequency_modulation_level; }
	void set_frequency_modulation_level(int p_value) { frequency_modulation_level = p_value; }
	bool is_envelope_reset_on_attack() const { return envelope_reset_on_attack; }
	void set_envelope_reset_on_attack(bool p_reset) { envelope_reset_on_attack = p_reset; }

	void initialize();
	void copy_from(const Ref<SiOPMOperatorParams> &p_params);

	SiOPMOperatorParams();
	~SiOPMOperatorParams() {}
};

VARIANT_ENUM_CAST(SiOPMOperatorParams::SSGEnvelopeControl);

#endif // SIOPM_OPERATOR_PARAMS_H

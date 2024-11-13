/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_OPERATOR_H
#define SIOPM_OPERATOR_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "sion_enums.h"
#include "templates/singly_linked_list.h"

using namespace godot;

class SiOPMOperatorParams;
class SiOPMRefTable;
class SiOPMSoundChip;
class SiOPMWavePCMData;
class SiOPMWaveTable;

// This operator implementation is based on the OPM emulation of MAME,
// and it's extended as follows:
// 1) You can set the phase offset of the pulse generator;
// 2) You can select the wave form from wave tables (see SiOPMRefTable);
// 3) You can set the key scale level;
// 4) You can fix the pitch;
// 5) You can set SGG envelope control in OPNA.
class SiOPMOperator : public Object {
	GDCLASS(SiOPMOperator, Object)

public:
	enum EGState {
		EG_ATTACK  = 0,
		EG_DECAY   = 1,
		EG_SUSTAIN = 2,
		EG_RELEASE = 3,
		EG_OFF     = 4,

		EG_MAX
	};

private:
	static const int _eg_next_state_table[2][EG_MAX];

	SiOPMRefTable *_table = nullptr;
	SiOPMSoundChip *_sound_chip = nullptr;

	// FM module parameters.

	// Attack rate [0,63].
	int _attack_rate = 0;
	// Decay rate [0,63].
	int _decay_rate = 0;
	// Sustain rate [0,63].
	int _sustain_rate = 0;
	// Release rate [0,63].
	int _release_rate = 0;
	// Sustain level [0,15].
	int _sustain_level = 0;
	// Total level [0,127].
	int _total_level = 0;
	// Key scaling rate = 5-ks [5,2].
	int _key_scaling_rate = 0;
	// Key scaling level [0,3].
	int _key_scaling_level = 0;
	// Fine multiple [64,128,256,384,512...].
	int _fine_multiple = 0;
	// Raw detune1 [0,7].
	int _detune1 = 0;
	// Raw detune2 [0,3].
	int _detune2 = 0;
	// Amp modulation shift [16,0].
	int _amplitude_modulation_shift = 0;
	// Key code = oct << 4 + note [0,127].
	int _key_code = 0;

	// Mute [0 / SiOPMRefTable::ENV_BOTTOM].
	int _mute = 0;
	int _ssg_type_envelope_control = 0;
	bool _envelope_reset_on_attack = false;

	void _update_key_code(int p_value);
	void _update_total_level();

	// Pulse generator.

	int _pg_type = SiONPulseGeneratorType::PULSE_SINE;
	SiONPitchTableType _pt_type = SiONPitchTableType::PITCH_TABLE_OPM;
	Vector<int> _wave_table;
	// Phase shift.
	int _wave_fixed_bits = 0;
	// Phase step shift.
	int _wave_phase_step_shift = 0;
	Vector<int> _pitch_table;
	int _pitch_table_filter = 0;

	int _phase = 0;
	int _phase_step = 0;
	// -1 means no phase reset.
	int _key_on_phase = 0;
	bool _pitch_fixed = false;

	// Pitch index = note * 64 + key fraction.
	int _pitch_index = 0;
	// Detune for pTSS. 1 halftone divides into 64 steps.
	int _pitch_index_shift = 0;
	// Detune for pitch modulation.
	int _pitch_index_shift2 = 0;
	// Frequency modulation left-shift. 15 for FM, fb+6 for feedback.
	int _fm_shift = 0;

	void _update_pitch();
	void _update_phase_step(int p_step);

	// Envelope generator.

	EGState _eg_state = EG_OFF;
	int _eg_timer = 0;
	// Timer stepping by samples.
	int _eg_timer_step = 0;
	// Counter rounded on 8.
	int _eg_counter = 0;
	// Internal sustain level [0, SiOPMRefTable::ENV_BOTTOM].
	int _eg_sustain_level = 0;
	// Internal total level [0,1024] = ((tl + f(kc, ksl)) << 3) + _eg_tl_offset + 192.
	int _eg_total_level = 0;
	// Internal total level offset by volume [-192,832].
	int _eg_tl_offset = 0;
	// Internal key scaling rate = _kc >> _ks [0,32].
	int _eg_key_scale_rate = 0;
	// Internal key scaling level right shift = _ksl[0,1,2,3]->[8,2,1,0].
	int _eg_key_scale_level_rshift = 0;
	// Envelope generator level [0,1024].
	int _eg_level = 0;
	// Envelope generator output [0,1024<<3].
	int _eg_output = 0;
	// SSG envelope control attack rate switch.
	int _eg_ssgec_attack_rate = 0;
	// SSG envelope control state.
	int _eg_ssgec_state = 0;

	Vector<int> _eg_increment_table;
	int _eg_state_shift_level = 0;
	int _eg_state_table_index = 0;
	Vector<int> _eg_level_table;

	void _shift_eg_state(EGState p_state);

	// PCM wave.

	int _pcm_channel_num = 0;
	int _pcm_start_point = 0;
	int _pcm_end_point = 0;
	int _pcm_loop_point = 0;

	// Pipes.

	bool _final = false;
	SinglyLinkedList<int> *_in_pipe = nullptr;
	SinglyLinkedList<int> *_base_pipe = nullptr;
	SinglyLinkedList<int> *_out_pipe = nullptr;
	SinglyLinkedList<int> *_feed_pipe = nullptr;

protected:
	static void _bind_methods() {}

	String _to_string() const;

public:
	static const int PCM_WAVE_FIXED_BITS = 11;

	// FM module parameters.

	int get_attack_rate() const { return _attack_rate; }
	void set_attack_rate(int p_value);

	int get_decay_rate() const { return _decay_rate; }
	void set_decay_rate(int p_value);

	int get_sustain_rate() const { return _sustain_rate; }
	void set_sustain_rate(int p_value);

	int get_release_rate() const { return _release_rate; }
	void set_release_rate(int p_value);

	int get_sustain_level() const { return _sustain_level; }
	void set_sustain_level(int p_value);

	int get_total_level() const { return _total_level; }
	void set_total_level(int p_value);
	void offset_total_level(int p_offset);

	int get_key_scaling_rate() const;
	void set_key_scaling_rate(int p_value);

	int get_key_scaling_level() const { return _key_scaling_level; }
	void set_key_scaling_level(int p_value, bool p_silent = false);

	int get_multiple() const;
	void set_multiple(int p_value);

	// Fine multiple for pTSS. 128=x1.
	int get_fine_multiple() const { return _fine_multiple; }
	void set_fine_multiple(int p_value);

	int get_detune1() const { return _detune1; }
	void set_detune1(int p_value);

	int get_detune2() const { return _detune2; }
	void set_detune2(int p_value);

	bool is_amplitude_modulation_enabled() const;
	void set_amplitude_modulation_enabled(bool p_enabled);

	int get_amplitude_modulation_shift() const;
	void set_amplitude_modulation_shift(int p_value);

	int get_key_code() const { return _key_code; }
	void set_key_code(int p_value);

	bool is_mute() const;
	void set_mute(bool p_mute);

	int get_ssg_type_envelope_control() const { return _ssg_type_envelope_control; }
	void set_ssg_type_envelope_control(int p_value);

	bool is_envelope_reset_on_attack() const { return _envelope_reset_on_attack; }
	void set_envelope_reset_on_attack(bool p_reset) { _envelope_reset_on_attack = p_reset; }

	// Pulse generator.

	int get_pulse_generator_type() const { return _pg_type; }
	void set_pulse_generator_type(int p_type);
	SiONPitchTableType get_pitch_table_type() const { return _pt_type; }
	void set_pitch_table_type(SiONPitchTableType p_type);

	int get_wave_value(int p_index) const;
	int get_wave_fixed_bits() const { return _wave_fixed_bits; }

	int get_phase() const { return _phase; }
	void set_phase(int p_value) { _phase = p_value; }
	void adjust_phase(int p_diff) { _phase += p_diff; }

	int get_key_on_phase_raw() const { return _key_on_phase; }

	int get_pitch_index() const { return _pitch_index; }
	void set_pitch_index(int p_value);
	bool is_pitch_fixed() const { return _pitch_fixed; }
	// Setting to 0 disables the pitch fixed flag.
	void set_fixed_pitch_index(int p_value);

	int get_ptss_detune() const { return _pitch_index_shift; }
	void set_ptss_detune(int p_value);
	int get_pm_detune() const { return _pitch_index_shift2; }
	void set_pm_detune(int p_value);

	int get_fm_shift() const { return _fm_shift; }

	// The value of 255 means no phase reset, -1 means random.
	int get_key_on_phase() const;
	void set_key_on_phase(int p_phase);

	// Frequency modulation level. 15 is standard modulation.
	int get_fm_level() const;
	void set_fm_level(int p_level);

	// Key fraction [0-63].
	int get_key_fraction() const;
	void set_key_fraction(int p_value);

	// F-Number for OPNA.
	void set_fnumber(int p_value);

	void tick_pulse_generator(int p_extra = 0);

	// Envelope generator.

	EGState get_eg_state() const { return _eg_state; }
	void set_eg_state(EGState p_state) { _shift_eg_state(p_state); }

	int get_eg_output() const { return _eg_output; }
	void tick_eg(int p_timer_initial);
	void update_eg_output();
	void update_eg_output_from(SiOPMOperator *p_other);

	// PCM wave.

	int get_pcm_channel_num() const { return _pcm_channel_num; }
	int get_pcm_start_point() const { return _pcm_start_point; }
	int get_pcm_end_point() const { return _pcm_end_point; }
	int get_pcm_loop_point() const { return _pcm_loop_point; }

	// Pipes.

	bool is_final() const { return _final; }

	SinglyLinkedList<int> *get_in_pipe() const { return _in_pipe; }
	SinglyLinkedList<int> *get_base_pipe() const { return _base_pipe; }
	SinglyLinkedList<int> *get_out_pipe() const { return _out_pipe; }
	SinglyLinkedList<int> *get_feed_pipe() const { return _feed_pipe; }

	void set_pipes(SinglyLinkedList<int> *p_out_pipe, SinglyLinkedList<int> *p_in_pipe = nullptr, bool p_final = false);
	void set_base_pipe(SinglyLinkedList<int> *p_pipe) { _base_pipe = p_pipe; }

	//

	void set_operator_params(const Ref<SiOPMOperatorParams> &p_params);
	void get_operator_params(const Ref<SiOPMOperatorParams> &r_params);
	void set_wave_table(const Ref<SiOPMWaveTable> &p_wave_table);
	void set_pcm_data(const Ref<SiOPMWavePCMData> &p_pcm_data);

	void note_on();
	void note_off();

	//

	void initialize();
	void reset();

	SiOPMOperator(SiOPMSoundChip *p_chip = nullptr);
	~SiOPMOperator() {}
};

#endif // SIOPM_OPERATOR_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_BASE_H
#define SIOPM_CHANNEL_BASE_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include "sion_enums.h"
#include "chip/channels/siopm_channel_manager.h"
#include "chip/siopm_ref_table.h"
#include "templates/singly_linked_list.h"

using namespace godot;

class SiOPMChannelParams;
class SiOPMSoundChip;
class SiOPMStream;
class SiOPMWaveBase;

// SiOPM sound channel base class.
// Sound channels generate wave data and write it into streaming buffer.
class SiOPMChannelBase : public Object {
	GDCLASS(SiOPMChannelBase, Object)

	friend class SiOPMChannelManager;

	// LP filter envelope status.
	enum FilterState {
		EG_ATTACK = 0,
		EG_DECAY1 = 1,
		EG_DECAY2 = 2,
		EG_SUSTAIN = 3,
		EG_RELEASE = 4,
		EG_OFF = 5,
	};

public:
	enum OutputMode {
		OUTPUT_STANDARD = 0,  // Standard output.
		OUTPUT_OVERWRITE = 1, // Overwrite output pipe.
		OUTPUT_ADD = 2,       // Add to output pipe.
	};

	enum InputMode {
		INPUT_ZERO = 0,     // No input from pipe.
		INPUT_PIPE = 1,     // Input from pipe.
		INPUT_FEEDBACK = 2, /// Input from feedback.
	};

	enum FilterType {
		FILTER_LP = 0, // Low pass.
		FILTER_BP = 1, // Band pass.
		FILTER_HP = 2, // High pass.
	};

private:
	bool _is_free = true;
	SiOPMChannelManager::ChannelType _channel_type = SiOPMChannelManager::CHANNEL_MAX;
	SiOPMChannelBase *_next = nullptr;
	SiOPMChannelBase *_prev = nullptr;

protected:
	static void _bind_methods();

	//

	SiOPMRefTable *_table = nullptr;
	SiOPMSoundChip *_sound_chip = nullptr;

	Callable _process_function;

	void _no_process(int p_length);

	bool _is_note_on = false;

	// Pipe buffer.

	int _buffer_index = 0;
	int _input_level = 0;
	double _ringmod_level = 0;
	InputMode _input_mode = InputMode::INPUT_ZERO;
	OutputMode _output_mode = OutputMode::OUTPUT_STANDARD;
	SinglyLinkedList<int> *_in_pipe = nullptr;
	SinglyLinkedList<int> *_ring_pipe = nullptr;
	SinglyLinkedList<int> *_base_pipe = nullptr;
	SinglyLinkedList<int> *_out_pipe = nullptr;

	// Volume and stream.

	Vector<SiOPMStream *> _streams;
	Vector<double> _volumes;
	bool _is_idling = true;
	int _pan = 64;
	bool _has_effect_send = false;
	bool _mute = false;
	int _velocity_table[SiOPMRefTable::TL_TABLE_SIZE];
	int _expression_table[SiOPMRefTable::TL_TABLE_SIZE];

	// Low pass filter.

	bool _filter_on = false;
	int _filter_type = 0;
	int _cutoff_frequency = 0;
	int _cutoff_offset = 0;
	double _resonance = 0;
	double _filter_variables[3] = {};
	int _filter_eg_residue = 0;
	int _filter_eg_step = 0;
	int _filter_eg_next = 0;       // Phase shift.
	int _filter_eg_cutoff_inc = 0; // Direction.
	int _filter_eg_state = 0;
	int _filter_eg_time[6] = {};   // Rate.
	int _filter_eg_cutoff[6] = {}; // Level.

	// Low frequency oscillator (LFO).

	int _frequency_ratio = 0;
	int _lfo_on = 0; // Treated as a boolean flag.
	int _lfo_timer = 0;
	int _lfo_timer_step = 0;
	int _lfo_timer_step_buffer = 0;
	int _lfo_phase = 0;
	Vector<int> _lfo_wave_table;
	int _lfo_wave_shape = 0;

	SinglyLinkedList<int> *_rotate_pipe(SinglyLinkedList<int> *p_pipe, int p_length);
	void _apply_ring_modulation(SinglyLinkedList<int> *p_target, int p_length);
	// NOTE: Original code would implicitly use the filter variables if nothing was passed as the 3rd argument. We make this explicit.
	void _apply_sv_filter(SinglyLinkedList<int> *p_target, int p_length, double (&r_variables)[3]);
	void _reset_sv_filter_state();
	bool _try_shift_sv_filter_state(int p_state);
	void _shift_sv_filter_state(int p_state);

public:
	SiOPMChannelManager::ChannelType get_channel_type() const { return _channel_type; }

	virtual void get_channel_params(const Ref<SiOPMChannelParams> &p_params) const {}
	virtual void set_channel_params(const Ref<SiOPMChannelParams> &p_params, bool p_with_volume, bool p_with_modulation = true) {}

	virtual void set_wave_data(const Ref<SiOPMWaveBase> &p_wave_data) {}
	virtual void set_channel_number(int p_value) {}
	virtual void set_register(int p_address, int p_data) {}

	virtual void set_algorithm(int p_operator_count, bool p_analog_like, int p_algorithm) {}
	virtual void set_feedback(int p_level, int p_connection) {}
	virtual void set_parameters(Vector<int> p_params) {}
	virtual void set_types(int p_pg_type, SiONPitchTableType p_pt_type) {}
	virtual void set_all_attack_rate(int p_value) {}
	virtual void set_all_release_rate(int p_value) {}

	virtual int get_master_volume() const;
	virtual void set_master_volume(int p_value);

	virtual int get_pan() const;
	virtual void set_pan(int p_value);

	virtual bool is_mute() const { return _mute; }
	virtual void set_mute(bool p_value) { _mute = p_value; }

	virtual int get_pitch() const { return 0; }
	virtual void set_pitch(int p_value) {}

	virtual void set_active_operator_index(int p_value) {}
	virtual void set_release_rate(int p_value) {}
	virtual void set_total_level(int p_value) {}
	virtual void set_fine_multiple(int p_value) {}
	virtual void set_phase(int p_value) {}
	virtual void set_detune(int p_value) {}
	virtual void set_fixed_pitch(int p_value) {}
	virtual void set_ssg_envelope_control(int p_value) {}
	virtual void set_envelope_reset(bool p_reset) {}

	virtual int get_buffer_index() const { return _buffer_index; }
	virtual bool is_note_on() const { return _is_note_on; }
	virtual bool is_idling() const { return _is_idling; }

	virtual bool is_filter_active() const { return _filter_on; }
	virtual int get_filter_type() const { return _filter_type; }
	virtual void set_filter_type(int p_type);

	// Volume control.

	virtual void set_all_stream_send_levels(Vector<int> p_levels);
	virtual void set_stream_buffer(int p_stream_num, SiOPMStream *p_stream = nullptr);
	virtual void set_stream_send(int p_stream_num, double p_volume);
	virtual double get_stream_send(int p_stream_num);
	virtual void offset_volume(int p_expression, int p_velocity) {}

	// LFO control.

	virtual void set_frequency_ratio(int p_ratio) { _frequency_ratio = p_ratio; }
	virtual void initialize_lfo(int p_waveform, Vector<int> p_custom_wave_table = Vector<int>());
	virtual void set_lfo_cycle_time(double p_ms);
	virtual void set_amplitude_modulation(int p_depth) {}
	virtual void set_pitch_modulation(int p_depth) {}

	// Filter control.

	virtual void activate_filter(bool p_active) { _filter_on = p_active; }
	virtual void set_sv_filter(int p_cutoff = 128, int p_resonance = 0, int p_attack_rate = 0, int p_decay_rate1 = 0, int p_decay_rate2 = 0, int p_release_rate = 0, int p_decay_cutoff1 = 128, int p_decay_cutoff2 = 128, int p_sustain_cutoff = 128, int p_release_cutoff = 128);
	virtual void offset_filter(int p_offset);

	// Connection control.

	virtual void set_input(int p_level, int p_pipe_index);
	virtual void set_ring_modulation(int p_level, int p_pipe_index);
	virtual void set_output(OutputMode p_output_mode, int p_pipe_index);
	virtual void set_volume_tables(int (&p_velocity_table)[SiOPMRefTable::TL_TABLE_SIZE], int (&p_expression_table)[SiOPMRefTable::TL_TABLE_SIZE]);

	// Processing.

	virtual void note_on();
	virtual void note_off();

	virtual void reset_channel_buffer_status();
	virtual void buffer(int p_length);
	virtual void buffer_no_process(int p_length);

	//

	virtual String to_string() const { return "SiOPMChannelBase"; }

	virtual void initialize(SiOPMChannelBase *p_prev, int p_buffer_index);
	virtual void reset();

	SiOPMChannelBase(SiOPMSoundChip *p_chip = nullptr);
	~SiOPMChannelBase() {}
};

#endif // SIOPM_CHANNEL_BASE_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_TRACK_H
#define SIMML_TRACK_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "sion_enums.h"
#include "sequencer/base/beats_per_minute.h"
#include "sequencer/simml_data.h"
#include "sequencer/simml_ref_table.h"
#include "templates/singly_linked_list.h"

using namespace godot;

class MMLExecutor;
class MMLSequence;
class SiMMLChannelSettings;
class SiMMLEnvelopeTable;
class SiOPMChannelBase;

class SiMMLTrack : public Object {
	GDCLASS(SiMMLTrack, Object)

public:
	// Mask bits for event_mask and @mask command.
	enum MaskBits {
		NO_MASK       =  0,
		MASK_VOLUME   =  1, // Mask volume commands (v,x,&#64;v,"(",")")
		MASK_PAN      =  2, // Mask panning commands (p,&#64;p)
		MASK_QUANTIZE =  4, // Mask quantize commands (q,&#64;q)
		MASK_OPERATOR =  8, // Mask operator setting commands (s,&#64;al,&#64;fb,i,&#64;,&#64;rr,&#64;tl,&#64;ml,&#64;st,&#64;ph,&#64;fx,&#64;se,&#64;er)
		MASK_ENVELOPE = 16, // Mask table envelop commands (&#64;&#64;,na,np,nt,nf,_&#64;&#64;,_na,_np,_nt,_nf)
		MASK_MODULATE = 32, // Mask modulation commands (ma,mp)
		MASK_SLUR     = 64, // Mask slur and pitch-bending commands (&amp;,&amp;&amp;,*)
	};

	enum ProcessMode {
		NORMAL = 0,
		ENVELOPE = 2,
	};

	enum EventTriggerType {
		NO_EVENTS = 0,
		EVENT_FRAME = 1,
		EVENT_STREAM = 2,

		EVENT_BOTH = EVENT_FRAME | EVENT_STREAM,
	};

private:

	static const int SWEEP_FINESS = 128;
	static const int FIXED_BITS = 16;
	static const int SWEEP_MAX = 8192 << FIXED_BITS;

	static SinglyLinkedList<int> *_envelope_zero_table;

	// Properties and data.

	// Sound module's channel controlled by this track.
	SiOPMChannelBase *_channel = nullptr;
	MMLExecutor *_executor = nullptr;
	SiMMLChannelSettings *_channel_settings = nullptr;

	Ref<SiMMLData> _mml_data;
	SiMMLRefTable *_table = nullptr;

	// This value is specified by user and contains the track starter.
	int _internal_track_id = 0;
	// This value is unique and set by system, the lower numbered track processes sound first.
	int _track_number = 0;
	int _channel_number = 0;

	int _process_mode = ProcessMode::NORMAL;
	int _track_start_delay = 0;
	int _track_stop_delay = 0;
	bool _stop_with_reset = false;
	bool _is_disposable = false;
	// Priority number to overwrite when tracks overflow.
	int _priority = 0;
	int _default_fps = 0;

	int _velocity_mode = 0;
	int _velocity_shift = 0;
	int _expression_mode = 0;
	// Linked to operator's total level.
	int _velocity = 0;   // [0-256-512]
	int _expression = 0; // [0-128]

	int _pitch_index = 0;
	int _pitch_bend = 0;
	int _pitch_shift = 0;
	int _voice_index = 0; // Tone number
	int _note = 0;        // Note number
	int _note_shift = 0;
	double _quantize_ratio = 0;
	int _quantize_count = 0;

	// Settings.

	int _setting_process_mode[2] = {};

	Vector<SinglyLinkedList<int> *> _setting_envelope_exp;
	Vector<SinglyLinkedList<int> *> _setting_envelope_voice;
	Vector<SinglyLinkedList<int> *> _setting_envelope_note;
	Vector<SinglyLinkedList<int> *> _setting_envelope_pitch;
	Vector<SinglyLinkedList<int> *> _setting_envelope_filter;
	bool _setting_exp_offset[2] = {};
	// PNS (pitch, note, sweep)
	bool _setting_pns_or[2] = {};

	int _setting_counter_exp[2] = {};
	int _setting_counter_voice[2] = {};
	int _setting_counter_note[2] = {};
	int _setting_counter_pitch[2] = {};
	int _setting_counter_filter[2] = {};

	Vector<SinglyLinkedList<int> *> _table_envelope_mod_amp;
	Vector<SinglyLinkedList<int> *> _table_envelope_mod_pitch;
	int _setting_sweep_step[2] = {};
	int _setting_sweep_end[2] = {};
	int _envelope_interval = 0;

	// Envelopes.

	SinglyLinkedList<int> *_envelope_exp = nullptr;
	SinglyLinkedList<int> *_envelope_voice = nullptr;
	SinglyLinkedList<int> *_envelope_note = nullptr;
	SinglyLinkedList<int> *_envelope_pitch = nullptr;
	SinglyLinkedList<int> *_envelope_filter = nullptr;

	int _counter_exp = 0;
	int _max_counter_exp = 0;
	int _counter_voice = 0;
	int _max_counter_voice = 0;
	int _counter_note = 0;
	int _max_counter_note = 0;
	int _counter_pitch = 0;
	int _max_counter_pitch = 0;
	int _counter_filter = 0;
	int _max_counter_filter = 0;

	SinglyLinkedList<int> *_envelope_mod_amp = nullptr;
	SinglyLinkedList<int> *_envelope_mod_pitch = nullptr;
	int _sweep_step = 0;
	int _sweep_end = 0;
	int _sweep_pitch = 0;
	int _envelope_exp_offset = 0;
	bool _envelope_pitch_active = false;

	// Residue of the previous envelope process
	int _residue = 0;

	SinglyLinkedList<int> *_make_modulation_table(int p_depth, int p_end_depth, int p_delay, int p_term);

	// Events.

	uint32_t _event_mask = MaskBits::NO_MASK;

	Callable _callback_before_note_on;
	Callable _callback_before_note_off;
	Callable _callback_update_register;

	Callable _event_trigger_on;
	Callable _event_trigger_off;
	// -1 means trigger is not set.
	int _event_trigger_id = -1;
	// 0 means trigger is not set.
	EventTriggerType _event_trigger_type_on = EventTriggerType::NO_EVENTS;
	// 0 means trigger is not set.
	EventTriggerType _event_trigger_type_off = EventTriggerType::NO_EVENTS;

	void _default_update_register(int p_address, int p_data);

	// Playback.

	int _key_on_counter = 0;
	int _key_on_length = 0;
	int _key_on_delay = 0;
	bool _flag_no_key_on = false;

	void _enable_envelope_mode(int p_note_on);
	void _disable_envelope_mode(int p_note_on);
	int _buffer_envelope(int p_length, int p_step);
	void _process_buffer(int p_length);

	void _toggle_key();
	void _key_on();
	void _key_off();
	void _update_process(int p_key_on);

	void _mml_key_on(int p_note);

protected:
	static void _bind_methods();

public:
	static const int TRACK_ID_FILTER = 0xffff;
	static const int TRACK_TYPE_FILTER = 0xff0000;

	enum TrackTypeID {
		MML_TRACK         = 0x10000, // Main MML tracks
		MIDI_TRACK        = 0x20000, // MIDI tracks
		DRIVER_NOTE       = 0x30000, // Tracks created by SiONDriver.noteOn() or SiONDriver.playSound()
		DRIVER_SEQUENCE   = 0x40000, // Tracks created by SiONDriver.sequenceOn()
		DRIVER_BACKGROUND = 0x50000, // SiONDriver's background sound tracks
		USER_CONTROLLED   = 0x60000, // User controlled tracks
	};

	// Properties and data.

	SiOPMChannelBase *get_channel() const { return _channel; }
	void set_channel(SiOPMChannelBase *p_channel) { _channel = p_channel; }
	MMLExecutor *get_executor() const { return _executor; }
	MMLSequence *set_channel_parameters(Vector<int> p_params);

	int get_track_number() const { return _track_number; }
	void set_track_number(int p_number) { _track_number = p_number; }

	int get_internal_track_id() const { return _internal_track_id; }
	int get_track_id() const;
	int get_track_type_id() const;

	// This value only is available in the track playing an MML sequence.
	Ref<SiMMLData> get_mml_data() const { return _mml_data; }
	Ref<BeatsPerMinute> get_bpm_settings() const;

	// Channel number, set by 2nd argument of % command. Usually same as voice index / program number (except for APU).
	int get_channel_number() const { return _channel_number; }
	void set_channel_number(int p_number) { _channel_number = p_number; }
	// Program number, set by 2nd argument of % command and 1st arg. of &#64; command. Usually same as channel number (except for APU).
	int get_program_number() const { return _voice_index; }

	// Start delay in sample count. Usualy this returns 0 except after SiONDriver's note_on.
	int get_track_start_delay() const { return _track_start_delay; }
	// Stop delay in sample count. Usualy this returns 0 except after SiONDriver's note_off.
	int get_track_stop_delay() const { return _track_stop_delay; }

	int get_priority() const;

	// This function always returns true from not-disposable track.
	bool is_active() const;
	bool is_playing_sequence() const;
	bool is_finished() const;

	// Disposable track will free automatically when finished rendering.
	bool is_disposable() const { return _is_disposable; };
	void set_disposable() { _is_disposable = true; }

	int get_velocity_mode() const { return _velocity_mode; }
	void set_velocity_mode(int p_mode);

	int get_velocity_shift() const { return _velocity_shift; }
	void set_velocity_shift(int p_value) { _velocity_shift = p_value; }

	int get_expression_mode() const { return _expression_mode; }
	void set_expression_mode(int p_mode);

	int get_velocity() const { return _velocity; }
	void set_velocity(int p_value);

	int get_expression() const { return _expression; }
	void set_expression(int p_value);

	double get_output_level() const;

	int get_pitch_bend() const { return _pitch_bend; }
	void set_pitch_bend(int p_value);
	void start_pitch_bending(int p_note_from, int p_tick_length);

	int get_pitch_shift() const { return _pitch_shift; }
	void set_pitch_shift(int p_value) { _pitch_shift = p_value; }

	int get_note() const { return _note; }
	void set_note_immediately(int p_note, int p_sample_length, bool p_slur = false);

	int get_note_shift() const { return _note_shift; }
	void set_note_shift(int p_value) { _note_shift = p_value; }

	double get_quantize_ratio() const { return _quantize_ratio; }
	void set_quantize_ratio(double p_value) { _quantize_ratio = p_value; }

	int get_quantize_count() const { return _quantize_count; }
	void set_quantize_count(int p_value) { _quantize_count = p_value; }

	// Channel properties.

	void set_channel_module_type(SiONModuleType p_type, int p_channel_num = INT32_MIN, int p_tone_num = INT32_MIN);

	void reset_volume_offset();

	int get_master_volume() const;
	void set_master_volume(int p_value);

	int get_effect_send1() const;
	int get_effect_send2() const;
	int get_effect_send3() const;
	int get_effect_send4() const;

	void set_effect_send1(int p_value);
	void set_effect_send2(int p_value);
	void set_effect_send3(int p_value);
	void set_effect_send4(int p_value);

	bool is_mute() const;
	void set_mute(bool p_value);

	int get_pan() const;
	void set_pan(int p_value);

	// Envelopes.

	void set_portament(int p_frame);

	void set_envelope_fps(int p_fps);
	void set_release_sweep(int p_sweep);
	void set_modulation_envelope(bool p_is_pitch_mod, int p_depth, int p_end_depth, int p_delay, int p_term);
	void set_tone_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step);
	void set_amplitude_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step, bool p_offset = false);
	void set_filter_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step);
	void set_pitch_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step);
	void set_note_envelope(int p_note_on, SiMMLEnvelopeTable *p_table, int p_step);

	// Events.

	uint32_t get_event_mask() const { return _event_mask; }
	void set_event_mask(uint32_t p_mask) { _event_mask = p_mask; }

	int get_event_trigger_id() const { return _event_trigger_id; }
	EventTriggerType get_event_trigger_type_on() const { return _event_trigger_type_on; }
	EventTriggerType get_event_trigger_type_off() const { return _event_trigger_type_off; }

	void set_update_register_callback(const Callable &p_func);
	void call_update_register(int p_address, int p_data);

	void set_note_on_callback(const Callable &p_func = nullptr);
	void set_note_off_callback(const Callable &p_func = nullptr);
	// This overrides custom note callbacks.
	void set_event_trigger_callbacks(int p_id, EventTriggerType p_note_on_type = EventTriggerType::EVENT_FRAME, EventTriggerType p_note_off_type = EventTriggerType::NO_EVENTS);

	void trigger_note_callback(bool p_note_on);
	void trigger_note_on_event(int p_id, EventTriggerType p_trigger_type = EventTriggerType::EVENT_FRAME);

	// Handler for MMLEvent::REST
	void handle_rest_event();
	// Handler for MMLEvent::NOTE
	void handle_note_event(int p_note, int p_length);
	// Slur without next notes key on. This has to be called just after key_on().
	void handle_slur();
	// Slur with next notes key on. This has to be called just after key_on().
	void handle_slur_weak();
	// Set pitch bend (and slur) immediately.
	void handle_pitch_bend(int p_next_note, int p_term);
	// Handler for MML v command.
	void handle_velocity(int p_value);
	// Handler for MML v command's shift.
	void handle_velocity_shift(int p_value);

	// Playback.

	int prepare_buffer(int p_buffer_length);
	void buffer(int p_length);

	void key_on(int p_note, int p_tick_length = 0, int p_sample_delay = 0);
	void key_off(int p_sample_delay = 0, bool p_with_reset = false);

	void sequence_on(MMLSequence *p_sequence, int p_sample_length = 0, int p_sample_delay = 0);
	void sequence_off(int p_sample_delay = 0, bool p_with_reset = false);

	void limit_key_length(int p_stop_delay);
	void change_note_length(int p_length);
	void set_key_on_delay(int p_delay) { _key_on_delay = p_delay; }

	//

	void reset(int p_buffer_index);
	void initialize(MMLSequence *p_sequence, int p_fps, int p_internal_track_id, const Callable &p_event_trigger_on, const Callable &p_event_trigger_off, bool p_disposable);

	SiMMLTrack();
	~SiMMLTrack();
};

#endif // SIMML_TRACK_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SEQUENCER_H
#define SIMML_SEQUENCER_H

#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include "sequencer/base/mml_sequencer.h"

using namespace godot;

class MMLExecutorConnector;
class MMLSequenceGroup;
class SiMMLTable;
class SiMMLTrack;
class SiOPMChannelParams;
class SiOPMModule;

// The SiMMLSequencer operates SiOPMModule by MML.
// SiMMLSequencer -> SiMMLTrack -> SiOPMChannelFM -> SiOPMOperator. (-> means "operates")
class SiMMLSequencer : public MMLSequencer {
	GDCLASS(SiMMLSequencer, MMLSequencer)

	static const int MAX_PARAM_COUNT = 16;
	static const int MACRO_SIZE = 26;
	static const int DEFAULT_MAX_TRACK_COUNT = 128;

	SiMMLTable *_table = nullptr;
	SiOPMModule *_module = nullptr;
	MMLExecutorConnector *_connector = nullptr;

	String _title;

	// Tracks.

	List<SiMMLTrack *> _free_tracks;

	int _max_track_count = DEFAULT_MAX_TRACK_COUNT;
	Vector<SiMMLTrack *> _tracks;
	SiMMLTrack *_current_track = nullptr;

	int _processed_sample_count = 0;
	bool _is_sequence_finished = true;

	void _free_all_tracks();
	void _initialize_track(SiMMLTrack *p_track, int p_internal_track_id, bool p_disposable);
	SiMMLTrack *_find_lowest_priority_track();

	// Compilation and processing.

	bool _dummy_process = false;
	bool _bpm_change_enabled = false;

	virtual String _on_before_compile(String p_mml) override;
	virtual void _on_after_compile(MMLSequenceGroup *p_group) override;
	virtual void _on_process(int p_length, MMLEvent *p_event) override;
	virtual void _on_timer_interruption() override;
	virtual void _on_beat(int p_delay_samples, int p_beat_counter) override;
	virtual void _on_table_parse(MMLEvent *p_prev, String p_table) override;
	virtual void _on_tempo_changed(double p_tempo_ratio) override;

	// Parser.

	Vector<String> _macro_strings;
	bool _macro_expand_dynamic = false;

	String _expand_macro(String p_macro, bool p_nested = false);

	int _internal_table_index = 0;

	void _reset_parser_parameters();

	void _parse_command_init_sequence(SiOPMChannelParams *p_params, String p_postfix);
	void _parse_tmode_command(String p_mml);
	void _parse_vmode_command(String p_mml);
	bool _try_set_sampler_wave(int p_index, String p_mml);
	bool _try_set_pcm_wave(int p_index, String p_mml);
	bool _try_set_pcm_voice(int p_index, String p_mml, String p_postfix);
	void _try_process_command_callback(String p_command, int p_number, String p_content, String p_postfix);

	bool _parse_system_command_before(String p_command, String p_param);
	MMLSequence *_parse_system_command_after(MMLSequenceGroup *p_seq_group, MMLSequence *p_command_seq);

	// Internal callbacks.

	/// Processing events.

	MMLEvent *_on_mml_rest(MMLEvent *p_event);           // rest
	MMLEvent *_on_mml_note(MMLEvent *p_event);           // note
	MMLEvent *_on_mml_driver_note_on(MMLEvent *p_event); // SiONDriver::note_on()
	MMLEvent *_on_mml_slur(MMLEvent *p_event);           // &
	MMLEvent *_on_mml_slur_weak(MMLEvent *p_event);      // &&
	MMLEvent *_on_mml_pitch_bend(MMLEvent *p_event);     // *

	// Driver track events.

	MMLEvent *_on_mml_quant_ratio(MMLEvent *p_event);     // quantize ratio
	MMLEvent *_on_mml_quant_count(MMLEvent *p_event);     // quantize count
	MMLEvent *_on_mml_event_mask(MMLEvent *p_event);      // @mask
	MMLEvent *_on_mml_detune(MMLEvent *p_event);          // k
	MMLEvent *_on_mml_key_transition(MMLEvent *p_event);  // kt
	MMLEvent *_on_mml_relative_detune(MMLEvent *p_event); // !@kr

	// Envelope events.

	MMLEvent *_on_mml_envelope_fps(MMLEvent *p_event);               // @fps
	MMLEvent *_on_mml_tone_envelope(MMLEvent *p_event);              // @@
	MMLEvent *_on_mml_amplitude_envelope(MMLEvent *p_event);         // na
	MMLEvent *_on_mml_amplitude_envelope_tsscp(MMLEvent *p_event);   // !na
	MMLEvent *_on_mml_pitch_envelope(MMLEvent *p_event);             // np
	MMLEvent *_on_mml_note_envelope(MMLEvent *p_event);              // nt
	MMLEvent *_on_mml_filter_envelope(MMLEvent *p_event);            // nf
	MMLEvent *_on_mml_tone_release_envelope(MMLEvent *p_event);      // _@@
	MMLEvent *_on_mml_amplitude_release_envelope(MMLEvent *p_event); // _na
	MMLEvent *_on_mml_pitch_release_envelope(MMLEvent *p_event);     // _np
	MMLEvent *_on_mml_note_release_envelope(MMLEvent *p_event);      // _nt
	MMLEvent *_on_mml_filter_release_envelope(MMLEvent *p_event);    // _nf

	// Internal table envelope events.

	MMLEvent *_on_mml_filter(MMLEvent *p_event);               // @f
	MMLEvent *_on_mml_filter_mode(MMLEvent *p_event);          // %f
	MMLEvent *_on_mml_lf_oscillator(MMLEvent *p_event);        // @lfo[cycle_frames],[ws]
	MMLEvent *_on_mml_pitch_modulation(MMLEvent *p_event);     // mp [depth],[end_depth],[delay],[term]
	MMLEvent *_on_mml_amplitude_modulation(MMLEvent *p_event); // ma [depth],[end_depth],[delay],[term]
	MMLEvent *_on_mml_portament(MMLEvent *p_event);            // po [term]

	// IO events.

	MMLEvent *_on_mml_volume(MMLEvent *p_event);             // v
	MMLEvent *_on_mml_volume_shift(MMLEvent *p_event);       // (, )
	MMLEvent *_on_mml_volume_setting(MMLEvent *p_event);     // %v
	MMLEvent *_on_mml_expression(MMLEvent *p_event);         // x
	MMLEvent *_on_mml_expression_setting(MMLEvent *p_event); // %x
	MMLEvent *_on_mml_master_volume(MMLEvent *p_event);      // @v
	MMLEvent *_on_mml_pan(MMLEvent *p_event);                // p
	MMLEvent *_on_mml_fine_pan(MMLEvent *p_event);           // @p
	MMLEvent *_on_mml_input(MMLEvent *p_event);              // @i
	MMLEvent *_on_mml_output(MMLEvent *p_event);             // @o
	MMLEvent *_on_mml_ring_modulation(MMLEvent *p_event);    // @r

	// Sound channel events.

	MMLEvent *_on_mml_module_type(MMLEvent *p_event);             // %
	MMLEvent *_on_mml_event_trigger(MMLEvent *p_event);           // %t
	MMLEvent *_on_mml_dispatch_event(MMLEvent *p_event);          // %e
	MMLEvent *_on_mml_clock(MMLEvent *p_event);                   // @clock
	MMLEvent *_on_mml_algorithm(MMLEvent *p_event);               // @al
	MMLEvent *_on_mml_operator_parameter(MMLEvent *p_event);      // @
	MMLEvent *_on_mml_feedback(MMLEvent *p_event);                // @fb
	MMLEvent *_on_mml_slot_index(MMLEvent *p_event);              // i
	MMLEvent *_on_mml_operator_release_rate(MMLEvent *p_event);   // @rr
	MMLEvent *_on_mml_operator_total_level(MMLEvent *p_event);    // @tl
	MMLEvent *_on_mml_operator_multiple(MMLEvent *p_event);       // @ml
	MMLEvent *_on_mml_operator_detune(MMLEvent *p_event);         // @dt
	MMLEvent *_on_mml_operator_phase(MMLEvent *p_event);          // @ph
	MMLEvent *_on_mml_operator_fixed_note(MMLEvent *p_event);     // @fx
	MMLEvent *_on_mml_operator_ssg_envelope(MMLEvent *p_event);   // @se
	MMLEvent *_on_mml_operator_envelope_reset(MMLEvent *p_event); // @er
	MMLEvent *_on_mml_sustain(MMLEvent *p_event);                 // s
	MMLEvent *_on_mml_register_update(MMLEvent *p_event);         // register

	// External callbacks.

	Callable _callback_event_note_on;
	Callable _callback_event_note_off;
	Callable _callback_tempo_changed;
	Callable _callback_timer;
	Callable _callback_beat;
	// The function signature is bool (SiMMLData *, const Variant &). Return false to append the command to SiONData.system_commands.
	Callable _callback_parse_system_command;

	//

	int _envelope_event_id = 0;

	void _register_process_events();
	void _register_dummy_process_events();

	void _register_event_listeners();
	void _reset_initial_operator_params();
	void _reset_parser_settings();

protected:
	static void _bind_methods();

public:
	String get_title() const { return _title; }

	double get_effective_bpm() const;
	void set_effective_bpm(double p_value);

	// Tracks.

	int get_max_track_count() const { return _max_track_count; }
	void set_max_track_count(int p_value) { _max_track_count = p_value; }

	Vector<SiMMLTrack *> get_tracks() const { return _tracks; }
	SiMMLTrack *get_current_track() const { return _current_track; }
	void reset_all_tracks();

	SiMMLTrack *find_active_track(int p_internal_track_id, int p_delay = -1);
	SiMMLTrack *create_controllable_track(int p_internal_track_id = 0, bool p_disposable = true);

	bool is_ready_to_process() const;
	int get_processed_sample_count() const { return _processed_sample_count; }

	bool is_finished() const;
	bool is_sequence_finished() const { return _is_sequence_finished; }
	void stop_sequence();

	// Compilation and processing.

	bool is_dummy_process() const { return _dummy_process; }
	void process_dummy(int p_sample_count);

	virtual bool prepare_compile(MMLData *p_data, String p_mml) override;
	virtual void prepare_process(MMLData *p_data, int p_sample_rate, int p_buffer_length) override;
	virtual void process() override;

	// Current writing position in the streaming buffer, always less than length of the buffer.
	int get_stream_writing_residue() const { return _global_buffer_index; }

	// External callbacks.

	void set_note_on_callback(const Callable &p_func) { _callback_event_note_on = p_func; }
	void set_note_off_callback(const Callable &p_func) { _callback_event_note_off = p_func; }
	void set_tempo_changed_callback(const Callable &p_func) { _callback_tempo_changed = p_func; }
	void set_timer_callback(const Callable &p_func) { _callback_timer = p_func; }
	void set_beat_callback(const Callable &p_func) { _callback_beat = p_func; }
	void set_beat_callback_filter(int p_filter) { _on_beat_callback_filter = p_filter; }

	//

	SiMMLSequencer(SiOPMModule *p_module = nullptr);
	~SiMMLSequencer() {}
};

#endif // SIMML_SEQUENCER_H

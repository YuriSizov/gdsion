/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_sequencer.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/variant.hpp>
#include "sion_enums.h"
#include "processor/channels/siopm_channel_base.h"
#include "processor/siopm_channel_params.h"
#include "processor/siopm_module.h"
#include "processor/siopm_operator_params.h"
#include "processor/siopm_ref_table.h"
#include "processor/wave/siopm_wave_base.h"
#include "processor/wave/siopm_wave_pcm_table.h"
#include "sequencer/base/mml_executor.h"
#include "sequencer/base/mml_executor_connector.h"
#include "sequencer/base/mml_parser.h"
#include "sequencer/base/mml_parser_settings.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/base/mml_sequence_group.h"
#include "sequencer/base/mml_system_command.h"
#include "sequencer/simml_data.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_track.h"
#include "sequencer/simml_voice.h"
#include "utils/translator_util.h"

using namespace godot;

// Properties.

double SiMMLSequencer::get_effective_bpm() const {
	return is_ready_to_process() ? get_bpm() : get_default_bpm();
}

void SiMMLSequencer::set_effective_bpm(double p_value) {
	set_default_bpm(p_value);

	if (is_ready_to_process()) {
		ERR_FAIL_COND_MSG(_bpm_change_enabled, "SiMMLSequencer: Cannot change BPM while rendering (SiONTrackEvent::NOTE_*_STREAM).");
		set_bpm(p_value);
	}
}

// Tracks.

void SiMMLSequencer::_free_all_tracks() {
	for (SiMMLTrack *track : _tracks) {
		_free_tracks.push_back(track);
	}
	_tracks.clear();
}

void SiMMLSequencer::reset_all_tracks() {
	for (SiMMLTrack *track : _tracks) {
		track->reset(0);
		track->set_velocity(_parser_settings->default_volume);
		track->set_quantize_ratio((double)_parser_settings->default_quant_ratio / _parser_settings->max_quant_ratio);
		track->set_quantize_count(calculate_sample_count(_parser_settings->default_quant_count));
		track->get_channel()->set_master_volume(_parser_settings->default_fine_volume);
	}

	_processed_sample_count = 0;
	_is_sequence_finished = (_tracks.size() == 0);
}

void SiMMLSequencer::_initialize_track(SiMMLTrack *p_track, int p_internal_track_id, bool p_disposable) {
	p_track->initialize(nullptr, 60, (p_internal_track_id >= 0 ? p_internal_track_id : 0), _callback_event_note_on, _callback_event_note_off, p_disposable);
	p_track->reset(_global_buffer_index);
	p_track->get_channel()->set_master_volume(_parser_settings->default_fine_volume);
}

SiMMLTrack *SiMMLSequencer::_find_lowest_priority_track() {
	int index = -1;
	int max_priority = 0;

	for (int i = _tracks.size() - 1; i >= 0; i--) {
		SiMMLTrack *track = _tracks[i];
		if (track->get_priority() >= max_priority) {
			index = i;
			max_priority = track->get_priority();
		}
	}

	if (max_priority == 0) {
		return nullptr;
	}
	return _tracks[index];
}

SiMMLTrack *SiMMLSequencer::find_active_track(int p_internal_track_id, int p_delay) {
	for (SiMMLTrack *track : _tracks) {
		if (track->get_internal_track_id() != p_internal_track_id || !track->is_active()) {
			continue;
		}

		if (p_delay == -1) {
			return track;
		}

		int diff = track->get_track_start_delay() - p_delay;
		if (diff > -8 && diff < 8) {
			return track;
		}
	}

	return nullptr;
}

SiMMLTrack *SiMMLSequencer::create_controllable_track(int p_internal_track_id, bool p_disposable) {
	for (int i = _tracks.size() - 1; i >= 0; i--) {
		SiMMLTrack *track = _tracks[i];
		if (!track->is_active()) {
			_initialize_track(track, p_internal_track_id, p_disposable);
			return track;
		}
	}

	SiMMLTrack *track = nullptr;
	if (_tracks.size() < _max_track_count) {
		if (!_free_tracks.is_empty()) {
			track = _free_tracks.back()->get();
			_free_tracks.pop_back();
		} else {
			track = memnew(SiMMLTrack);
		}

		track->set_track_number(_tracks.size());
		_tracks.push_back(track);
	} else {
		track = _find_lowest_priority_track();
		if (!track) {
			return nullptr;
		}
	}

	_initialize_track(track, p_internal_track_id, p_disposable);
	return track;
}

bool SiMMLSequencer::is_ready_to_process() const {
	return _tracks.size() > 0;
}

bool SiMMLSequencer::is_finished() const {
	if (!_is_sequence_finished) {
		return false;
	}

	for (SiMMLTrack *track : _tracks) {
		if (!track->is_finished()) {
			return false;
		}
	}

	return true;
}

void SiMMLSequencer::stop_sequence() {
	_is_sequence_finished = true;
}

// Compilation and processing.

String SiMMLSequencer::_on_before_compile(String p_mml) {
	_reset_parser_parameters();

	String mml = p_mml + "\n";

	// Remove comments.

	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "gms". Behavioral implications require investigation.
	Ref<RegEx> re_comments = RegEx::create_from_string("/\\*.*?\\*/|//.*?[\\r\\n]+");
	mml = re_comments->sub(mml, "", true);

	// Format last.

	char last_char;
	int i = mml.length();
	do {
		if (i == 0) {
			return "";
		}

		i--;
		last_char = mml[i];
	} while (last_char == ' ' || last_char == '\t' || last_char == '\r' || last_char == '\n');

	mml = mml.substr(0, i + 1);
	if (last_char != ';') {
		mml += ';';
	}

	// Expand macros.

	String expanded_mml;

	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "gms". Behavioral implications require investigation.
	Ref<RegEx> re_sequence = RegEx::create_from_string("[ \\t\\r\\n]*(#([A-Z@\\-]+)(\\+=|=)?)?([^;{]*({.*?})?[^;]*);");
	Ref<RegEx> re_macro_id = RegEx::create_from_string("([A-Z])?(-([A-Z])?)?");

	TypedArray<RegExMatch> matches = re_sequence->search_all(mml);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> res = matches[i];

		// Normal sequence.
		if (res->get_string(1).is_empty()) {
			expanded_mml += _expand_macro(res->get_string(4)) + ";";
			continue;
		}

		// System command.
		if (res->get_string(3).is_empty()) {
			if (res->get_string(2) == "END") {
				break; // The #END command.
			}
			if (!_parse_system_command_before(res->get_string(1), res->get_string(4))) {
				// If parsing returned false, we'll try it again after compiling MML.
				expanded_mml += res->get_string(0);
			}
			continue;
		}

		// Macro definition.

		String macro_id = res->get_string(2);
		bool concat = (res->get_string(3) == "+=");

		// Parse macro IDs.
		TypedArray<RegExMatch> mid_matches = re_macro_id->search_all(macro_id);
		for (int j = 0; j < mid_matches.size(); j++) {
			Ref<RegExMatch> mid_res = mid_matches[j];

			int start_id = 0;
			if (!mid_res->get_string(1).is_empty()) {
				start_id = mid_res->get_string(1).unicode_at(0) - 'A';
			}

			int end_id = start_id;
			if (!mid_res->get_string(2).is_empty()) {
				if (!mid_res->get_string(3).is_empty()) {
					end_id = mid_res->get_string(3).unicode_at(0) - 'A';
				} else {
					end_id = MACRO_SIZE - 1;
				}
			}

			for (int k = start_id; k < end_id; k++) {
				String value = (_macro_expand_dynamic ? res->get_string(4) : _expand_macro(res->get_string(4)));

				if (concat) {
					_macro_strings.write[k] += value;
				} else {
					_macro_strings.write[k] = value;
				}
			}
		}
	}

	// Expand repeat.

	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "gms". Behavioral implications require investigation.
	Ref<RegEx> re_repeat = RegEx::create_from_string("!\\[(\\d*)(.*?)(!\\|(.*?))?!\\](\\d*)");
	matches = re_repeat->search_all(expanded_mml);
	// Iterate backwards so we can do in-place replacements without disturbing indices.
	for (int i = matches.size() - 1; i >= 0; i--) {
		Ref<RegExMatch> res = matches[i];

		int repeat_count = 1;
		if (!res->get_string(1).is_empty()) {
			repeat_count = res->get_string(1).to_int() - 1;
		} else if (!res->get_string(5).is_empty()) {
			repeat_count = res->get_string(5).to_int() - 1;
		}

		if (repeat_count > 256) {
			repeat_count = 256;
		}

		String rep = res->get_string(2);
		if (!res->get_string(3).is_empty()) {
			rep += res->get_string(4);
		}

		String replacement = rep.repeat(repeat_count) + res->get_string(2);

		// Take the rest of the string (around the match) and insert the replaced substring.
		expanded_mml = expanded_mml.substr(0, res->get_start()) + replacement + expanded_mml.substr(res->get_end() + 1);
	}

	return expanded_mml;
}

void SiMMLSequencer::_on_after_compile(MMLSequenceGroup *p_group) {
	MMLSequence *sequence = p_group->get_head_sequence();
	while (sequence) {
		if (sequence->is_system_command()) {
			sequence = _parse_system_command_after(p_group, sequence);
		} else {
			sequence = sequence->get_next_sequence();
		}
	}
}

void SiMMLSequencer::_on_process(int p_length, MMLEvent *p_event) {
	_current_track->buffer(p_length);
}

void SiMMLSequencer::_on_timer_interruption() {
	if (_dummy_process) {
		return;
	}

	if (_callback_timer.is_valid()) {
		_callback_timer.call();
	}
}

void SiMMLSequencer::_on_beat(int p_delay_samples, int p_beat_counter) {
	if (_dummy_process) {
		return;
	}

	if (_callback_beat.is_valid()) {
		_callback_beat.call(p_delay_samples, p_beat_counter);
	}
}

void SiMMLSequencer::_on_table_parse(MMLEvent *p_prev, String p_table) {
	ERR_FAIL_COND_MSG(p_prev->id < _envelope_event_id || p_prev->id > _envelope_event_id + 10, "SiMMLSequencer : Internal table is available only for envelope commands.");

	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "ms". Behavioral implications require investigation.
	Ref<RegEx> re_table = RegEx::create_from_string("\\{([^}]*)\\}(.*)");

	Ref<RegExMatch> res = re_table->search(p_table);
	ERR_FAIL_COND_MSG(!res.is_valid(), "SiMMLSequencer: Invalid table format.");

	String data = res->get_string(1);
	String postfix = res->get_string(2);

	SiMMLEnvelopeTable *env_table = memnew(SiMMLEnvelopeTable);
	env_table->parse_mml(data, postfix);
	ERR_FAIL_COND_MSG(!env_table->head, vformat("SiMMLSequencer: Invalid table parameter '%s' in the {..} command.", data));

	Ref<SiMMLData> simml_data = mml_data;
	simml_data->set_envelope_table(_internal_table_index, env_table);

	p_prev->data = _internal_table_index;
	_internal_table_index--;
}

void SiMMLSequencer::_on_tempo_changed(double p_tempo_ratio) {
	for (SiMMLTrack *track : _tracks) {
		if (track->get_bpm_settings().is_null()) {
			track->get_executor()->on_tempo_changed(p_tempo_ratio);
		}
	}

	if (_callback_tempo_changed.is_valid()) {
		_callback_tempo_changed.call(_global_buffer_index, _dummy_process);
	}
}

void SiMMLSequencer::process_dummy(int p_sample_count) {
	int buffer_count = p_sample_count / _module->get_buffer_length();
	if (buffer_count == 0) {
		return;
	}

	// Temporary enable dummy mode and register events.
	_dummy_process = true;
	_register_dummy_process_events();

	// Process things.
	for (int i = 0; i < buffer_count; i++) {
		process();
	}

	// Set everything back to normal.
	_dummy_process = false;
	_register_process_events();
}

bool SiMMLSequencer::prepare_compile(const Ref<MMLData> &p_data, String p_mml) {
	_free_all_tracks();
	return MMLSequencer::prepare_compile(p_data, p_mml);
}

void SiMMLSequencer::prepare_process(const Ref<MMLData> &p_data, int p_sample_rate, int p_buffer_length) {
	_free_all_tracks();
	_processed_sample_count = 0;
	_bpm_change_enabled = true;

	MMLSequencer::prepare_process(p_data, p_sample_rate, p_buffer_length);

	if (mml_data.is_valid()) {
		MMLSequence *sequence = mml_data->get_sequence_group()->get_head_sequence();
		int index = 0;

		while (sequence) {
			if (sequence->is_active()) {
				SiMMLTrack *track = nullptr;
				if (!_free_tracks.is_empty()) {
					track = _free_tracks.back()->get();
					_free_tracks.pop_back();
				} else {
					track = memnew(SiMMLTrack);
				}

				int internal_track_id = index | SiMMLTrack::MML_TRACK;
				track->initialize(sequence, mml_data->get_default_fps(), internal_track_id, _callback_event_note_on, _callback_event_note_off, true);
				track->set_track_number(index);
				_tracks.push_back(track);

				index++;
			}

			sequence = sequence->get_next_sequence();
		}
	}

	reset_all_tracks();
}

void SiMMLSequencer::process() {
	// Prepare for buffering.
	for (SiMMLTrack *track : _tracks) {
		track->get_channel()->reset_channel_buffer_status();
	}

	// Buffering.

	bool finished = true;
	start_global_sequence();

	do {
		int buffering_length = execute_global_sequence();
		_bpm_change_enabled = false;

		for (SiMMLTrack *track : _tracks) {
			_current_track = track;
			int length = track->prepare_buffer(buffering_length);
			_bpm = track->get_bpm_settings();
			if (_bpm.is_null()) {
				_bpm = _adjustible_bpm;
			}

			finished = process_executor(track->get_executor(), length) && finished;
		}

		_bpm_change_enabled = true;
	} while (!check_global_sequence_end());

	_bpm = _adjustible_bpm;
	_current_track = nullptr;
	_processed_sample_count += _module->get_buffer_length();

	_is_sequence_finished = finished;
}

// Parser.

String SiMMLSequencer::_expand_macro(String p_macro, bool p_nested) {
	// Note that the original code has a broken circular call check. It never updates the flag
	// storage, and the check is written incorrectly too. We attempt to fix it here based on the
	// intention of the original code rather than the actual implementation.

	static uint32_t flag_macro_expanded = 0;

	if (p_macro.is_empty()) {
		return "";
	}

	if (!p_nested) {
		// This is a top-level call, so we reset the flag storage.
		// We expect only one chain of _expand_macro() calls to be executed at the same time.
		flag_macro_expanded = 0;
	}

	String expanded_macro = p_macro;

	Ref<RegEx> re_macro = RegEx::create_from_string("([A-Z])(\\(([\\-\\d]+)\\))?");
	TypedArray<RegExMatch> matches = re_macro->search_all(expanded_macro);
	// Iterate backwards so we can do in-place replacements without disturbing indices.
	for (int i = matches.size() - 1; i >= 0; i--) {
		Ref<RegExMatch> res = matches[i];

		int index = res->get_string(1).unicode_at(0) - 'A';

		// Check for circular calls.
		int flag = 1 << index;
		ERR_FAIL_COND_V_MSG(flag_macro_expanded & flag, p_macro, vformat("SiMMLSequencer: Failed to expand a macro due to a circular reference, '%s'.", res->get_string()));
		flag_macro_expanded |= flag;

		String replacement;
		if (!_macro_strings[i].is_empty()) {
			if (!res->get_string(2).is_empty()) {
				int t = 0;
				if (!res->get_string(3).is_empty()) {
					t = res->get_string(3).to_int();
				}

				replacement = "!@ns" + itos(t) + (_macro_expand_dynamic ? _expand_macro(_macro_strings[i], true) : _macro_strings[i]) + "!@ns" + itos(-t);
			} else {
				replacement = (_macro_expand_dynamic ? _expand_macro(_macro_strings[i], true) : _macro_strings[i]);
			}
		}

		// Take the rest of the string (around the match) and insert the replaced substring.
		expanded_macro = expanded_macro.substr(0, res->get_start()) + replacement + expanded_macro.substr(res->get_end() + 1);
	}

	return expanded_macro;
}

void SiMMLSequencer::_reset_parser_parameters() {
	_internal_table_index = 511;
	_title = "";

	_parser_settings->octave_polarization = 1;
	_parser_settings->volume_polarization = 1;
	_parser_settings->default_quant_ratio = 6;
	_parser_settings->max_quant_ratio     = 8;

	_macro_expand_dynamic = false;
	MMLParser::get_instance()->set_key_signature("C");

	for (int i = 0; i < _macro_strings.size(); i++) {
		_macro_strings.write[i] = "";
	}
}

void SiMMLSequencer::_parse_command_init_sequence(SiOPMChannelParams *p_params, String p_postfix) {
	MMLSequence *sequence = p_params->get_init_sequence();

	MMLParser::get_instance()->prepare_parse(_parser_settings, p_postfix);
	MMLEvent *event = MMLParser::get_instance()->parse();
	if (!event || !event->next) {
		return;
	}
	sequence->cutout(event);

	MMLEvent *prev = sequence->get_head_event();
	while (prev->next) {
		MMLEvent *next = prev->next;
		ERR_FAIL_COND_MSG(next->length != 0, vformat("SiMMLSequencer: Initializing sequence cannot contain processing events, '%s'.", p_postfix));
		ERR_FAIL_COND_MSG(next->id == MMLEvent::MOD_TYPE || next->id == MMLEvent::MOD_PARAM, vformat("SiMMLSequencer: Initializing sequence cannot contain '%%' or '@', '%s'.", p_postfix));

		if (next->id == MMLEvent::TABLE_EVENT) {
			// Parse table events and keep the pointer.
			parse_table_event(prev);
		} else {
			// Move to the next event in every other case.
			prev = next;
		}
	}
}

void SiMMLSequencer::_parse_tmode_command(String p_mml) {
	Ref<RegEx> re_tcommand = RegEx::create_from_string("(unit|timerb|fps)=?([\\d.]*)");
	Ref<RegExMatch> res = re_tcommand->search(p_mml);

	String value_string = res->get_string(2);
	double value = value_string.is_valid_float() ? value_string.to_float() : 0;

	if (res->get_string(1) == "unit") {
		mml_data->set_tcommand_mode(MMLData::TCOMMAND_BPM);
		mml_data->set_tcommand_resolution(value > 0 ? (1.0 / value) : 1.0);

	} else if (res->get_string(1) == "timerb") {
		mml_data->set_tcommand_mode(MMLData::TCOMMAND_TIMERB);
		mml_data->set_tcommand_resolution((value > 0 ? value : 4000) * 1.220703125);

	} else if (res->get_string(1) == "fps") {
		mml_data->set_tcommand_mode(MMLData::TCOMMAND_FRAME);
		mml_data->set_tcommand_resolution(value > 0 ? value * 60 : 3600);
	}
}

void SiMMLSequencer::_parse_vmode_command(String p_mml) {
	Ref<RegEx> re_vcommand = RegEx::create_from_string("(n88|mdx|psg|mck|tss|%[xv])(\\d*)(\\s*,?\\s*(\\d?))");
	TypedArray<RegExMatch> matches = re_vcommand->search_all(p_mml);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> res = matches[i];

		if (res->get_string(1) == "%v") {
			int mode = res->get_string(2).to_int();
			mml_data->set_default_velocity_mode((mode >= 0 && mode < SiOPMRefTable::VM_MAX) ? mode : 0);

			int shift = 4;
			if (!res->get_string(4).is_empty()) {
				shift = res->get_string(4).to_int();
			}
			mml_data->set_default_velocity_shift((shift >= 0 && shift < 8) ? shift : 0);

		} else if (res->get_string(1) == "%x") {
			int mode = res->get_string(2).to_int();
			mml_data->set_default_expression_mode((mode >= 0 && mode < SiOPMRefTable::VM_MAX) ? mode : 0);

		} else if (res->get_string(1) == "n88" || res->get_string(1) == "mdx") {
			mml_data->set_default_velocity_mode(SiOPMRefTable::VM_DR32DB);
			mml_data->set_default_expression_mode(SiOPMRefTable::VM_DR48DB);

		} else if (res->get_string(1) == "psg") {
			mml_data->set_default_velocity_mode(SiOPMRefTable::VM_DR48DB);
			mml_data->set_default_expression_mode(SiOPMRefTable::VM_DR48DB);

		} else { // mck/tss
			mml_data->set_default_velocity_mode(SiOPMRefTable::VM_LINEAR);
			mml_data->set_default_expression_mode(SiOPMRefTable::VM_LINEAR);
		}
	}
}

bool SiMMLSequencer::_try_set_sampler_wave(int p_index, String p_mml) {
	if (SiOPMRefTable::get_instance()->sound_reference.is_empty()) {
		return false;
	}

	int bank = (p_index >> SiOPMRefTable::NOTE_BITS) & (SiOPMRefTable::SAMPLER_TABLE_MAX - 1);
	int index = p_index & (SiOPMRefTable::NOTE_TABLE_SIZE - 1);

	Ref<SiMMLData> simml_data = mml_data;
	SiOPMWaveSamplerTable *table = simml_data->get_sampler_table(bank);
	return TranslatorUtil::parse_sampler_wave(table, index, p_mml, SiOPMRefTable::get_instance()->sound_reference);
}

bool SiMMLSequencer::_try_set_pcm_wave(int p_index, String p_mml) {
	if (SiOPMRefTable::get_instance()->sound_reference.is_empty()) {
		return false;
	}

	Ref<SiMMLData> simml_data = mml_data;
	Ref<SiMMLVoice> voice = simml_data->get_pcm_voice(p_index);
	SiOPMWavePCMTable *table = Object::cast_to<SiOPMWavePCMTable>(voice->get_wave_data());
	if (!table) {
		return false;
	}

	return TranslatorUtil::parse_pcm_wave(table, p_mml, SiOPMRefTable::get_instance()->sound_reference);
}

bool SiMMLSequencer::_try_set_pcm_voice(int p_index, String p_mml, String p_postfix) {
	if (SiOPMRefTable::get_instance()->sound_reference.is_empty()) {
		return false;
	}

	Ref<SiMMLData> simml_data = mml_data;
	Ref<SiMMLVoice> voice = simml_data->get_pcm_voice(p_index);
	if (voice.is_null()) {
		return false;
	}

	return TranslatorUtil::parse_pcm_voice(voice, p_mml, p_postfix, simml_data->get_envelope_tables());
}

void SiMMLSequencer::_try_process_command_callback(String p_command, int p_number, String p_content, String p_postfix) {
	Ref<MMLSystemCommand> command_obj;
	command_obj.instantiate();
	command_obj->command = p_command;
	command_obj->number = p_number;
	command_obj->content = p_content;
	command_obj->postfix = p_postfix;

	if (_callback_parse_system_command.is_valid()) {
		Ref<SiMMLData> simml_data = mml_data;
		bool parsed = _callback_parse_system_command.call(simml_data, command_obj);
		if (parsed) {
			return;
		}
	}

	// Wasn't parsed, add it to the list.
	mml_data->get_system_commands().push_back(command_obj);
}

bool SiMMLSequencer::_parse_system_command_before(String p_command, String p_param) {
	// FIXME: Godot's RegEx implementation doesn't support passing global flags. This pattern originally used "ms". Behavioral implications require investigation.
	Ref<RegEx> re_param = RegEx::create_from_string("\\s*(\\d*)\\s*(\\{(.*?)\\})?(.*)");
	Ref<RegExMatch> res = re_param->search(p_param);

	int number = res->get_string(1).to_int();
	bool has_content = (!res->get_string(2).is_empty());
	String content = res->get_string(3);
	String postfix = res->get_string(4);

	// Tone settings.

#define PARSE_TONE_PARAMS(m_func)                                         \
	Ref<SiMMLData> simml_data = mml_data;                                 \
	SiOPMChannelParams *params = simml_data->get_channel_params(number);  \
	m_func(params, content);                                              \
	if (!postfix.is_empty()) {                                            \
		_parse_command_init_sequence(params, postfix);                    \
	}

	if (p_command == "#@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_params);
		return true;
	}
	if (p_command == "#OPM@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_opm_params);
		return true;
	}
	if (p_command == "#OPN@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_opn_params);
		return true;
	}
	if (p_command == "#OPL@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_opl_params);
		return true;
	}
	if (p_command == "#OPX@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_opx_params);
		return true;
	}
	if (p_command == "#MA@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_ma3_params);
		return true;
	}
	if (p_command == "#AL@") {
		PARSE_TONE_PARAMS(TranslatorUtil::parse_al_params);
		return true;
	}

#undef PARSE_TONE_PARAMS

	// Parser settings.

	if (p_command == "#TITLE") {
		mml_data->set_title(has_content ? content : postfix);
		return true;
	}
	if (p_command == "#FPS") {
		mml_data->set_default_fps(number > 0 ? number : (has_content ? content.to_int() : 60));
		return true;
	}
	if (p_command == "#SIGN") {
		MMLParser::get_instance()->set_key_signature(has_content ? content : postfix);
		return true;
	}
	if (p_command == "#MACRO") {
		String data = (has_content ? content : postfix);
		if (data == "dynamic") {
			_macro_expand_dynamic = true;
		} else if (data == "static") {
			_macro_expand_dynamic = false;
		} else {
			ERR_FAIL_V_MSG(true, vformat("SiMMLSequencer: Invalid parameter '%s' for command '%s'.", data, p_command));
		}
		return true;
	}
	if (p_command == "#QUANT") {
		if (number > 0) {
			_parser_settings->max_quant_ratio = number;
			_parser_settings->default_quant_ratio = (int)(number * 0.75);
		}
		return true;
	}
	if (p_command == "#TMODE") {
		_parse_tmode_command(content);
		return true;
	}
	if (p_command == "#VMODE") {
		_parse_vmode_command(content);
		return true;
	}
	if (p_command == "#REV") { // Reverse
		String data = (has_content ? content : postfix);
		if (data == "") {
			_parser_settings->octave_polarization = -1;
			_parser_settings->volume_polarization = -1;
		} else if (data == "octave") {
			_parser_settings->octave_polarization = -1;
		} else if (data == "volume") {
			_parser_settings->volume_polarization = -1;
		} else {
			ERR_FAIL_V_MSG(true, vformat("SiMMLSequencer: Invalid parameter '%s' for command '%s'.", data, p_command));
		}
		return true;
	}

	// Tables.

	if (p_command == "#TABLE") {
		ERR_FAIL_COND_V_MSG((number < 0 || number > 254), true, vformat("SiMMLSequencer: Parameter '%d' for command '%s' is outside of valid range (%d : %d).", number, p_command, 0, 254));

		SiMMLEnvelopeTable *env_table = memnew(SiMMLEnvelopeTable);
		env_table->parse_mml(content, postfix);
		ERR_FAIL_COND_V_MSG(!env_table->head, true, vformat("SiMMLSequencer: Invalid parameter '%s' for command '%s'.", content, p_command));

		Ref<SiMMLData> simml_data = mml_data;
		simml_data->set_envelope_table(number, env_table);
		return true;
	}
	if (p_command == "#WAV") {
		ERR_FAIL_COND_V_MSG((number < 0 || number > 255), true, vformat("SiMMLSequencer: Parameter '%d' for command '%s' is outside of valid range (%d : %d).", number, p_command, 0, 255));

		Ref<SiMMLData> simml_data = mml_data;
		simml_data->set_wave_table(number, TranslatorUtil::parse_wav(content, postfix));
		return true;
	}
	if (p_command == "#WAVB") {
		ERR_FAIL_COND_V_MSG((number < 0 || number > 255), true, vformat("SiMMLSequencer: Parameter '%d' for command '%s' is outside of valid range (%d : %d).", number, p_command, 0, 255));

		Ref<SiMMLData> simml_data = mml_data;
		simml_data->set_wave_table(number, TranslatorUtil::parse_wavb(has_content ? content : postfix));
		return true;
	}

	// PCM voices.

	if (p_command == "#SAMPLER") {
		ERR_FAIL_COND_V_MSG((number < 0 || number > 255), true, vformat("SiMMLSequencer: Parameter '%d' for command '%s' is outside of valid range (%d : %d).", number, p_command, 0, 255));

		if (!_try_set_sampler_wave(number, content)) {
			_try_process_command_callback(p_command, number, content, postfix);
		}
		return true;
	}
	if (p_command == "#PCMWAVE") {
		ERR_FAIL_COND_V_MSG((number < 0 || number > 255), true, vformat("SiMMLSequencer: Parameter '%d' for command '%s' is outside of valid range (%d : %d).", number, p_command, 0, 255));

		if (!_try_set_pcm_wave(number, content)) {
			_try_process_command_callback(p_command, number, content, postfix);
		}
		return true;
	}
	if (p_command == "#PCMVOICE") {
		ERR_FAIL_COND_V_MSG((number < 0 || number > 255), true, vformat("SiMMLSequencer: Parameter '%d' for command '%s' is outside of valid range (%d : %d).", number, p_command, 0, 255));

		if (!_try_set_pcm_voice(number, content, postfix)) {
			_try_process_command_callback(p_command, number, content, postfix);
		}
		return true;
	}

	// Commands to be handled after parsing.
	if (p_command == "#FM") {
		return false;
	}

	// Known but unsupported commands.
	if (p_command == "#WAVEXP" || p_command == "#WAVEXP" || p_command == "#WAVEXP") {
		WARN_PRINT(vformat("SiMMLSequencer: Command '%s' is not supported at this time.", p_command));
		return true;
	}

	// User defined commands, probably.
	_try_process_command_callback(p_command, number, content, postfix);
	return true;
}

MMLSequence *SiMMLSequencer::_parse_system_command_after(MMLSequenceGroup *p_seq_group, MMLSequence *p_command_seq) {
	String command = p_command_seq->get_system_command();

	Ref<RegEx> re_command = RegEx::create_from_string("#(FM)[{ \\t\\r\\n]*([^}]*)");
	Ref<RegExMatch> res = re_command->search(command);

	// Remove it from the chain to skip.
	MMLSequence *sequence = p_command_seq->remove_from_chain();

	// Parse the command.
	if (res.is_valid()) {
		ERR_FAIL_COND_V_MSG(res->get_string(1) != "FM", nullptr, vformat("SiMMLSequencer: Invalid system command letter, '%s'.", command));
		ERR_FAIL_COND_V_MSG(res->get_string(2).is_empty(), nullptr, vformat("SiMMLSequencer: Invalid system command syntax, '%s'.", command));

		_connector->parse(res->get_string(2));
		sequence = _connector->connect(p_seq_group, sequence);
	}

	return sequence->get_next_sequence();
}

// Internal callbacks.

// These macros help to clean up repeating code quite a bit, although they also hide
// bits of logic, which may not be so great for understanding. Still, it's like 50
// methods with a lot of the same boilerplate, so lesser of two evils, and all that.

#define GET_EV_PARAMS(m_count)                                           \
	Vector<int> ev_params;                                               \
	ev_params.resize_zeroed(MAX_PARAM_COUNT);                            \
	MMLEvent *next_event = p_event->get_parameters(&ev_params, m_count);

#define BIND_EV_PARAM(m_var, m_index, m_default)                                    \
	int m_var = (ev_params[m_index] != INT32_MIN ? ev_params[m_index] : m_default);

#define BIND_EV_PARAM_RANGE(m_var, m_index, m_min, m_max, m_default)                                          \
	int m_var = (ev_params[m_index] >= m_min && ev_params[m_index] < m_max) ? ev_params[m_index] : m_default;

/// Processing events.

MMLEvent *SiMMLSequencer::_on_mml_rest(MMLEvent *p_event) {
	_current_track->handle_rest_event();
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *SiMMLSequencer::_on_mml_note(MMLEvent *p_event) {
	_current_track->handle_note_event(p_event->data, calculate_sample_count(p_event->length));
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *SiMMLSequencer::_on_mml_driver_note_on(MMLEvent *p_event) {
	_current_track->set_note_immediately(p_event->data, calculate_sample_count(p_event->length));
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *SiMMLSequencer::_on_mml_slur(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_SLUR) {
		_current_track->change_note_length(calculate_sample_count(p_event->length));
	} else {
		_current_track->handle_slur();
	}
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *SiMMLSequencer::_on_mml_slur_weak(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_SLUR) {
		_current_track->change_note_length(calculate_sample_count(p_event->length));
	} else {
		_current_track->handle_slur_weak();
	}
	return _current_executor->publish_processing_event(p_event);
}

MMLEvent *SiMMLSequencer::_on_mml_pitch_bend(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_SLUR) {
		_current_track->change_note_length(calculate_sample_count(p_event->length));
	} else {
		if (!p_event->next || p_event->next->id != MMLEvent::NOTE) {
			return p_event->next; // Check the next note.
		}

		int term = calculate_sample_count(p_event->length);
		_current_track->handle_pitch_bend(p_event->next->data, term);
	}
	return _current_executor->publish_processing_event(p_event);
}

// Driver track events.

MMLEvent *SiMMLSequencer::_on_mml_quant_ratio(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_QUANTIZE) {
		return p_event->next; // Check the mask.
	}

	_current_track->set_quantize_ratio((double)p_event->data / _parser_settings->max_quant_ratio);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_quant_count(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(quant_count, 0, 0);
	BIND_EV_PARAM(key_delay, 1, 0);

	quant_count *= _parser_settings->resolution / _parser_settings->max_quant_count;
	key_delay *= _parser_settings->resolution / _parser_settings->max_quant_count;

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_QUANTIZE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_quantize_count(calculate_sample_count(quant_count));
	_current_track->set_key_on_delay(calculate_sample_count(key_delay));
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_event_mask(MMLEvent *p_event) {
	_current_track->set_event_mask(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_detune(MMLEvent *p_event) {
	_current_track->set_pitch_shift(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_key_transition(MMLEvent *p_event) {
	_current_track->set_note_shift(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_relative_detune(MMLEvent *p_event) {
	_current_track->set_pitch_shift(_current_track->get_pitch_shift() + (p_event->data == INT32_MIN ? 0 : p_event->data));
	return p_event->next;
}

// Envelope events.

MMLEvent *SiMMLSequencer::_on_mml_envelope_fps(MMLEvent *p_event) {
	int frame = (p_event->data == INT32_MIN || p_event->data == 0) ? 60 : p_event->data;
	if (frame > 1000) {
		frame = 1000;
	}

	_current_track->set_envelope_fps(frame);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_tone_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_tone_envelope(1, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_amplitude_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_amplitude_envelope(1, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_amplitude_envelope_tsscp(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_amplitude_envelope(1, _table->get_envelope_table(idx), step, true);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_pitch_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_pitch_envelope(1, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_note_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}
	_current_track->set_note_envelope(1, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_filter_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_filter_envelope(1, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_tone_release_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_tone_envelope(0, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_amplitude_release_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_amplitude_envelope(0, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_pitch_release_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_pitch_envelope(0, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_note_release_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_note_envelope(0, _table->get_envelope_table(idx), step);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_filter_release_envelope(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(idx, 0, 0, 255, -1);
	BIND_EV_PARAM(step, 1, 1);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_ENVELOPE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_filter_envelope(0, _table->get_envelope_table(idx), step);
	return next_event->next;
}

// Internal table envelope events.

MMLEvent *SiMMLSequencer::_on_mml_filter(MMLEvent *p_event) {
	GET_EV_PARAMS(10);
	BIND_EV_PARAM(cut, 0, 128);
	BIND_EV_PARAM(res, 1,   0);
	BIND_EV_PARAM(ar,  2,   0);
	BIND_EV_PARAM(dr1, 3,   0);
	BIND_EV_PARAM(dr2, 4,   0);
	BIND_EV_PARAM(rr,  5,   0);
	BIND_EV_PARAM(dc1, 6, 128);
	BIND_EV_PARAM(dc2, 7,  64);
	BIND_EV_PARAM(sc,  8,  32);
	BIND_EV_PARAM(rc,  9, 128);

	_current_track->get_channel()->set_sv_filter(cut, res, ar, dr1, dr2, rr, dc1, dc2, sc, rc);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_filter_mode(MMLEvent *p_event) {
	_current_track->get_channel()->set_filter_type(p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_lf_oscillator(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(cycle_time, 0, 20); // One third of a second.
	BIND_EV_PARAM(waveform, 1, SiOPMRefTable::LFO_WAVE_TRIANGLE);

	cycle_time *= 1000/60; // Convert to ms.

	if (waveform > 7 && waveform < 255) { // Custom table.
		SiMMLEnvelopeTable *ev_table = _table->get_envelope_table(ev_params[1]);
		if (ev_table) {
			Vector<int> table_vector;
			ev_table->to_vector(256, &table_vector, 0, 255);
			_current_track->get_channel()->initialize_lfo(-1, table_vector);
		} else {
			_current_track->get_channel()->initialize_lfo(SiOPMRefTable::LFO_WAVE_TRIANGLE);
		}
	} else {
		_current_track->get_channel()->initialize_lfo(waveform);
	}

	_current_track->get_channel()->set_lfo_cycle_time(cycle_time);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_pitch_modulation(MMLEvent *p_event) {
	GET_EV_PARAMS(4);
	BIND_EV_PARAM(depth,     0, 0);
	BIND_EV_PARAM(end_depth, 1, 0);
	BIND_EV_PARAM(delay,     2, 0);
	BIND_EV_PARAM(term,      3, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_MODULATE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_modulation_envelope(true, depth, end_depth, delay, term);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_amplitude_modulation(MMLEvent *p_event) {
	GET_EV_PARAMS(4);
	BIND_EV_PARAM(depth,     0, 0);
	BIND_EV_PARAM(end_depth, 1, 0);
	BIND_EV_PARAM(delay,     2, 0);
	BIND_EV_PARAM(term,      3, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_MODULATE) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_modulation_envelope(false, depth, end_depth, delay, term);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_portament(MMLEvent *p_event) {
	int frame = (p_event->data == INT32_MIN ? 0 : p_event->data);

	_current_track->set_portament(frame);
	return p_event->next;
}

// IO events.

MMLEvent *SiMMLSequencer::_on_mml_volume(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_VOLUME) {
		return p_event->next; // Check the mask.
	}

	_current_track->handle_velocity(p_event->data); // velocity (data << 3 = 16->128)
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_volume_shift(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_VOLUME) {
		return p_event->next; // Check the mask.
	}

	_current_track->handle_velocity_shift(p_event->data); // velocity (data << 3 = 16->128)
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_volume_setting(MMLEvent *p_event) {
	GET_EV_PARAMS(SiOPMModule::STREAM_SEND_SIZE);
	BIND_EV_PARAM(velocity_mode,  0, 0);
	BIND_EV_PARAM(velocity_shift, 1, 4);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_VOLUME) {
		return next_event->next; // Check the mask.
	}

	_current_track->set_velocity_mode(velocity_mode);
	_current_track->set_velocity_shift(velocity_shift);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_expression(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_VOLUME) {
		return p_event->next; // Check the mask.
	}

	_current_track->set_expression(p_event->data == INT32_MIN ? 128 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_expression_setting(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_VOLUME) {
		return p_event->next; // Check the mask.
	}

	_current_track->set_expression_mode(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_master_volume(MMLEvent *p_event) {
	GET_EV_PARAMS(SiOPMModule::STREAM_SEND_SIZE);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_VOLUME) {
		return next_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_all_stream_send_levels(ev_params);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_pan(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_PAN) {
		return p_event->next; // Check the mask.
	}

	int pan = p_event->data == INT32_MIN ? 0 : ((p_event->data << 4) - 64);

	_current_track->get_channel()->set_pan(pan);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_fine_pan(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_PAN) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_pan(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_input(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(level, 0, 5);
	BIND_EV_PARAM(index, 1, 0);

	_current_track->get_channel()->set_input(level, index);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_output(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(mode, 0, 2);
	BIND_EV_PARAM(index, 1, 0);

	_current_track->get_channel()->set_output((SiOPMChannelBase::OutputMode)mode, index);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_ring_modulation(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(level, 0, 4);
	BIND_EV_PARAM(index, 1, 0);

	_current_track->get_channel()->set_ring_modulation(level, index);
	return next_event->next;
}

// Sound channel events.

MMLEvent *SiMMLSequencer::_on_mml_module_type(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM_RANGE(type, 0, 0, MT_MAX, MT_ALL);
	BIND_EV_PARAM(channel_num, 1, INT32_MIN);

	_current_track->set_channel_module_type((SiONModuleType)type, channel_num);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_event_trigger(MMLEvent *p_event) {
	GET_EV_PARAMS(3);
	BIND_EV_PARAM(id,       0, 0);
	BIND_EV_PARAM(type_on,  1, 1);
	BIND_EV_PARAM(type_off, 2, 1);

	_current_track->set_event_trigger_callbacks(id, (SiMMLTrack::EventTriggerType)type_on, (SiMMLTrack::EventTriggerType)type_off);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_dispatch_event(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(id,      0, 0);
	BIND_EV_PARAM(type_on, 1, 1);

	_current_track->trigger_note_on_event(id, (SiMMLTrack::EventTriggerType)type_on);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_clock(MMLEvent *p_event) {
	_current_track->get_channel()->set_frequency_ratio(p_event->data == INT32_MIN ? 100 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_algorithm(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(op_count, 0, 0);
	BIND_EV_PARAM(algorithm, 1, _table->algorithm_init[op_count]);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_algorithm(op_count, algorithm);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_parameter(MMLEvent *p_event) {
	GET_EV_PARAMS(MAX_PARAM_COUNT);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	MMLSequence *sequence = _current_track->set_channel_parameters(ev_params);
	if (sequence) {
		sequence->connect_before(next_event->next);
		return sequence->get_head_event()->next;
	}

	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_feedback(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(level, 0, 0);
	BIND_EV_PARAM(connection, 1, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_feedback(level, connection);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_slot_index(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_active_operator_index(p_event->data == INT32_MIN ? 4 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_release_rate(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(release_rate, 0, INT32_MIN);
	BIND_EV_PARAM(release_sweep, 1, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	if (release_rate != INT32_MIN) {
		_current_track->get_channel()->set_release_rate(release_rate);
	}

	_current_track->set_release_sweep(release_sweep);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_total_level(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_total_level(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_multiple(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(value_base, 0, 0);
	BIND_EV_PARAM(value_offset, 1, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_fine_multiple((value_base << 7) + value_offset);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_detune(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_detune(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_phase(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_phase(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_fixed_note(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(value_base, 0, 0);
	BIND_EV_PARAM(value_offset, 1, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_fixed_pitch((value_base << 6) + value_offset);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_ssg_envelope(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_ssg_envelope_control(p_event->data == INT32_MIN ? 0 : p_event->data);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_operator_envelope_reset(MMLEvent *p_event) {
	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return p_event->next; // Check the mask.
	}

	_current_track->get_channel()->set_envelope_reset(p_event->data == 1);
	return p_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_sustain(MMLEvent *p_event) {
	GET_EV_PARAMS(2);
	BIND_EV_PARAM(release_rate, 0, INT32_MIN);
	BIND_EV_PARAM(release_sweep, 1, 0);

	if (_current_track->get_event_mask() & SiMMLTrack::MASK_OPERATOR) {
		return next_event->next; // Check the mask.
	}

	if (release_rate != INT32_MIN) {
		_current_track->get_channel()->set_all_release_rate(release_rate);
	}

	_current_track->set_release_sweep(release_sweep);
	return next_event->next;
}

MMLEvent *SiMMLSequencer::_on_mml_register_update(MMLEvent *p_event) {
	GET_EV_PARAMS(2);

	_current_track->call_update_register(ev_params[0], ev_params[1]);
	return next_event->next;
}

#undef GET_EV_PARAMS
#undef BIND_EV_PARAM
#undef BIND_EV_PARAM_RANGE

//

void SiMMLSequencer::_register_process_events() {
	_set_mml_event_listener(MMLEvent::NOP,       Callable((MMLSequencer *)this, "_default_on_no_operation"));
	_set_mml_event_listener(MMLEvent::PROCESS,   Callable((MMLSequencer *)this, "_default_on_process"));
	_set_mml_event_listener(MMLEvent::REST,      Callable(this, "_on_mml_rest"));
	_set_mml_event_listener(MMLEvent::NOTE,      Callable(this, "_on_mml_note"));
	_set_mml_event_listener(MMLEvent::SLUR,      Callable(this, "_on_mml_slur"));
	_set_mml_event_listener(MMLEvent::SLUR_WEAK, Callable(this, "_on_mml_slur_weak"));
	_set_mml_event_listener(MMLEvent::PITCHBEND, Callable(this, "_on_mml_pitch_bend"));
}

void SiMMLSequencer::_register_dummy_process_events() {
	_set_mml_event_listener(MMLEvent::NOP,       Callable((MMLSequencer *)this, "_no_process"));
	_set_mml_event_listener(MMLEvent::PROCESS,   Callable((MMLSequencer *)this, "_dummy_on_process"));
	_set_mml_event_listener(MMLEvent::REST,      Callable((MMLSequencer *)this, "_dummy_on_process_event"));
	_set_mml_event_listener(MMLEvent::NOTE,      Callable((MMLSequencer *)this, "_dummy_on_process_event"));
	_set_mml_event_listener(MMLEvent::SLUR,      Callable((MMLSequencer *)this, "_dummy_on_process_event"));
	_set_mml_event_listener(MMLEvent::SLUR_WEAK, Callable((MMLSequencer *)this, "_dummy_on_process_event"));
	_set_mml_event_listener(MMLEvent::PITCHBEND, Callable((MMLSequencer *)this, "_dummy_on_process_event"));
}

void SiMMLSequencer::_register_event_listeners() {
	// Pitch.
	_create_mml_event_listener("k",    Callable(this, "_on_mml_detune"));
	_create_mml_event_listener("kt",   Callable(this, "_on_mml_key_transition"));
	_create_mml_event_listener("!@kr", Callable(this, "_on_mml_relative_detune"));

	// Track settings.
	_create_mml_event_listener("@mask", Callable(this, "_on_mml_event_mask"));
	_set_mml_event_listener(MMLEvent::QUANT_RATIO,  Callable(this, "_on_mml_quant_ratio"));
	_set_mml_event_listener(MMLEvent::QUANT_COUNT,  Callable(this, "_on_mml_quant_count"));

	// Volume.
	_create_mml_event_listener("p",  Callable(this, "_on_mml_pan"));
	_create_mml_event_listener("@p", Callable(this, "_on_mml_fine_pan"));
	_create_mml_event_listener("@f", Callable(this, "_on_mml_filter"));
	_create_mml_event_listener("x",  Callable(this, "_on_mml_expression"));
	_set_mml_event_listener(MMLEvent::VOLUME,       Callable(this, "_on_mml_volume"));
	_set_mml_event_listener(MMLEvent::VOLUME_SHIFT, Callable(this, "_on_mml_volume_shift"));
	_set_mml_event_listener(MMLEvent::FINE_VOLUME,  Callable(this, "_on_mml_master_volume"));
	_create_mml_event_listener("%v",  Callable(this, "_on_mml_volume_setting"));
	_create_mml_event_listener("%x",  Callable(this, "_on_mml_expression_setting"));
	_create_mml_event_listener("%f",  Callable(this, "_on_mml_filter_mode"));

	// Channel settings.
	_create_mml_event_listener("@clock", Callable(this, "_on_mml_clock"));
	_create_mml_event_listener("@al", Callable(this, "_on_mml_algorithm"));
	_create_mml_event_listener("@fb", Callable(this, "_on_mml_feedback"));
	_create_mml_event_listener("@r",  Callable(this, "_on_mml_ring_modulation"));
	_set_mml_event_listener(MMLEvent::MOD_TYPE,    Callable(this, "_on_mml_module_type"));
	_set_mml_event_listener(MMLEvent::INPUT_PIPE,  Callable(this, "_on_mml_input"));
	_set_mml_event_listener(MMLEvent::OUTPUT_PIPE, Callable(this, "_on_mml_output"));
	_create_mml_event_listener("%t",  Callable(this, "_on_mml_event_trigger"));
	_create_mml_event_listener("%e",  Callable(this, "_on_mml_dispatch_event"));

	// Operator settings.
	_create_mml_event_listener("i",   Callable(this, "_on_mml_slot_index"));
	_create_mml_event_listener("@rr", Callable(this, "_on_mml_operator_release_rate"));
	_create_mml_event_listener("@tl", Callable(this, "_on_mml_operator_total_level"));
	_create_mml_event_listener("@ml", Callable(this, "_on_mml_operator_multiple"));
	_create_mml_event_listener("@dt", Callable(this, "_on_mml_operator_detune"));
	_create_mml_event_listener("@ph", Callable(this, "_on_mml_operator_phase"));
	_create_mml_event_listener("@fx", Callable(this, "_on_mml_operator_fixed_note"));
	_create_mml_event_listener("@se", Callable(this, "_on_mml_operator_ssg_envelope"));
	_create_mml_event_listener("@er", Callable(this, "_on_mml_operator_envelope_reset"));
	_set_mml_event_listener(MMLEvent::MOD_PARAM, Callable(this, "_on_mml_operator_parameter"));
	_create_mml_event_listener("s",   Callable(this, "_on_mml_sustain"));

	// Modulation.
	_create_mml_event_listener("@lfo", Callable(this, "_on_mml_lf_oscillator"));
	_create_mml_event_listener("mp", Callable(this, "_on_mml_pitch_modulation"));
	_create_mml_event_listener("ma", Callable(this, "_on_mml_amplitude_modulation"));

	// Envelope.
	_create_mml_event_listener("@fps", Callable(this, "_on_mml_envelope_fps"));
	_envelope_event_id = _create_mml_event_listener("@@", Callable(this, "_on_mml_tone_envelope"));
	_create_mml_event_listener("na", Callable(this, "_on_mml_amplitude_envelope"));
	_create_mml_event_listener("np", Callable(this, "_on_mml_pitch_envelope"));
	_create_mml_event_listener("nt", Callable(this, "_on_mml_note_envelope"));
	_create_mml_event_listener("nf", Callable(this, "_on_mml_filter_envelope"));
	_create_mml_event_listener("_@@", Callable(this, "_on_mml_tone_release_envelope"));
	_create_mml_event_listener("_na", Callable(this, "_on_mml_amplitude_release_envelope"));
	_create_mml_event_listener("_np", Callable(this, "_on_mml_pitch_release_envelope"));
	_create_mml_event_listener("_nt", Callable(this, "_on_mml_note_release_envelope"));
	_create_mml_event_listener("_nf", Callable(this, "_on_mml_filter_release_envelope"));
	_create_mml_event_listener("!na", Callable(this, "_on_mml_amplitude_envelope_tsscp"));
	_create_mml_event_listener("po",  Callable(this, "_on_mml_portament"));

	// These can be swapped for dummy processing.
	_register_process_events();

	_set_mml_event_listener(MMLEvent::DRIVER_NOTE, Callable(this, "_on_mml_driver_note_on"));
	_set_mml_event_listener(MMLEvent::REGISTER,    Callable(this, "_on_mml_register_update"));
}

void SiMMLSequencer::_reset_initial_operator_params() {
	SiOPMOperatorParams *op_params = _module->get_init_operator_params();

	op_params->set_attack_rate(63);
	op_params->set_decay_rate(0);
	op_params->set_sustain_rate(0);
	op_params->set_release_rate(28);
	op_params->set_sustain_level(0);
	op_params->set_total_level(0);
	op_params->set_key_scaling_rate(0);
	op_params->set_key_scaling_level(0);
	op_params->set_fine_multiple(128);
	op_params->set_detune1(0);
	op_params->set_detune(0);
	op_params->set_amplitude_modulation_shift(1);
	op_params->set_initial_phase(0);
	op_params->set_fixed_pitch(0);
	op_params->set_frequency_modulation_level(5);
	op_params->set_pulse_generator_type(PG_SQUARE);
}

void SiMMLSequencer::_reset_parser_settings() {
	_parser_settings->default_bpm = 120;
	_parser_settings->default_l_value  = 4;
	_parser_settings->default_quant_ratio = 6;
	_parser_settings->max_quant_ratio = 8;
	_parser_settings->set_default_octave(5);
	_parser_settings->max_volume = 512;
	_parser_settings->default_volume = 256;
	_parser_settings->max_fine_volume = 128;
	_parser_settings->default_fine_volume = 64;
}

void SiMMLSequencer::_bind_methods() {
	// To be used as callables.

	ClassDB::bind_method(D_METHOD("_on_mml_rest", "event"),                       &SiMMLSequencer::_on_mml_rest);
	ClassDB::bind_method(D_METHOD("_on_mml_note", "event"),                       &SiMMLSequencer::_on_mml_note);
	ClassDB::bind_method(D_METHOD("_on_mml_slur", "event"),                       &SiMMLSequencer::_on_mml_slur);
	ClassDB::bind_method(D_METHOD("_on_mml_slur_weak", "event"),                  &SiMMLSequencer::_on_mml_slur_weak);
	ClassDB::bind_method(D_METHOD("_on_mml_pitch_bend", "event"),                 &SiMMLSequencer::_on_mml_pitch_bend);
	ClassDB::bind_method(D_METHOD("_on_mml_detune", "event"),                     &SiMMLSequencer::_on_mml_detune);
	ClassDB::bind_method(D_METHOD("_on_mml_key_transition", "event"),             &SiMMLSequencer::_on_mml_key_transition);
	ClassDB::bind_method(D_METHOD("_on_mml_relative_detune", "event"),            &SiMMLSequencer::_on_mml_relative_detune);
	ClassDB::bind_method(D_METHOD("_on_mml_event_mask", "event"),                 &SiMMLSequencer::_on_mml_event_mask);
	ClassDB::bind_method(D_METHOD("_on_mml_quant_ratio", "event"),                &SiMMLSequencer::_on_mml_quant_ratio);
	ClassDB::bind_method(D_METHOD("_on_mml_quant_count", "event"),                &SiMMLSequencer::_on_mml_quant_count);
	ClassDB::bind_method(D_METHOD("_on_mml_pan", "event"),                        &SiMMLSequencer::_on_mml_pan);
	ClassDB::bind_method(D_METHOD("_on_mml_fine_pan", "event"),                   &SiMMLSequencer::_on_mml_fine_pan);
	ClassDB::bind_method(D_METHOD("_on_mml_filter", "event"),                     &SiMMLSequencer::_on_mml_filter);
	ClassDB::bind_method(D_METHOD("_on_mml_expression", "event"),                 &SiMMLSequencer::_on_mml_expression);
	ClassDB::bind_method(D_METHOD("_on_mml_volume", "event"),                     &SiMMLSequencer::_on_mml_volume);
	ClassDB::bind_method(D_METHOD("_on_mml_volume_shift", "event"),               &SiMMLSequencer::_on_mml_volume_shift);
	ClassDB::bind_method(D_METHOD("_on_mml_master_volume", "event"),              &SiMMLSequencer::_on_mml_master_volume);
	ClassDB::bind_method(D_METHOD("_on_mml_volume_setting", "event"),             &SiMMLSequencer::_on_mml_volume_setting);
	ClassDB::bind_method(D_METHOD("_on_mml_expression_setting", "event"),         &SiMMLSequencer::_on_mml_expression_setting);
	ClassDB::bind_method(D_METHOD("_on_mml_filter_mode", "event"),                &SiMMLSequencer::_on_mml_filter_mode);
	ClassDB::bind_method(D_METHOD("_on_mml_clock", "event"),                      &SiMMLSequencer::_on_mml_clock);
	ClassDB::bind_method(D_METHOD("_on_mml_algorithm", "event"),                  &SiMMLSequencer::_on_mml_algorithm);
	ClassDB::bind_method(D_METHOD("_on_mml_feedback", "event"),                   &SiMMLSequencer::_on_mml_feedback);
	ClassDB::bind_method(D_METHOD("_on_mml_ring_modulation", "event"),            &SiMMLSequencer::_on_mml_ring_modulation);
	ClassDB::bind_method(D_METHOD("_on_mml_module_type", "event"),                &SiMMLSequencer::_on_mml_module_type);
	ClassDB::bind_method(D_METHOD("_on_mml_input", "event"),                      &SiMMLSequencer::_on_mml_input);
	ClassDB::bind_method(D_METHOD("_on_mml_output", "event"),                     &SiMMLSequencer::_on_mml_output);
	ClassDB::bind_method(D_METHOD("_on_mml_event_trigger", "event"),              &SiMMLSequencer::_on_mml_event_trigger);
	ClassDB::bind_method(D_METHOD("_on_mml_dispatch_event", "event"),             &SiMMLSequencer::_on_mml_dispatch_event);
	ClassDB::bind_method(D_METHOD("_on_mml_slot_index", "event"),                 &SiMMLSequencer::_on_mml_slot_index);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_release_rate", "event"),      &SiMMLSequencer::_on_mml_operator_release_rate);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_total_level", "event"),       &SiMMLSequencer::_on_mml_operator_total_level);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_multiple", "event"),          &SiMMLSequencer::_on_mml_operator_multiple);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_detune", "event"),            &SiMMLSequencer::_on_mml_operator_detune);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_phase", "event"),             &SiMMLSequencer::_on_mml_operator_phase);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_fixed_note", "event"),        &SiMMLSequencer::_on_mml_operator_fixed_note);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_ssg_envelope", "event"),      &SiMMLSequencer::_on_mml_operator_ssg_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_envelope_reset", "event"),    &SiMMLSequencer::_on_mml_operator_envelope_reset);
	ClassDB::bind_method(D_METHOD("_on_mml_operator_parameter", "event"),         &SiMMLSequencer::_on_mml_operator_parameter);
	ClassDB::bind_method(D_METHOD("_on_mml_sustain", "event"),                    &SiMMLSequencer::_on_mml_sustain);
	ClassDB::bind_method(D_METHOD("_on_mml_lf_oscillator", "event"),              &SiMMLSequencer::_on_mml_lf_oscillator);
	ClassDB::bind_method(D_METHOD("_on_mml_pitch_modulation", "event"),           &SiMMLSequencer::_on_mml_pitch_modulation);
	ClassDB::bind_method(D_METHOD("_on_mml_amplitude_modulation", "event"),       &SiMMLSequencer::_on_mml_amplitude_modulation);
	ClassDB::bind_method(D_METHOD("_on_mml_envelope_fps", "event"),               &SiMMLSequencer::_on_mml_envelope_fps);
	ClassDB::bind_method(D_METHOD("_on_mml_tone_envelope", "event"),              &SiMMLSequencer::_on_mml_tone_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_amplitude_envelope", "event"),         &SiMMLSequencer::_on_mml_amplitude_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_pitch_envelope", "event"),             &SiMMLSequencer::_on_mml_pitch_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_note_envelope", "event"),              &SiMMLSequencer::_on_mml_note_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_filter_envelope", "event"),            &SiMMLSequencer::_on_mml_filter_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_tone_release_envelope", "event"),      &SiMMLSequencer::_on_mml_tone_release_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_amplitude_release_envelope", "event"), &SiMMLSequencer::_on_mml_amplitude_release_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_pitch_release_envelope", "event"),     &SiMMLSequencer::_on_mml_pitch_release_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_note_release_envelope", "event"),      &SiMMLSequencer::_on_mml_note_release_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_filter_release_envelope", "event"),    &SiMMLSequencer::_on_mml_filter_release_envelope);
	ClassDB::bind_method(D_METHOD("_on_mml_amplitude_envelope_tsscp", "event"),   &SiMMLSequencer::_on_mml_amplitude_envelope_tsscp);
	ClassDB::bind_method(D_METHOD("_on_mml_portament", "event"),                  &SiMMLSequencer::_on_mml_portament);
	ClassDB::bind_method(D_METHOD("_on_mml_driver_note_on", "event"),             &SiMMLSequencer::_on_mml_driver_note_on);
	ClassDB::bind_method(D_METHOD("_on_mml_register_update", "event"),            &SiMMLSequencer::_on_mml_register_update);
}

SiMMLSequencer::SiMMLSequencer(SiOPMModule *p_module) :
		MMLSequencer() {
	_table = SiMMLRefTable::get_instance();
	_module = p_module;
	_connector = memnew(MMLExecutorConnector);

	_macro_strings.resize_zeroed(MACRO_SIZE);

	_register_event_listeners();
	_reset_initial_operator_params();
	_reset_parser_settings();
}

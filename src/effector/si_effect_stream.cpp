/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_stream.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include "chip/siopm_sound_chip.h"
#include "chip/siopm_stream.h"
#include "effector/si_effector.h"

int SiEffectStream::get_pan() const {
	return _pan - 64;
}

void SiEffectStream::set_pan(int p_value) {
	_pan = CLAMP(p_value + 64, 0, 128);
}

bool SiEffectStream::is_outputting_directly() const {
	return (!_has_effect_send && _volumes[0] == 1 && _pan == 64);
}

void SiEffectStream::set_all_stream_send_levels(Vector<int> p_param) {
	for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		int value = p_param[i];
		_volumes.write[i] = value == INT32_MIN ? 0 : (value * 0.0078125);
	}

	_has_effect_send = false;
	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		if (_volumes[i] > 0) {
			_has_effect_send = true;
		}
	}
}

void SiEffectStream::set_stream_send(int p_stream_num, double p_volume) {
	_volumes.write[p_stream_num] = p_volume;
	if (p_stream_num == 0) {
		return;
	}

	if (p_volume > 0) {
		_has_effect_send = true;
		return;
	}

	_has_effect_send = false;
	for (int i = 1; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		if (_volumes[i] > 0) {
			_has_effect_send = true;
		}
	}
}

double SiEffectStream::get_stream_send(int p_stream_num) {
	return _volumes[p_stream_num];
}

void SiEffectStream::connect(SiOPMStream *p_output) {
	_output_streams.write[0] = p_output;
}

int SiEffectStream::prepare_process() {
	if (_chain.is_empty()) {
		return 0;
	}

	_stream->set_channel_count(_chain[0]->prepare_process());
	for (int i = 1; i < _chain.size(); i++) {
		_chain[i]->prepare_process();
	}

	return _stream->get_channel_count();
}

int SiEffectStream::process(int p_start_idx, int p_length, bool p_write_in_stream) {
	Vector<double> *buffer = _stream->get_buffer_ptr();
	int channel_count = _stream->get_channel_count();

	for (int i = 0; i < _chain.size(); i++) {
		channel_count = _chain[i]->process(channel_count, buffer, p_start_idx, p_length);
	}

	if (p_write_in_stream) {
		if (_has_effect_send) {
			for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
				if (_volumes[i] > 0) {
					SiOPMStream *stream = _output_streams[i];
					if (!stream) {
						stream = _sound_chip->get_stream_slot(i);
					}
					if (stream) {
						stream->write_from_vector(buffer, p_start_idx, p_start_idx, p_length, _volumes[i], _pan, 2);
					}
				}
			}
		} else {
			SiOPMStream *stream = _output_streams[0];
			if (!stream) {
				stream = _sound_chip->get_output_stream();
			}
			stream->write_from_vector(buffer, p_start_idx, p_start_idx, p_length, _volumes[0], _pan, 2);
		}
	}

	return channel_count;
}

//

void SiEffectStream::_add_effect(String p_cmd, Vector<double> p_args, int p_argc) {
	if (p_argc == 0) {
		return;
	}

	Ref<SiEffectBase> effect = SiEffector::get_effect_instance(p_cmd);
	if (effect.is_valid()) {
		effect->set_by_mml(p_args);
		_chain.push_back(effect);
	}
}

void SiEffectStream::_set_volume(int p_slot, String p_cmd, Vector<double> p_args, int p_argc) {
	if (p_argc == 0) {
		return;
	}

	if (p_cmd == "p") {
		_pan = (((int)p_args[0]) << 4) - 64;
	} else if (p_cmd == "@p") {
		_pan = (int)p_args[0];
	} else if (p_cmd == "@v") {
		double value = ((int)p_args[0]) * 0.0078125;
		set_stream_send(0, CLAMP(value, 0, 1));

		int max_count = p_argc;
		if ((max_count + p_slot) >= SiOPMSoundChip::STREAM_SEND_SIZE) {
			max_count = SiOPMSoundChip::STREAM_SEND_SIZE - p_slot - 1;
		}

		for (int i = 1; i < max_count; i++) {
			value = ((int)p_args[i]) * 0.0078125;
			set_stream_send(i + p_slot, CLAMP(value, 0, 1));
		}
	}
}

void SiEffectStream::parse_mml(int p_slot, String p_mml, String p_postfix) {
	const int max_argc = 16;

	int argc = 0;
	Vector<double> args;
	args.resize_zeroed(max_argc);

#define CLEAR_ARGS()                      \
	for (int a = 0; a < max_argc; a++) {  \
		args.write[a] = NAN;              \
	}                                     \
	argc = 0;

	// Reset and clear everything.
	initialize(0);
	CLEAR_ARGS();

	String command;

	Ref<RegEx> re_mml = RegEx::create_from_string("([a-zA-Z_]+|,)\\s*([.\\-\\d]+)?");
	Ref<RegEx> re_postfix = RegEx::create_from_string("(p|@p|@v|,)\\s*([.\\-\\d]+)?");

	// Parse MML.

	TypedArray<RegExMatch> matches = re_mml->search_all(p_mml);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> res = matches[i];

		if (res->get_string(1) == ",") {
			args.write[argc] = res->get_string(2).to_float();
			argc++;
		} else {
			_add_effect(command, args, argc);
			CLEAR_ARGS();

			command = res->get_string(1);
			args.write[0] = res->get_string(2).to_float();
			argc = 1;
		}
	}

	_add_effect(command, args, argc);
	CLEAR_ARGS();

	// Parse the postfix.

	matches = re_postfix->search_all(p_postfix);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> res = matches[i];

		if (res->get_string(1) == ",") {
			args.write[argc] = res->get_string(2).to_float();
			argc++;
		} else {
			_set_volume(p_slot, command, args, argc);
			CLEAR_ARGS();

			command = res->get_string(1);
			args.write[0] = res->get_string(2).to_float();
			argc = 1;
		}
	}

	_set_volume(p_slot, command, args, argc);
	CLEAR_ARGS();

#undef CLEAR_ARGS
}

void SiEffectStream::initialize(int p_depth) {
	free();
	reset();

	for (int i = 0; i < SiOPMSoundChip::STREAM_SEND_SIZE; i++) {
		_volumes.write[i] = 0;
		_output_streams.write[i] = nullptr;
	}

	_volumes.write[0] = 1;
	_pan = 64;
	_has_effect_send = false;
	_depth = p_depth;
}

void SiEffectStream::reset() {
	_stream->resize(_sound_chip->get_buffer_length() << 1);
	_stream->clear();
}

void SiEffectStream::free() {
	for (Ref<SiEffectBase> effect : _chain) {
		effect->set_free(true);
	}
	_chain.clear();
}

SiEffectStream::SiEffectStream(SiOPMSoundChip *p_chip, SiOPMStream *p_stream) {
	_sound_chip = p_chip;
	if (p_stream) {
		_stream = p_stream;
	} else {
		_stream = memnew(SiOPMStream);
	}

	_volumes.resize_zeroed(SiOPMSoundChip::STREAM_SEND_SIZE);
	_output_streams.resize_zeroed(SiOPMSoundChip::STREAM_SEND_SIZE);
}

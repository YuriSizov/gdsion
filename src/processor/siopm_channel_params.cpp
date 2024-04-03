/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_params.h"

#include <godot_cpp/core/class_db.hpp>
#include "processor/siopm_module.h"
#include "processor/siopm_operator_params.h"
#include "processor/siopm_table.h"
#include "sequencer/base/mml_sequence.h"

using namespace godot;

SiOPMOperatorParams *SiOPMChannelParams::get_operator_params(int p_index) {
	// TODO: Check against operator_count? It can be bigger than the list size.
	ERR_FAIL_INDEX_V(p_index, operator_params.size(), nullptr);

	return operator_params[p_index];
}

double SiOPMChannelParams::get_master_volume(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, master_volumes.size(), 0);

	return master_volumes[p_index];
}

void SiOPMChannelParams::set_master_volume(int p_index, double p_value) {
	ERR_FAIL_INDEX(p_index, master_volumes.size());

	master_volumes.write[p_index] = p_value;
}

int SiOPMChannelParams::get_lfo_frame() const {
	return (int)(SiOPMTable::LFO_TIMER_INITIAL * 0.346938775510204 / lfo_frequency_step);
}

void SiOPMChannelParams::set_lfo_frame(int p_fps) {
	lfo_frequency_step = (int)(SiOPMTable::LFO_TIMER_INITIAL/(p_fps * 2.882352941176471));
}

void SiOPMChannelParams::set_by_opm_register(int p_channel, int p_address, int p_data) {
	if (p_address < 0x20) { // Module parameter
		switch (p_address) {
			case 15: { // NOIZE:7 FREQ:4-0 for channel#7
				if (p_channel == 7 && (p_data & 128)) {
					operator_params[3]->pulse_generator_type = SiOPMTable::PG_NOISE_PULSE;
					operator_params[3]->pitch_table_type = SiOPMTable::PT_OPM_NOISE;
					operator_params[3]->fixed_pitch = ((p_data & 31) << 6) + 2048;
				}
			} break;

			case 24: { // LFO FREQ:7-0 for all 8 channels
				lfo_frequency_step = SiOPMTable::get_instance()->lfo_timer_steps[p_data];
			} break;

			case 25: { // A(0)/P(1):7 DEPTH:6-0 for all 8 channels
				if (p_data & 128) {
					pitch_modulation_depth = p_data & 127;
				} else {
					amplitude_modulation_depth = p_data & 127;
				}
			} break;

			case 27: { // LFO WS:10 for all 8 channels
				lfo_wave_shape = p_data & 3;
			} break;
		}
	} else if (p_channel == (p_address & 7)) {
		if (p_address < 0x40) { // Channel parameter
			switch ((p_address - 0x20) >> 3) {
				case 0: { // L:7 R:6 FB:5-3 ALG:2-0
					algorithm = p_data & 7;
					feedback = (p_data >> 3) & 7;

					int value = p_data >> 6;
					master_volumes.write[0] = (value != 0 ? 0.5 : 0);
					pan = (value == 1 ? 128 : (value == 2 ? 0 : 64));
				} break;

				case 1: break; // KC:6-0
				case 2: break; // KF:6-0
				case 3: break; // PMS:6-4 AMS:10
			}
		} else { // Operator parameter
			int ops[4] = { 3, 1, 2, 0 };
			int op_index = ops[(p_address >> 3) & 3];
			SiOPMOperatorParams *op_params = operator_params[op_index];

			switch ((p_address - 0x40) >> 5) {
				case 0: { // DT1:6-4 MUL:3-0
					op_params->detune1    = (p_data >> 4) & 7;
					op_params->set_multiple((p_data     ) & 15);
				} break;
				case 1: { // TL:6-0
					op_params->total_level = p_data & 127;
				} break;
				case 2: { // KS:76 AR:4-0
					op_params->key_scaling_rate = (p_data >> 6) & 3;
					op_params->attack_rate      = (p_data & 31) << 1;
				} break;
				case 3: { // AMS:7 DR:4-0
					op_params->amplitude_modulation_shift = ((p_data >> 7) & 1) << 1;
					op_params->decay_rate                  = (p_data & 31) << 1;
				} break;
				case 4: { // DT2:76 SR:4-0
					int options[4] = { 0, 384, 500, 608 };
					op_params->detune = options[(p_data >> 6) & 3];
					op_params->sustain_rate   = (p_data & 31) << 1;
				} break;
				case 5: { // SL:7-4 RR:3-0
					op_params->sustain_level = (p_data >> 4) & 15;
					op_params->release_rate  = (p_data & 15) << 2;
				} break;
			}
		}
	}
}

String SiOPMChannelParams::to_string() const {
	// Class was renamed to say "params", but this may be needed for compatibility,
	// so keeping the old name here.
	String str = "SiOPMChannelParam : ";
	str += "opeCount=" + itos(operator_count) + "\n";

#define FORMAT_VALUE(m_name, m_value)                   \
	str += "  " + String(m_name) + "=" + itos(m_value) + "\n";

#define FORMAT_VALUE_PAIR(m_name1, m_value1, m_name2, m_value2)                                    \
	str += "  " + String(m_name1) + "=" + itos(m_value1) + " / " + String(m_name2) + "=" + itos(m_value2) + "\n";

	FORMAT_VALUE("freq.ratio", envelope_frequency_ratio);
	FORMAT_VALUE("alg", algorithm);
	FORMAT_VALUE_PAIR("fb ", feedback,  "fbc", feedback_connection);
	FORMAT_VALUE_PAIR("lws", lfo_wave_shape, "lfq", SiOPMTable::LFO_TIMER_INITIAL * 0.005782313 / lfo_frequency_step);
	FORMAT_VALUE_PAIR("amd", amplitude_modulation_depth, "pmd", pitch_modulation_depth);
	FORMAT_VALUE_PAIR("vol", master_volumes[0],  "pan", pan - 64);
	FORMAT_VALUE("filter type", filter_type);
	FORMAT_VALUE_PAIR("co", filter_cutoff, "res", filter_resonance);

#undef FORMAT_VALUE
#undef FORMAT_VALUE_PAIR

	str += "fenv=" + itos(filter_attack_rate) + "/" + itos(filter_decay_rate1) + "/"+ itos(filter_decay_rate2) + "/"+ itos(filter_release_rate) + "\n";
	str += "feco=" + itos(filter_decay_offset1) + "/"+ itos(filter_decay_offset2) + "/"+ itos(filter_sustain_offset) + "/"+ itos(filter_release_offset) + "\n";

	for (int i = 0; i < operator_count; i++) {
		str += operator_params[i]->to_string() + "\n";
	}

	return str;
}

void SiOPMChannelParams::initialize() {
	operator_count = 1;

	algorithm = 0;
	feedback = 0;
	feedback_connection = 0;

	lfo_wave_shape = SiOPMTable::LFO_WAVE_TRIANGLE;
	lfo_frequency_step = 12126; // 12126 = 30 frames / 100 fratio

	amplitude_modulation_depth = 0;
	pitch_modulation_depth = 0;
	envelope_frequency_ratio = 100;

	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		master_volumes.write[i] = 0;
	}
	master_volumes.write[0] = 0.5;
	pan = 64;

	filter_type = 0;
	filter_cutoff = 128;
	filter_resonance = 0;
	filter_attack_rate = 0;
	filter_decay_rate1 = 0;
	filter_decay_rate2 = 0;
	filter_release_rate = 0;
	filter_decay_offset1 = 128;
	filter_decay_offset2 = 64;
	filter_sustain_offset = 32;
	filter_release_offset = 128;

	for (int i = 0; i < 4; i++) {
		operator_params[i]->initialize();
	}

	init_sequence->free();
}

void SiOPMChannelParams::copy_from(SiOPMChannelParams *p_params) {
	operator_count = p_params->operator_count;

	algorithm = p_params->algorithm;
	feedback = p_params->feedback;
	feedback_connection = p_params->feedback_connection;

	lfo_wave_shape = p_params->lfo_wave_shape;
	lfo_frequency_step = p_params->lfo_frequency_step;

	amplitude_modulation_depth = p_params->amplitude_modulation_depth;
	pitch_modulation_depth = p_params->pitch_modulation_depth;
	envelope_frequency_ratio = p_params->envelope_frequency_ratio;

	for (int i = 1; i < SiOPMModule::STREAM_SEND_SIZE; i++) {
		master_volumes.write[i] = p_params->master_volumes[i];
	}
	pan = p_params->pan;

	filter_type = p_params->filter_type;
	filter_cutoff = p_params->filter_cutoff;
	filter_resonance = p_params->filter_resonance;
	filter_attack_rate = p_params->filter_attack_rate;
	filter_decay_rate1 = p_params->filter_decay_rate1;
	filter_decay_rate2 = p_params->filter_decay_rate2;
	filter_release_rate = p_params->filter_release_rate;
	filter_decay_offset1 = p_params->filter_decay_offset1;
	filter_decay_offset2 = p_params->filter_decay_offset2;
	filter_sustain_offset = p_params->filter_sustain_offset;
	filter_release_offset = p_params->filter_release_offset;

	for (int i = 0; i < 4; i++) {
		operator_params[i]->copy_from(p_params->operator_params[i]);
	}

	init_sequence->free();
}

void SiOPMChannelParams::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_filter_type"), &SiOPMChannelParams::get_filter_type);
	ClassDB::bind_method(D_METHOD("set_filter_type", "value"), &SiOPMChannelParams::set_filter_type);

	ClassDB::bind_method(D_METHOD("get_filter_cutoff"), &SiOPMChannelParams::get_filter_cutoff);
	ClassDB::bind_method(D_METHOD("set_filter_cutoff", "value"), &SiOPMChannelParams::set_filter_cutoff);

	ClassDB::bind_method(D_METHOD("get_filter_resonance"), &SiOPMChannelParams::get_filter_resonance);
	ClassDB::bind_method(D_METHOD("set_filter_resonance", "value"), &SiOPMChannelParams::set_filter_resonance);

	ClassDB::bind_method(D_METHOD("get_filter_attack_rate"), &SiOPMChannelParams::get_filter_attack_rate);
	ClassDB::bind_method(D_METHOD("set_filter_attack_rate", "value"), &SiOPMChannelParams::set_filter_attack_rate);

	ClassDB::bind_method(D_METHOD("get_filter_decay_rate1"), &SiOPMChannelParams::get_filter_decay_rate1);
	ClassDB::bind_method(D_METHOD("set_filter_decay_rate1", "value"), &SiOPMChannelParams::set_filter_decay_rate1);

	ClassDB::bind_method(D_METHOD("get_filter_decay_rate2"), &SiOPMChannelParams::get_filter_decay_rate2);
	ClassDB::bind_method(D_METHOD("set_filter_decay_rate2", "value"), &SiOPMChannelParams::set_filter_decay_rate2);

	ClassDB::bind_method(D_METHOD("get_filter_release_rate"), &SiOPMChannelParams::get_filter_release_rate);
	ClassDB::bind_method(D_METHOD("set_filter_release_rate", "value"), &SiOPMChannelParams::set_filter_release_rate);

	ClassDB::bind_method(D_METHOD("get_filter_decay_offset1"), &SiOPMChannelParams::get_filter_decay_offset1);
	ClassDB::bind_method(D_METHOD("set_filter_decay_offset1", "value"), &SiOPMChannelParams::set_filter_decay_offset1);

	ClassDB::bind_method(D_METHOD("get_filter_decay_offset2"), &SiOPMChannelParams::get_filter_decay_offset2);
	ClassDB::bind_method(D_METHOD("set_filter_decay_offset2", "value"), &SiOPMChannelParams::set_filter_decay_offset2);

	ClassDB::bind_method(D_METHOD("get_filter_sustain_offset"), &SiOPMChannelParams::get_filter_sustain_offset);
	ClassDB::bind_method(D_METHOD("set_filter_sustain_offset", "value"), &SiOPMChannelParams::set_filter_sustain_offset);

	ClassDB::bind_method(D_METHOD("get_filter_release_offset"), &SiOPMChannelParams::get_filter_release_offset);
	ClassDB::bind_method(D_METHOD("set_filter_release_offset", "value"), &SiOPMChannelParams::set_filter_release_offset);

	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_type"), "set_filter_type", "get_filter_type");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_cutoff"), "set_filter_cutoff", "get_filter_cutoff");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_resonance"), "set_filter_resonance", "get_filter_resonance");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_attack_rate"), "set_filter_attack_rate", "get_filter_attack_rate");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_decay_rate1"), "set_filter_decay_rate1", "get_filter_decay_rate1");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_decay_rate2"), "set_filter_decay_rate2", "get_filter_decay_rate2");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_release_rate"), "set_filter_release_rate", "get_filter_release_rate");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_decay_offset1"), "set_filter_decay_offset1", "get_filter_decay_offset1");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_decay_offset2"), "set_filter_decay_offset2", "get_filter_decay_offset2");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_sustain_offset"), "set_filter_sustain_offset", "get_filter_sustain_offset");
	ClassDB::add_property("SiOPMChannelParams", PropertyInfo(Variant::INT, "filter_release_offset"), "set_filter_release_offset", "get_filter_release_offset");
}

SiOPMChannelParams::SiOPMChannelParams() {
	init_sequence = memnew(MMLSequence);
	master_volumes.clear();
	master_volumes.resize_zeroed(SiOPMModule::STREAM_SEND_SIZE);

	operator_params.clear();
	for (int i = 0; i < 4; i++) {
		operator_params.push_back(memnew(SiOPMOperatorParams));
	}

	initialize();
}

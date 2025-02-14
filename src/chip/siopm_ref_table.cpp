/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "chip/siopm_ref_table.h"

#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/classes/random_number_generator.hpp>
#include "sequencer/simml_voice.h"

using namespace godot;

SiOPMRefTable *SiOPMRefTable::_instance = nullptr;

const double SiOPMRefTable::NOISE_WAVE_OUTPUT  = 1;
const double SiOPMRefTable::SQUARE_WAVE_OUTPUT = 1;
const double SiOPMRefTable::OUTPUT_MAX         = 0.5;

void SiOPMRefTable::initialize() {
	if (_instance) {
		return;
	}

	// Sets the instance internally.
	memnew(SiOPMRefTable());
}

void SiOPMRefTable::finalize() {
	if (_instance) {
		memdelete(_instance);
		_instance = nullptr;
	}
}

//

int SiOPMRefTable::calculate_log_table_index(double p_number) {
	// Original code suggests that the incoming number must be between -1 and 1, but this isn't true in practice.
	// ERR_FAIL_COND_V(p_number < -1 || p_number > 1, LOG_TABLE_BOTTOM);

	static const double log_coefficient = 369.3299304675746; // 369.3299304675746 = 256/log(2)
	static const double log_threshold = 0.0001220703125;     // 0.0001220703125 = 1/(2^13)

	if (p_number < 0) {
		return (p_number < -log_threshold) ? (((int(log(-p_number) * -log_coefficient + 0.5) + 1) << 1) + 1) : LOG_TABLE_BOTTOM;
	} else {
		return (p_number > log_threshold) ? ((int(log(p_number) * -log_coefficient + 0.5) + 1) << 1) : LOG_TABLE_BOTTOM;
	}
}

//

void SiOPMRefTable::reset_all_user_tables() {
	for (int i = 0; i < WAVE_TABLE_MAX; i++) {
		_custom_wave_tables.write[i] = Ref<SiOPMWaveTable>();
	}

	for (int i = 0; i < PCM_DATA_MAX; i++) {
		if (_pcm_voices[i].is_valid()) {
			Ref<SiOPMWavePCMTable> pcm_table = _pcm_voices[i]->get_wave_data();
			if (pcm_table.is_valid()) {
				pcm_table->clear();
			}
			_pcm_voices.write[i] = Ref<SiMMLVoice>();
		}
	}

	_stencil_custom_wave_tables.clear();
	_stencil_pcm_voices.clear();
}

void SiOPMRefTable::register_wave_table(int p_index, const Ref<SiOPMWaveTable> &p_table) {
	int index = p_index & (WAVE_TABLE_MAX - 1);
	_custom_wave_tables.write[index] = p_table;

	// MA-3 waveforms support up to 3 user defined values. If this table is one of the first
	// 3 custom tables, use it.
	if (index < 3) {
		// User defined waves are at offsets 15,23,31.
		wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_SINE + 15 + index * 8] = p_table;
	}
}

Ref<SiOPMWaveSamplerData> SiOPMRefTable::register_sampler_data(int p_index, const Variant &p_data, bool p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count) {
	Ref<SiOPMWaveSamplerData> sampler_data = memnew(SiOPMWaveSamplerData(p_data, p_ignore_note_off, p_pan, p_src_channel_count, p_channel_count));

	int bank = (p_index >> NOTE_BITS) & (SAMPLER_TABLE_MAX - 1);
	sampler_tables[bank]->set_sample(sampler_data, p_index & (SAMPLER_DATA_MAX - 1));

	return sampler_data;
}

Ref<SiOPMWaveTable> SiOPMRefTable::get_wave_table(int p_index) {
	if (p_index < SiONPulseGeneratorType::PULSE_CUSTOM) {
		ERR_FAIL_INDEX_V(p_index, wave_tables.size(), no_wave_table);
		return wave_tables[p_index];
	}
	if (p_index < SiONPulseGeneratorType::PULSE_PCM) {
		int table_index = p_index - SiONPulseGeneratorType::PULSE_CUSTOM;

		if (table_index < _stencil_custom_wave_tables.size() && _stencil_custom_wave_tables[table_index].is_valid()) {
			return _stencil_custom_wave_tables[table_index];
		}

		if (table_index < _custom_wave_tables.size() && _custom_wave_tables[table_index].is_valid()) {
			return _custom_wave_tables[table_index];
		}

		return no_wave_table_opm;
	}

	return no_wave_table;
}

Ref<SiOPMWavePCMTable> SiOPMRefTable::get_pcm_data(int p_index) {
	int table_index = p_index & (PCM_DATA_MAX - 1);

	if (table_index < _stencil_pcm_voices.size() && _stencil_pcm_voices[table_index].is_valid()) {
		return _stencil_pcm_voices[table_index]->get_wave_data();
	}

	if (table_index < _pcm_voices.size() && _pcm_voices[table_index].is_valid()) {
		return _pcm_voices[table_index]->get_wave_data();
	}

	return nullptr;
}

Ref<SiMMLVoice> SiOPMRefTable::get_global_pcm_voice(int p_index) {
	int index = p_index & (PCM_DATA_MAX - 1);
	if (_pcm_voices[index].is_null()) {
		_pcm_voices.write[index] = SiMMLVoice::create_blank_pcm_voice(index);
	}

	return _pcm_voices[index];
}

Ref<SiMMLVoice> SiOPMRefTable::set_global_pcm_voice(int p_index, const Ref<SiMMLVoice> &p_from_voice) {
	int index = p_index & (PCM_DATA_MAX - 1);
	if (_pcm_voices[index].is_null()) {
		Ref<SiMMLVoice> voice;
		voice.instantiate();
		_pcm_voices.write[index] = voice;
	}

	_pcm_voices[index]->copy_from(p_from_voice);
	return _pcm_voices[index];
}

void SiOPMRefTable::set_sampler_table_stencil(int p_index, const Ref<SiOPMWaveSamplerTable> &p_table) {
	ERR_FAIL_INDEX(p_index, sampler_tables.size());

	sampler_tables[p_index]->set_stencil(p_table);
}

void SiOPMRefTable::clear_sampler_table_stencil(int p_index) {
	ERR_FAIL_INDEX(p_index, sampler_tables.size());

	sampler_tables[p_index]->set_stencil(nullptr);
}

//

void SiOPMRefTable::_set_constants(int p_fm_clock, double p_psg_clock, int p_sampling_rate) {
	ERR_FAIL_COND_MSG((p_sampling_rate != 44100 && p_sampling_rate != 22050), vformat("SiOPMRefTable: Invalid sampling rate '%d', only 44100 and 22050 are allowed.", p_sampling_rate));

	fm_clock = p_fm_clock;
	psg_clock = p_psg_clock;
	sampling_rate  = p_sampling_rate;
	sample_rate_pitch_shift = (sampling_rate == 44100 ? 0 : 1);
	clock_ratio = ((fm_clock / 64) << CLOCK_RATIO_BITS) / sampling_rate;
}

void SiOPMRefTable::_create_eg_tables() {
	// Table selector & timer steps for rates.
	{
		int i = 0;
		for (; i < 44; i++) { // rate = 0-43
			eg_timer_steps   [i] = int((1<<(i>>2)) * clock_ratio);
			eg_table_selector[i] = (i & 3);
		}
		for (; i < 48; i++) { // rate = 44-47
			eg_timer_steps   [i] = int(2047 * clock_ratio);
			eg_table_selector[i] = (i & 3);
		}
		for (; i < 60; i++) { // rate = 48-59
			eg_timer_steps   [i] = int(2047 * clock_ratio);
			eg_table_selector[i] = i - 44;
		}
		for (; i < 96; i++) { // rate = 60-95 (rate=60-95 are same as rate=63(maximum))
			eg_timer_steps   [i] = int(2047 * clock_ratio);
			eg_table_selector[i] = 16;
		}
		for (; i < 128; i++) { // rate = 96-127 (dummies for ar,dr,sr=0)
			eg_timer_steps   [i] = 0;
			eg_table_selector[i] = 17;
		}
	}

	// Level tables for SSG envelope.
	{
		int table_size = 1 << ENV_BITS;
		int half_size = table_size >> 2;

		int i = 0;
		for (; i < half_size; i++) {
			eg_level_tables[0][i] = i;               // normal table
			eg_level_tables[1][i] = i << 2;          // ssg positive
			eg_level_tables[2][i] = 512 - (i << 2);  // ssg negative
			eg_level_tables[3][i] = 512 + (i << 2);  // ssg positive + offset
			eg_level_tables[4][i] = 1024 - (i << 2); // ssg negative + offset
			eg_level_tables[5][i] = 0;               // ssg fixed at max
			eg_level_tables[6][i] = 1024;            // ssg fixed at min
		}
		for (; i < table_size; i++) {
			eg_level_tables[0][i] = i;               // normal table
			eg_level_tables[1][i] = 1024;            // ssg positive
			eg_level_tables[2][i] = 0;               // ssg negative
			eg_level_tables[3][i] = 1024;            // ssg positive + offset
			eg_level_tables[4][i] = 512;             // ssg negative + offset
			eg_level_tables[5][i] = 0;               // ssg fixed at max
			eg_level_tables[6][i] = 1024;            // ssg fixed at min
		}
	}

	// Sustain and total level tables.
	{
		for (int i = 0; i < 15; i++) {
			eg_sustain_level_table[i] = i << 5;
		}
		// sl(15) -> sl(1023)
		eg_sustain_level_table[15] = 31 << 5;

		// v(0-256) -> total_level(832,0). Translate linear volume to log scale gain.
		eg_total_level_tables[VM_LINEAR][0] = ENV_BOTTOM;
		eg_total_level_tables[VM_DR96DB][0] = ENV_BOTTOM;
		eg_total_level_tables[VM_DR64DB][0] = ENV_BOTTOM;
		eg_total_level_tables[VM_DR48DB][0] = ENV_BOTTOM;
		eg_total_level_tables[VM_DR32DB][0] = ENV_BOTTOM;
		for (int i = 1; i < 257; i++) {
			// 0.00390625 = 1/256
			eg_total_level_tables[VM_LINEAR][i] = calculate_log_table_index(i * 0.00390625) >> (LOG_VOLUME_BITS - ENV_BITS);
			eg_total_level_tables[VM_DR96DB][i] = (256 - i) * 4;                         //  (n/2)<<ENV_LSHIFT
			eg_total_level_tables[VM_DR64DB][i] = (int)((256 - i) * 2.6666666666666667); // ((n/2)<<ENV_LSHIFT)*2/3
			eg_total_level_tables[VM_DR48DB][i] = (256 - i) * 2;                         // ((n/2)<<ENV_LSHIFT)*1/2
			eg_total_level_tables[VM_DR32DB][i] = (int)((256 - i) * 1.333333333333333);  // ((n/2)<<ENV_LSHIFT)*1/3
		}
		// v(257-448) -> total_level (0,-192). Distortion.
		for (int i = 1; i < 193; i++) {
			int j = i + 256;
			eg_total_level_tables[VM_LINEAR][j] = -i;
			eg_total_level_tables[VM_DR96DB][j] = -i;
			eg_total_level_tables[VM_DR64DB][j] = -i;
			eg_total_level_tables[VM_DR48DB][j] = -i;
			eg_total_level_tables[VM_DR32DB][j] = -i;
		}
		// v(449-512) -> total_level=-192. Distortion.
		for (int i = 1; i < 65; i++) {
			int j = i + 448;
			eg_total_level_tables[VM_LINEAR][j] = ENV_TOP;
			eg_total_level_tables[VM_DR96DB][j] = ENV_TOP;
			eg_total_level_tables[VM_DR64DB][j] = ENV_TOP;
			eg_total_level_tables[VM_DR48DB][j] = ENV_TOP;
			eg_total_level_tables[VM_DR32DB][j] = ENV_TOP;
		}

		for (int i = 0; i < 129; i++) {
			// 0.0078125 = 1/128
			eg_linear_to_total_level_table[i] = calculate_log_table_index(i * 0.0078125) >> (LOG_VOLUME_BITS - ENV_BITS + ENV_LSHIFT);
		}
	}

	// Panning volume table.
	for (int i = 0; i < 129; i++) {
		pan_table[i] = Math::sin(i * 0.01227184630308513);  // 0.01227184630308513 = PI*0.5/128
	}
}

void SiOPMRefTable::_create_pg_tables() {
	// MIDI Note Number -> Key Code table
	for (int i = 0, j = 0; j < NOTE_TABLE_SIZE; i++, j = i - (i >> 2)) {
		if (i < 16) {
			note_number_to_key_code[j] = i;
		} else if (i < KEY_CODE_TABLE_SIZE) {
			note_number_to_key_code[j] = i - 16;
		} else {
			note_number_to_key_code[j] = KEY_CODE_TABLE_SIZE - 1;
		}
	}

	pitch_table.resize_zeroed(SiONPitchTableType::PITCH_TABLE_MAX);

	// Pitch table.
	{
		int table_step = HALF_TONE_RESOLUTION * 12; // 12 = 1 octave
		int table_size = PITCH_TABLE_SIZE;
		double pitch_delta = 1.0 / table_step;

		// Wave length table.
		{
			double pitch_value = 0;
			double pitch_coef = sampling_rate / 8.175798915643707; // = 5393.968278209282@44.1kHz sampling count @ MIDI note number = 0

			for (int i = 0; i < table_step; i++) {
				double value = Math::pow(2, -pitch_value) * pitch_coef;

				for (int j = i; j < table_size; j += table_step) {
					pitch_wave_length[j] = value;
					value *= 0.5;
				}

				pitch_value += pitch_delta;
			}
		}

		// Phase step tables.

		// OPM
		{
			Vector<int> table;
			table.resize_zeroed(PITCH_TABLE_SIZE);

			double pitch_value = 0;
			double pitch_coef = 8.175798915643707 * PHASE_MAX / sampling_rate; // dphase @ MIDI note number = 0

			for (int i = 0; i < table_step; i++) {
				double value = Math::pow(2, pitch_value) * pitch_coef;

				for (int j = i; j < table_size; j += table_step) {
					table.write[j] = (int)value;
					value *= 2;
				}

				pitch_value += pitch_delta;
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_OPM] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_OPM] = 0;
		}

		// PCM
		{
			Vector<int> table;
			table.resize_zeroed(PITCH_TABLE_SIZE);

			double pitch_value = 0;
			// dphase = pitchTablePCM[pitchIndex] >> (table_size (= PHASE_BITS - waveTable.fixedBits))
			double pitch_coef = 0.01858136117191752 * PHASE_MAX; // dphase @ MIDI note number = 0/ o0c=0.01858136117191752 -> o5a=1

			for (int i = 0; i < table_step; i++) {
				double value = Math::pow(2, pitch_value) * pitch_coef;

				for (int j = i; j < table_size; j += table_step) {
					table.write[j] = (int)value;
					value *= 2;
				}

				pitch_value += pitch_delta;
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_PCM] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_PCM] = 0xffffffff;
		}

		// PSG (table_size = 16)
		{
			Vector<int> table;
			table.resize_zeroed(PITCH_TABLE_SIZE);

			double pitch_value = 0;
			double pitch_coef = psg_clock * (PHASE_MAX >> 4) / sampling_rate;

			for (int i = 0; i < table_step; i++) {
				// 8.175798915643707 = [frequency @ MIDI note number = 0]
				// 130.8127826502993 = 8.175798915643707 * 16
				double value = psg_clock / (Math::pow(2, pitch_value) * 130.8127826502993);

				for (int j = i; j < table_size; j += table_step) {
					// Register value.
					int reg_value = MIN((int)(value + 0.5), 4096); // Cap at 4096.

					table.write[j] = (int)(pitch_coef / reg_value);
					value *= 0.5;
				}

				pitch_value += pitch_delta;
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_PSG] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_PSG] = 0;
		}
	}

	// Noise period tables.
	{
		// OPM noise period table.
		{
			int table_size = 32 << HALF_TONE_BITS;
			Vector<int> table;
			table.resize_zeroed(table_size);

			// noise_phase_shift = pitchTable[SiONPitchTableType::PITCH_TABLE_OPM_NOISE][noiseFreq] >> (PHASE_BITS - waveTable.fixedBits).
			double pitch_coef = PHASE_MAX * clock_ratio; // clock_ratio = ((clock/64)/rate) << CLOCK_RATIO_BITS

			int value = 0;
			for (int i = 0; i < 31; i++) {
				value = ((int)(pitch_coef / ((32 - i) * 0.5))) >> CLOCK_RATIO_BITS;

				for (int j = 0; j < HALF_TONE_RESOLUTION; j++) {
					table.write[(i << HALF_TONE_BITS) + j] = value;
				}
			}
			for (int i = 31 << HALF_TONE_BITS; i < table_size; i++) {
				table.write[i] = value;
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_OPM_NOISE] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_OPM_NOISE] = 0xffffffff;
		}

		// PSG noise period table.
		{
			int table_size = 32 << HALF_TONE_BITS;
			Vector<int> table;
			table.resize_zeroed(table_size);

			// noise_phase_shift = ((1<<PHASE_BIT)  /  ((nf/(clock/16))[sec]  /  (1/44100)[sec])) >> (PHASE_BIT - waveTable.fixedBits)
			double pitch_coef = PHASE_MAX * (double)fm_clock / (sampling_rate * 16);

			for (int i = 0; i < 32; i++) {
				int value = pitch_coef / i;

				for (int j = 0; j < HALF_TONE_RESOLUTION; j++) {
					table.write[(i << HALF_TONE_BITS) + j] = value;
				}
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_PSG_NOISE] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_PSG_NOISE] = 0xffffffff;
		}

		// APU noise period table.
		{
			static const int ref_values[] = { 4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068 };

			int table_size = 16 << HALF_TONE_BITS;
			Vector<int> table;
			table.resize_zeroed(table_size);

			// noise_phase_shift = ((1<<PHASE_BIT)  /  ((nf/clock)[sec]  /  (1/44100)[sec])) >> (PHASE_BIT - waveTable.fixedBits)
			double pitch_coef = PHASE_MAX * psg_clock / sampling_rate;

			for (int i = 0; i < 16; i++) {
				int value = pitch_coef / ref_values[i];

				for (int j = 0; j < HALF_TONE_RESOLUTION; j++) {
					table.write[(i << HALF_TONE_BITS) + j] = value;
				}
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_APU_NOISE] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_APU_NOISE] = 0xffffffff;
		}

		// Gameboy noise period.
		{
			static const int ref_values[] = {
				2,4,8,12,16,20,24,28,32,40,48,56,64,80,96,112,
				128,160,192,224,256,320,384,448,512,640,768,896,1024,1280,1536,1792,
				2048,2560,3072,3584,4096,5120,6144,7168,8192,10240,12288,14336,16384,20480,24576,28672,
				32768,40960,49152,57344,65536,81920,98304,114688,131072,163840,196608,229376,262144,327680,393216,458752
			};

			int table_size = 64 << HALF_TONE_BITS;
			Vector<int> table;
			table.resize_zeroed(table_size);

			// noise_phase_shift = ((1<<PHASE_BIT)  /  ((nf/clock)[sec]  /  (1/44100)[sec])) >> (PHASE_BIT - waveTable.fixedBits)
			double pitch_coef = PHASE_MAX * 1048576.0 / sampling_rate; // gb clock = 1048576

			for (int i = 0; i < 64; i++) {
				int value = pitch_coef / ref_values[i];

				for (int j = 0; j < HALF_TONE_RESOLUTION; j++) {
					table.write[(i << HALF_TONE_BITS) + j] = value;
				}
			}

			pitch_table.write[SiONPitchTableType::PITCH_TABLE_GB_NOISE] = table;
			phase_step_shift_filter[SiONPitchTableType::PITCH_TABLE_GB_NOISE] = 0xffffffff;
		}
	}

	// dt1 table.
	{
		// dt1 table from X68Sound.dll
		static const int ref_values[4][32] = {
			{ 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
			{ 0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  8,  8,  8,  8 },
			{ 1,  1,  1,  1,  2,  2,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  8,  8,  9, 10, 11, 12, 13, 14, 16, 16, 16, 16 },
			{ 2,  2,  2,  2,  2,  3,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  8,  8,  9, 10, 11, 12, 13, 14, 16, 17, 19, 20, 22, 22, 22, 22 }
		};

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < KEY_CODE_TABLE_SIZE; j++) {
				int value = ((int)(ref_values[i][j >> 2]) * 64 * clock_ratio) >> CLOCK_RATIO_BITS;

				dt1_table[i][j]     = value;
				dt1_table[i + 4][j] = -value;
			}
		}
	}

	// Log table.
	{
		{
			int table_start = (-ENV_TOP) << 3; // Start at -ENV_TOP.
			int table_step = table_start + LOG_TABLE_RESOLUTION * 2; // * 2 (positive & negative)
			int table_size = LOG_TABLE_SIZE;

			double pitch_delta = 1.0 / LOG_TABLE_RESOLUTION;
			double pitch_value = pitch_delta;

			for (int i = table_start; i < table_step; i += 2) {
				double value = Math::pow(2, LOG_VOLUME_BITS - pitch_value); // v=2^(LOG_VOLUME_BITS-1/256) at maximum (i=2)

				for (int j = i; j < table_size; j += LOG_TABLE_RESOLUTION * 2) {
					int reg_value = (int)value;

					log_table[j] = reg_value;
					log_table[j + 1] = -reg_value;
					value *= 0.5;
				}

				pitch_value += pitch_delta;
			}
		}

		// Saturation area.
		{
			int table_size = (-ENV_TOP) << 3;
			int value = log_table[table_size];

			for (int i = 0; i < table_size; i += 2) {
				log_table[i] = value;
				log_table[i + 1] = -value;
			}
		}

		// Zero fill area.
		{
			int table_size = LOG_TABLE_SIZE * 3;

			for (int i = LOG_TABLE_SIZE; i < table_size; i++) {
				log_table[i] = 0;
			}
		}
	}
}

void SiOPMRefTable::_create_wave_samples() {
	// Prepare tables.
	{
		int table_size = calculate_log_table_index(1);

		Vector<int> no_wave_table_wave;
		no_wave_table_wave.resize_zeroed(table_size);
		no_wave_table = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(no_wave_table_wave, SiONPitchTableType::PITCH_TABLE_PCM)));

		Vector<int> no_wave_table_opm_wave;
		no_wave_table_opm_wave.resize_zeroed(table_size);
		no_wave_table_opm = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(no_wave_table_opm_wave, SiONPitchTableType::PITCH_TABLE_OPM)));

		wave_tables.resize_zeroed(DEFAULT_PG_MAX);
		wave_tables.fill(no_wave_table);
		sampler_tables.resize_zeroed(SAMPLER_TABLE_MAX);
		for (int i = 0; i < SAMPLER_TABLE_MAX; i++) {
			Ref<SiOPMWaveSamplerTable> sampler = memnew(SiOPMWaveSamplerTable);
			sampler->clear();
			sampler_tables.write[i] = sampler;
		}

		_custom_wave_tables.resize_zeroed(WAVE_TABLE_MAX);
		_custom_wave_tables.fill(nullptr);
		_pcm_voices.resize_zeroed(PCM_DATA_MAX);
		_pcm_voices.fill(Ref<SiMMLVoice>());
	}

	// Sine wave tables.
	{
		int table_step = SAMPLING_TABLE_SIZE >> 1;
		int table_size = SAMPLING_TABLE_SIZE;

		Vector<int> table;
		table.resize_zeroed(table_size);

		double value_delta = 6.283185307179586 / table_size;
		double value_base = value_delta * 0.5;

		for (int i = 0; i < table_step; i++) {
			int value = calculate_log_table_index(Math::sin(value_base));

			table.write[i] = value;                  // positive
			table.write[i + table_step] = value + 1; // negative

			value_base += value_delta;
		}

		wave_tables.write[SiONPulseGeneratorType::PULSE_SINE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// Saw wave tables.
	{
		{
			int table_step = SAMPLING_TABLE_SIZE >> 1;
			int table_size = SAMPLING_TABLE_SIZE;

			Vector<int> table1;
			table1.resize_zeroed(table_size);
			Vector<int> table2;
			table2.resize_zeroed(table_size);

			double value_delta = 1.0 / table_step;
			double value_base = value_delta * 0.5;

			for (int i = 0; i < table_step; i++) {
				int value = calculate_log_table_index(value_base);

				table1.write[i]                  = value;     // positive
				table1.write[table_size - i - 1] = value + 1; // negative
				table2.write[table_step - i - 1] = value;     // positive
				table2.write[table_step + i]     = value + 1; // negative

				value_base += value_delta;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_SAW_UP]   = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table1)));
			wave_tables.write[SiONPulseGeneratorType::PULSE_SAW_DOWN] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table2)));
		}

		{
			int table_size = 32;

			Vector<int> table;
			table.resize_zeroed(table_size);

			double value_delta = 0.0625;
			double value_base = -0.96875;

			for (int i = 0; i < table_size; i++) {
				table.write[i] = calculate_log_table_index(value_base);

				value_base += value_delta;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_SAW_VC6] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}
	}

	// Triangle wave tables.
	{
		// Triangle wave.
		{
			int table_step = SAMPLING_TABLE_SIZE >> 2;
			int table_offset = SAMPLING_TABLE_SIZE >> 1;
			int table_size = SAMPLING_TABLE_SIZE;

			Vector<int> table;
			table.resize_zeroed(table_size);

			double value_delta = 1.0 / table_step;
			double value_base = value_delta * 0.5;

			for (int i = 0; i < table_step; i++) {
				int value = calculate_log_table_index(value_base);

				table.write[i]                    = value;     // positive
				table.write[table_offset - i - 1] = value;     // positive
				table.write[table_offset + i]     = value + 1; // negative
				table.write[table_size - i - 1]   = value + 1; // negative

				value_base += value_delta;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_TRIANGLE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}

		// FC triangle wave.
		{
			Vector<int> table;
			table.resize_zeroed(32);

			double value_delta = 0.125;
			double value_base = 0.125;

			table.write[0]  = LOG_TABLE_BOTTOM;
			table.write[15] = LOG_TABLE_BOTTOM;
			table.write[23] = 3;
			table.write[24] = 3;

			for (int i = 1; i < 8; i++) {
				int value = calculate_log_table_index(value_base);

				table.write[i]      = value;
				table.write[15 - i] = value;
				table.write[15 + i] = value + 1;
				table.write[32 - i] = value + 1;

				value_base += value_delta;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_TRIANGLE_FC] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}
	}

	// Square wave tables.
	{
		// 50% square wave.
		int value = calculate_log_table_index(SQUARE_WAVE_OUTPUT);
		Vector<int> table = { value, value + 1 };

		wave_tables.write[SiONPulseGeneratorType::PULSE_SQUARE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// Pulse wave tables.
	{
		Vector<int> base_table = wave_tables[SiONPulseGeneratorType::PULSE_SQUARE]->get_wavelet();

		// Pulse wave.
		// NOTE: The resolution of duty ratio is twice than pAPU. [pAPU pulse wave table] = waveTables[PULSE_PULSE+duty*2].
		{
			for (int j = 0; j < 16; j++) {
				Vector<int> table;
				table.resize_zeroed(16);

				for (int i = 0; i < 16; i++) {
					table.write[i] = (i < j ? base_table[0] : base_table[1]);
				}

				wave_tables.write[SiONPulseGeneratorType::PULSE_PULSE + j] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
			}
		}

		// Spike pulse.
		{
			int value = calculate_log_table_index(0);

			for (int j = 0; j < 16; j++) {
				Vector<int> table;
				table.resize_zeroed(32);

				int i = 0;
				int table_step = j << 1;
				for (; i < table_step; i++) {
					table.write[i] = (i < j ? base_table[0] : base_table[1]);
				}
				for (; i < 32; i++) {
					table.write[i] = value;
				}

				wave_tables.write[SiONPulseGeneratorType::PULSE_PULSE_SPIKE + j] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
			}
		}

	}

	// Konami bubble system wave tables.
	{
		static const int ref_table[32] = {
			-80,-112,-16,96,64,16,64,96,32,-16,64,112,80,0,32,48,
			-16,-96,0,80,16,-64,-48,-16,-96,-128,-80,0,-48,-112,-80,-32
		};

		Vector<int> table;
		table.resize_zeroed(32);

		for (int i = 0; i < 32; i++) {
			table.write[i] = calculate_log_table_index((double)ref_table[i] / 128.0);
		}

		wave_tables.write[SiONPulseGeneratorType::PULSE_KNM_BUBBLE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// Pseudo sync wave tables.
	{
		Vector<int> table1;
		table1.resize_zeroed(SAMPLING_TABLE_SIZE);
		Vector<int> table2;
		table2.resize_zeroed(SAMPLING_TABLE_SIZE);

		int table_step = SAMPLING_TABLE_SIZE;
		double value_delta = 1.0 / table_step;
		double value_base = value_delta * 0.5;

		for (int i = 0; i < table_step; i++) {
			int value = calculate_log_table_index(value_base);

			table1.write[i] = value + 1; // negative
			table2.write[i] = value;     // positive

			value_base += value_delta;
		}

		wave_tables.write[SiONPulseGeneratorType::PULSE_SYNC_LOW]  = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table1)));
		wave_tables.write[SiONPulseGeneratorType::PULSE_SYNC_HIGH] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table2)));
	}

	// Noise tables.
	{
		// White noise, pulse noise.
		// NOTE: Naive implementation. Details are shown in MAME or VirtuaNes source.
		{
			Vector<int> table1;
			table1.resize_zeroed(NOISE_TABLE_SIZE);
			Vector<int> table2;
			table2.resize_zeroed(NOISE_TABLE_SIZE);

			int table_step = NOISE_TABLE_SIZE;

			double value_coef = NOISE_WAVE_OUTPUT / 32768.0;
			int value_base = 1; // 15bit LFSR
			int value = calculate_log_table_index(NOISE_WAVE_OUTPUT);

			for (int i = 0; i < table_step; i++) {
				value_base = (((value_base << 13) ^ (value_base << 14)) & 0x4000) | (value_base >> 1);

				table1.write[i] = calculate_log_table_index((value_base & 0x7fff) * value_coef * 2 - 1);
				table2.write[i] = (value_base & 1 ? value : value + 1);
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE_WHITE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table1, SiONPitchTableType::PITCH_TABLE_PCM)));
			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE_PULSE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table2, SiONPitchTableType::PITCH_TABLE_PCM)));
			wave_tables.write[SiONPulseGeneratorType::PULSE_PC_NZ_OPM]   = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table2, SiONPitchTableType::PITCH_TABLE_OPM_NOISE)));
			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE] = wave_tables[SiONPulseGeneratorType::PULSE_NOISE_WHITE];
		}

		// FC short noise.
		//NOTE: Naive implementation. 93*11=1023 approx.-> 1024.
		{
			Vector<int> table;
			table.resize_zeroed(SAMPLING_TABLE_SIZE);

			int table_step = SAMPLING_TABLE_SIZE;
			int value_base = 1; // 15bit LFSR
			int value = calculate_log_table_index(NOISE_WAVE_OUTPUT);

			for (int i = 0; i < table_step; i++) {
				value_base = (((value_base << 8) ^ (value_base << 14)) & 0x4000) | (value_base >> 1);

				table.write[i] = (value_base & 1 ? value : value + 1);
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE_SHORT] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table, SiONPitchTableType::PITCH_TABLE_PCM)));
		}

		// GB short noise.
		{
			Vector<int> table;
			table.resize_zeroed(128);

			int value_base = 0xffff; // 16bit LFSR
			int value_offset = 0;
			int value = calculate_log_table_index(NOISE_WAVE_OUTPUT);

			for (int i = 0; i < 128; i++) {
				value_base += value_base + (((value_base >> 6) ^ (value_base >> 5)) & 1);
				value_offset ^= value_base & 1;

				table.write[i] = (value_offset & 1 ? value : value + 1);
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE_GB_SHORT] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table, SiONPitchTableType::PITCH_TABLE_PCM)));
		}

		// Periodic noise.
		{
			Vector<int> table;
			table.resize_zeroed(16);

			table.write[0] = calculate_log_table_index(SQUARE_WAVE_OUTPUT);
			for (int i = 1; i < 16; i++) {
				table.write[i] = LOG_TABLE_BOTTOM;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_PC_NZ_16BIT] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}

		// High-passed white noise.
		{
			Vector<int> base_table = wave_tables[SiONPulseGeneratorType::PULSE_NOISE_WHITE]->get_wavelet();
			Vector<int> table;
			table.resize_zeroed(NOISE_TABLE_SIZE);

			int table_step = NOISE_TABLE_SIZE;

			int value_offset = (-ENV_TOP) << 3;
			double value_coef = 16.0 / (1 << LOG_VOLUME_BITS);
			int log_value1 = base_table[0] + value_offset;
			int log_value2 = base_table[NOISE_TABLE_SIZE - 1] + value_offset;
			double value = (log_table[log_value1] - log_table[log_value2]) * 0.0625;

			table.write[0] = calculate_log_table_index(value * value_coef);
			for (int i = 1; i < table_step; i++) {
				log_value1 = base_table[i] + value_offset;
				log_value2 = base_table[i - 1] + value_offset;
				value = (value + log_table[log_value1] - log_table[log_value2]) * 0.0625;

				table.write[i] = calculate_log_table_index(value * value_coef);
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE_HIPASS] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table, SiONPitchTableType::PITCH_TABLE_PCM)));
		}

		// Pink noise.
		{
			Vector<int> base_table = wave_tables[SiONPulseGeneratorType::PULSE_NOISE_WHITE]->get_wavelet();
			Vector<int> table;
			table.resize_zeroed(NOISE_TABLE_SIZE);

			int table_step = NOISE_TABLE_SIZE;

			int value_offset = (-ENV_TOP) << 3;
			double value_coef = 0.125 / (1 << LOG_VOLUME_BITS);
			double b0 = 0;
			double b1 = 0;
			double b2 = 0;

			for (int i = 0; i < table_step; i++) {
				int log_value = base_table[i] + value_offset;
				int value_base = log_table[log_value];

				b0 = 0.99765 * b0 + value_base * 0.0990460;
				b1 = 0.96300 * b1 + value_base * 0.2965164;
				b2 = 0.57000 * b2 + value_base * 1.0526913;

				table.write[i] = calculate_log_table_index((b0 + b1 + b2 + value_base * 0.1848) * value_coef);
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_NOISE_PINK] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table, SiONPitchTableType::PITCH_TABLE_PCM)));
		}

		// Pitch-controllable noise.
		{
			Vector<int> base_table = wave_tables[SiONPulseGeneratorType::PULSE_NOISE_SHORT]->get_wavelet();
			Vector<int> table;
			table.resize_zeroed(SAMPLING_TABLE_SIZE);

			for (int j = 0; j < SAMPLING_TABLE_SIZE; j++) {
				int i = j * 11;
				int table_step = MIN((i + 11), SAMPLING_TABLE_SIZE);

				for (; i < table_step; i++) {
					table.write[i] = base_table[j];
				}
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_PC_NZ_SHORT] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}
	}

	// Ramp wave tables.
	{
		int table_size = SAMPLING_TABLE_SIZE;
		int table_offset = SAMPLING_TABLE_SIZE >> 1;
		int table_shift = SAMPLING_TABLE_SIZE >> 2;

		int prev = 0;
		for (int j = 1; j < 60; j++) {
			int curr = table_shift >> (j >> 3);
			curr -= (curr * (j & 7)) >> 4;

			if (prev == curr) {
				wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + 64 - j] = wave_tables[SiONPulseGeneratorType::PULSE_RAMP + 65 - j];
				wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + 64 + j] = wave_tables[SiONPulseGeneratorType::PULSE_RAMP + 63 + j];
				continue;
			}
			prev = curr;

			Vector<int> table1;
			table1.resize_zeroed(SAMPLING_TABLE_SIZE);
			Vector<int> table2;
			table2.resize_zeroed(SAMPLING_TABLE_SIZE);

			int table_step = table_offset - curr;

			double value_delta = 1.0 / table_step;
			double value_base = value_delta * 0.5;

			int i = 0;
			for (; i < table_step; i++) {
				int value = calculate_log_table_index(value_base);

				table1.write[i]                    = value;     // positive
				table1.write[table_size - i - 1]   = value + 1; // negative
				table2.write[table_offset + i]     = value + 1; // negative
				table2.write[table_offset - i - 1] = value;     // positive

				value_base += value_delta;
			}

			value_delta = 1.0 / (table_offset - table_step);

			for (; i < table_offset; i++) {
				int value = calculate_log_table_index(value_base);

				table1.write[i]                    = value;     // positive
				table1.write[table_size - i - 1]   = value + 1; // negative
				table2.write[table_offset + i]     = value + 1; // negative
				table2.write[table_offset - i - 1] = value;     // positive

				value_base -= value_delta;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + 64 - j] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table1)));
			wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + 64 + j] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table2)));
		}

		for (int j = 0; j < 5; j++) {
			wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + j] = wave_tables[SiONPulseGeneratorType::PULSE_SAW_UP];
		}
		for (int j = 124; j < 128; j++) {
			wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + j] = wave_tables[SiONPulseGeneratorType::PULSE_SAW_DOWN];
		}

		wave_tables.write[SiONPulseGeneratorType::PULSE_RAMP + 64] = wave_tables[SiONPulseGeneratorType::PULSE_TRIANGLE];
	}

	// MA3 wave tables.
	{
		// 0-5 - sine waves.
		_create_ma3_waveset(SiONPulseGeneratorType::PULSE_MA3_SINE, wave_tables[SiONPulseGeneratorType::PULSE_SINE]);

		// 6 - square wave.
		wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_SQUARE] = wave_tables[SiONPulseGeneratorType::PULSE_SQUARE];

		// 7 - downwards saw wave with sine flattening. Best name I can come up with, not sure if there is a more common description.
		{
			Vector<int> table1;
			table1.resize_zeroed(SAMPLING_TABLE_SIZE);

			int table_step = SAMPLING_TABLE_SIZE >> 2;
			int table_offset = SAMPLING_TABLE_SIZE >> 1;
			int table_size = SAMPLING_TABLE_SIZE;

			double value_delta = 6.283185307179586 / SAMPLING_TABLE_SIZE;
			double value_base = value_delta * 0.5;

			for (int i = 0; i < table_step; i++) {
				int value = calculate_log_table_index(1 - Math::sin(value_base));

				table1.write[i]                  = value;     // positive
				table1.write[i + table_step]     = LOG_TABLE_BOTTOM;
				table1.write[i + table_offset]   = LOG_TABLE_BOTTOM;
				table1.write[table_size - i - 1] = value + 1; // negative

				value_base += value_delta;
			}

			wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_SAW_SINE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table1)));
		}

		// 8-13 - triangle modulated sine
		{
			Vector<int> base_table = wave_tables[SiONPulseGeneratorType::PULSE_SINE]->get_wavelet();
			Vector<int> table;
			table.resize_zeroed(SAMPLING_TABLE_SIZE);

			int j = 0;
			for (int i = 0; i<SAMPLING_TABLE_SIZE; i++) {
				table.write[i] = base_table[i + j];
				j += 1 - (((i >> (SAMPLING_TABLE_BITS - 3)) + 1) & 2); // triangle wave
			}

			_create_ma3_waveset(SiONPulseGeneratorType::PULSE_MA3_TRI_SINE, Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table))));
		}

		// 14 - half square
		{
			int value = calculate_log_table_index(1);
			Vector<int> table = { value, LOG_TABLE_BOTTOM };

			wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_SQUARE_HALF] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}

		// 16-21 - triangle waves
		_create_ma3_waveset(SiONPulseGeneratorType::PULSE_MA3_TRI, wave_tables[SiONPulseGeneratorType::PULSE_TRIANGLE]);

		// 22 - quarter square doubled.
		{
			int value = calculate_log_table_index(1);
			Vector<int> table = { value, LOG_TABLE_BOTTOM, value, LOG_TABLE_BOTTOM };

			wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_SQUARE_QUART_DOUBLE] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}

		// 24-29 - upwards saw waves.
		_create_ma3_waveset(SiONPulseGeneratorType::PULSE_MA3_SAW, wave_tables[SiONPulseGeneratorType::PULSE_SAW_UP]);

		// 30 - quarter square wave.
		{
			int value = calculate_log_table_index(1);
			Vector<int> table = { value, LOG_TABLE_BOTTOM, LOG_TABLE_BOTTOM, LOG_TABLE_BOTTOM };

			wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_SQUARE_QUART] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
		}

		// 15,23,31 - user defined waves.
		wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_USER1] = no_wave_table;
		wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_USER2] = no_wave_table;
		wave_tables.write[SiONPulseGeneratorType::PULSE_MA3_USER3] = no_wave_table;
	}
}

void SiOPMRefTable::_create_ma3_waveset(int p_index, const Ref<SiOPMWaveTable> &p_table) {
	// MA-3 waveforms contain 4 sets with the same basic premise. We take the base one,
	// then modify it via the same transforms to get 5 variations.

	// 0 - Full wave.
	wave_tables.write[p_index] = p_table;
	Vector<int> basic_waveform = p_table->get_wavelet();

	// 1 - Half wave.
	{
		Vector<int> table;
		table.resize_zeroed(SAMPLING_TABLE_SIZE);
		int table_offset = SAMPLING_TABLE_SIZE >> 1;
		for (int i = 0; i < table_offset; i++) {
			table.write[i]                = basic_waveform[i];
			table.write[i + table_offset] = LOG_TABLE_BOTTOM;
		}
		wave_tables.write[p_index + 1] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// 2 - Half wave doubled.
	{
		Vector<int> table;
		table.resize_zeroed(SAMPLING_TABLE_SIZE);
		int table_offset = SAMPLING_TABLE_SIZE >> 1;
		for (int i = 0; i < table_offset; i++) {
			table.write[i]                = basic_waveform[i];
			table.write[i + table_offset] = basic_waveform[i];
		}
		wave_tables.write[p_index + 2] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// 3 - Quarter wave doubled.
	{
		Vector<int> table;
		table.resize_zeroed(SAMPLING_TABLE_SIZE);
		int table_offset = SAMPLING_TABLE_SIZE >> 2;
		for (int i = 0; i < table_offset; i++) {
			table.write[i]                    = basic_waveform[i];
			table.write[i + table_offset]     = LOG_TABLE_BOTTOM;
			table.write[i + table_offset * 2] = basic_waveform[i];
			table.write[i + table_offset * 3] = LOG_TABLE_BOTTOM;
		}
		wave_tables.write[p_index + 3] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// 4 - Sped up wave.
	{
		Vector<int> table;
		table.resize_zeroed(SAMPLING_TABLE_SIZE);
		int table_offset = SAMPLING_TABLE_SIZE >> 1;
		for (int i = 0; i < table_offset; i++) {
			table.write[i]                = basic_waveform[i << 1];
			table.write[i + table_offset] = LOG_TABLE_BOTTOM;
		}
		wave_tables.write[p_index + 4] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}

	// 5 - Sped up half wave doubled.
	{
		Vector<int> table;
		table.resize_zeroed(SAMPLING_TABLE_SIZE);
		int table_offset = SAMPLING_TABLE_SIZE >> 2;
		for (int i = 0; i < table_offset; i++) {
			table.write[i]                  = basic_waveform[i << 1];
			table.write[i + table_offset]   = table[i];
			table.write[i + table_offset * 2] = LOG_TABLE_BOTTOM;
			table.write[i + table_offset * 3] = LOG_TABLE_BOTTOM;
		}
		wave_tables.write[p_index + 5] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(table)));
	}
}

void SiOPMRefTable::_create_lfo_tables() {
	// Timer steps.
	// This calculation is hybrid between fmgen and x68sound.dll, and extend as 20bit fixed dicimal.
	for (int i = 0; i < LFO_TABLE_SIZE; i++) {
		int t = 16 + (i & 15);  // linear interpolation for 4LSBs
		int s = 15 - (i >> 4);  // log-scale shift for 4HSBs
		lfo_timer_steps[i] = ((t << (LFO_FIXED_BITS - 4)) * clock_ratio / (8 << s)) >> CLOCK_RATIO_BITS; // 4 from fmgen, 8 from x68sound.
	}

	// Wave tables.
	{
		// Saw wave.
		for (int i = 0; i < LFO_TABLE_SIZE; i++) {
			lfo_wave_tables[LFO_WAVE_SAW][i] = 255 - i;
			lfo_wave_tables[LFO_WAVE_SAW + 4][i] = i;
		}

		// Pulse wave.
		for (int i = 0; i < LFO_TABLE_SIZE; i++) {
			int value = (i < 128 ? 255 : 0);
			lfo_wave_tables[LFO_WAVE_SQUARE][i] = value;
			lfo_wave_tables[LFO_WAVE_SQUARE + 4][i] = 255 - value;
		}

		// Triangle wave.
		for (int i = 0; i < 64; i++) { // Quarter of LFO_TABLE_SIZE.
			int t = i << 1;
			lfo_wave_tables[LFO_WAVE_TRIANGLE][i]     = t + 128;
			lfo_wave_tables[LFO_WAVE_TRIANGLE][127 - i] = t + 128;
			lfo_wave_tables[LFO_WAVE_TRIANGLE][128 + i] = 126 - t;
			lfo_wave_tables[LFO_WAVE_TRIANGLE][255 - i] = 126 - t;
		}
		for (int i = 0; i < LFO_TABLE_SIZE; i++) {
			lfo_wave_tables[LFO_WAVE_TRIANGLE + 4][i] = 255 - lfo_wave_tables[LFO_WAVE_TRIANGLE][i];
		}

		// Noise wave.
		Ref<RandomNumberGenerator> rng;
		rng.instantiate();
		for (int i = 0; i < LFO_TABLE_SIZE; i++) {
			int value = rng->randi_range(0, 255);
			lfo_wave_tables[LFO_WAVE_NOISE][i] = value;
			lfo_wave_tables[LFO_WAVE_NOISE + 4][i] = 255 - value;
		}
	}

	// Chorus tables.
	for (int i = 0; i < LFO_TABLE_SIZE; i++) {
		lfo_chorus_tables[i] = (i - 128) * (i - 128);
	}
}

void SiOPMRefTable::_create_filter_tables() {
	for (int i = 0; i < 128; i++) {
		filter_cutoff_table[i] = i * i * 0.00006103515625; // 0.00006103515625 = 1/(128*128)
		filter_feedback_table[i] = 1.0 + 1.0 / (1.0 - filter_cutoff_table[i]); // ???
	}
	filter_cutoff_table[128] = 1;
	filter_feedback_table[128] = filter_cutoff_table[128]; // Original code assigns the value to itself here. Probably meant this instead.

	filter_eg_rate[0] = 0;
	for (int i = 1; i < 60; i++) {
		double shift = (double)(1 << (14 - (i >> 2)));
		double liner = (double)((i & 3) * 0.125 + 0.5);
		filter_eg_rate[i] = (int)(2.36514 * shift * liner + 0.5); // 2.36514 = 3 / ((fm_clock/64)/sampling_rate)
	}
	for (int i = 60; i < 64; i++) {
		filter_eg_rate[i] = 1;
	}
}

SiOPMRefTable::SiOPMRefTable(int p_fm_clock, double p_psg_clock, int p_sampling_rate) {
	if (!_instance) {
		_instance = this; // Do this early so it can be self-referenced.
	}

	_set_constants(p_fm_clock, p_psg_clock, p_sampling_rate);

	_create_eg_tables();
	_create_pg_tables();
	_create_wave_samples();
	_create_lfo_tables();
	_create_filter_tables();
}

SiOPMRefTable::~SiOPMRefTable() {
	wave_tables.clear();
	sampler_tables.clear();
	_custom_wave_tables.clear();
	_stencil_custom_wave_tables.clear();

	_pcm_voices.clear();
	_stencil_pcm_voices.clear();
}

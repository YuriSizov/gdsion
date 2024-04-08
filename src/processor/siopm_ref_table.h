/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_REF_TABLE_H
#define SIOPM_REF_TABLE_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
#include "sion_enums.h"
#include "sequencer/simml_voice.h"

using namespace godot;

class SiOPMWavePCMTable;
class SiOPMWaveSamplerData;
class SiOPMWaveSamplerTable;
class SiOPMWaveTable;

// Reference data object for the processor and related operations.
class SiOPMRefTable {

	static SiOPMRefTable *_instance;

	// Wave samples.

	// Custom wave tables.
	Vector<SiOPMWaveTable *> _custom_wave_tables;
	// Overriding custom wave tables.
	Vector<SiOPMWaveTable *> _stencil_custom_wave_tables;
	// PCM voices.
	Vector<Ref<SiMMLVoice>> _pcm_voices;
	// Overriding PCM voices.
	Vector<Ref<SiMMLVoice>> _stencil_pcm_voices;

	//

	void _set_constants(int p_fm_clock, double p_psg_clock, int p_sampling_rate);

	void _create_eg_tables();
	void _create_pg_tables();
	void _create_wave_samples();
	void _expand_ma3_waves(int p_index);
	void _create_lfo_tables();
	void _create_filter_tables();

public:
	static SiOPMRefTable *get_instance() { return _instance; }
	static void initialize();
	static void finalize() {}

	static int calculate_log_table_index(double p_number);

	//

	static const int ENV_BITS             = 10;   // Envelope output bit size.
	static const int ENV_TIMER_BITS       = 24;   // Envelope timer resolution bit size.
	static const int SAMPLING_TABLE_BITS  = 10;   // Sine wave table entries = 2 ^ SAMPLING_TABLE_BITS = 1024
	static const int HALF_TONE_BITS       = 6;    // Half tone resolution    = 2 ^ HALF_TONE_BITS      = 64
	static const int NOTE_BITS            = 7;    // Max note value          = 2 ^ NOTE_BITS           = 128
	static const int NOISE_TABLE_BITS     = 15;   // 32k noise
	static const int LOG_TABLE_RESOLUTION = 256;  // Log table resolution    = LOG_TABLE_RESOLUTION for every 1/2 scaling.
	static const int LOG_VOLUME_BITS      = 13;   // _logTable[0] = 2^13 at maximum
	static const int LOG_TABLE_MAX_BITS   = 16;   // _logTable entries
	static const int FIXED_BITS           = 16;   // Internal fixed point 16.16
	static const int PCM_BITS             = 20;   // Maximum PCM sample length = 2 ^ PCM_BITS = 1048576
	static const int LFO_FIXED_BITS       = 20;   // Fixed point for lfo timer
	static const int CLOCK_RATIO_BITS     = 10;   // Bits for clock/64/[sampling rate]

	static const double NOISE_WAVE_OUTPUT;     // -2044 < [noise amplitude] < 2040 -> NOISE_WAVE_OUTPUT=0.25
	static const double SQUARE_WAVE_OUTPUT;    //
	static const double OUTPUT_MAX;            // Maximum output

	static const int ENV_LSHIFT           = ENV_BITS - 7;                     // Shift number from input tl [0,127] to internal value [0,ENV_BOTTOM].
	static const int ENV_TIMER_INITIAL    = (2047 * 3) << CLOCK_RATIO_BITS;   // Envelope timer initial value.
	static const int LFO_TIMER_INITIAL    = 1 << LFO_FIXED_BITS;              // LFO timer initial value.
	static const int PHASE_BITS           = SAMPLING_TABLE_BITS + FIXED_BITS; // Internal phase is expressed by 10.16 fixed.
	static const int PHASE_MAX            = 1 << PHASE_BITS;
	static const int PHASE_FILTER         = PHASE_MAX - 1;
	static const int PHASE_SIGN_RSHIFT    = PHASE_BITS - 1;
	static const int SAMPLING_TABLE_SIZE  = 1 << SAMPLING_TABLE_BITS;
	static const int NOISE_TABLE_SIZE     = 1 << NOISE_TABLE_BITS;
	static const int PITCH_TABLE_SIZE     = 1 << (HALF_TONE_BITS + NOTE_BITS);
	static const int NOTE_TABLE_SIZE      = 1 << NOTE_BITS;
	static const int HALF_TONE_RESOLUTION = 1 << HALF_TONE_BITS;
	static const int LOG_TABLE_SIZE       = LOG_TABLE_MAX_BITS * LOG_TABLE_RESOLUTION * 2;   // *2 posi&nega
	static const int LFO_TABLE_SIZE       = 256;                                             // FIXED VALUE !!
	static const int TL_TABLE_SIZE        = 513;
	static const int KEY_CODE_TABLE_SIZE  = 128;                                             // FIXED VALUE !!
	static const int LOG_TABLE_BOTTOM     = LOG_VOLUME_BITS * LOG_TABLE_RESOLUTION * 2;      // Bottom value of log table = 6656
	static const int ENV_BOTTOM           = (LOG_VOLUME_BITS * LOG_TABLE_RESOLUTION) >> 2;   // Minimum gain of envelope = 832
	static const int ENV_TOP              = ENV_BOTTOM - (1<<ENV_BITS);                      // Maximum gain of envelope = -192
	static const int ENV_BOTTOM_SSGEC     = 1<<(ENV_BITS-3);                                 // Minimum gain of ssgec envelope = 128

	// TODO: Maybe use better names to explain what these constants hold, not what they are used for.
	// Also it's a bit awkward that MAX is overlapping with another value.
	static const int DEFAULT_PG_MAX = 256;   // Maximum value of default pulse generator types.
	static const int PG_FILTER      = 511;   // Maximum index from predefined pulse generator type values.

	static const int WAVE_TABLE_MAX    = 128;                // Custom wave table max.
	static const int PCM_DATA_MAX      = 128;                // PCM data max.
	static const int SAMPLER_TABLE_MAX = 4;                  // Sampler table max
	static const int SAMPLER_DATA_MAX  = NOTE_TABLE_SIZE;    // Sampler data max

	enum VelocityMode {
		VM_LINEAR = 0,  // linear scale
		VM_DR96DB = 1,  // log scale; dynamic range = 96dB; total level based.
		VM_DR64DB = 2,  // log scale; dynamic range = 64dB; fmp7 based.
		VM_DR48DB = 3,  // log scale; dynamic range = 48dB; PSG volume based.
		VM_DR32DB = 4,  // log scale; dynamic range = 32dB; based on N88 basic v command.
		VM_MAX = 5
	};

	enum LFOWaveShape {
		LFO_WAVE_SAW      = 0,
		LFO_WAVE_SQUARE   = 1,
		LFO_WAVE_TRIANGLE = 2,
		LFO_WAVE_NOISE    = 3,
		LFO_WAVE_MAX      = 8 // Values 4-7 are pairs for the first 0-3.
	};

	// All reference properties are made public to simplify code.
	// These are not expected to be written to externally. But that might happen.

	int sampling_rate = 0;
	int fm_clock = 0;
	double psg_clock = 0;
	// (fm_clock/64/sampling_rate) << CLOCK_RATIO_BITS
	int clock_ratio = 1;
	// 44100Hz=0, 22050Hz=1
	int sample_rate_pitch_shift = 0;

	// int->double ratio on pulse data
	double i2n = OUTPUT_MAX/(double)(1 << LOG_VOLUME_BITS);

	// Envelope generator.

	// EG increment table. This table is based on MAME's opm emulation.
	int eg_increment_tables[18][8] = {
		/*cycle:      0 1  2 3  4 5  6 7  */
		/* 0*/      { 0,1, 0,1, 0,1, 0,1 },  /* rates 00..11 0 (increment by 0 or 1) */
		/* 1*/      { 0,1, 0,1, 1,1, 0,1 },  /* rates 00..11 1 */
		/* 2*/      { 0,1, 1,1, 0,1, 1,1 },  /* rates 00..11 2 */
		/* 3*/      { 0,1, 1,1, 1,1, 1,1 },  /* rates 00..11 3 */
		/* 4*/      { 1,1, 1,1, 1,1, 1,1 },  /* rate 12 0 (increment by 1) */
		/* 5*/      { 1,1, 1,2, 1,1, 1,2 },  /* rate 12 1 */
		/* 6*/      { 1,2, 1,2, 1,2, 1,2 },  /* rate 12 2 */
		/* 7*/      { 1,2, 2,2, 1,2, 2,2 },  /* rate 12 3 */
		/* 8*/      { 2,2, 2,2, 2,2, 2,2 },  /* rate 13 0 (increment by 2) */
		/* 9*/      { 2,2, 2,4, 2,2, 2,4 },  /* rate 13 1 */
		/*10*/      { 2,4, 2,4, 2,4, 2,4 },  /* rate 13 2 */
		/*11*/      { 2,4, 4,4, 2,4, 4,4 },  /* rate 13 3 */
		/*12*/      { 4,4, 4,4, 4,4, 4,4 },  /* rate 14 0 (increment by 4) */
		/*13*/      { 4,4, 4,8, 4,4, 4,8 },  /* rate 14 1 */
		/*14*/      { 4,8, 4,8, 4,8, 4,8 },  /* rate 14 2 */
		/*15*/      { 4,8, 8,8, 4,8, 8,8 },  /* rate 14 3 */
		/*16*/      { 8,8, 8,8, 8,8, 8,8 },  /* rates 15 0, 15 1, 15 2, 15 3 (increment by 8) */
		/*17*/      { 0,0, 0,0, 0,0, 0,0 }   /* infinity rates for attack and decay(s) */
	};
	// EG increment table for attack. This table is based on fmgen (shift=0 means x0).
	int eg_increment_tables_attack[18][8] = {
		/*cycle:      0 1  2 3  4 5  6 7  */
		/* 0*/      { 0,4, 0,4, 0,4, 0,4 },  /* rates 00..11 0 (increment by 0 or 1) */
		/* 1*/      { 0,4, 0,4, 4,4, 0,4 },  /* rates 00..11 1 */
		/* 2*/      { 0,4, 4,4, 0,4, 4,4 },  /* rates 00..11 2 */
		/* 3*/      { 0,4, 4,4, 4,4, 4,4 },  /* rates 00..11 3 */
		/* 4*/      { 4,4, 4,4, 4,4, 4,4 },  /* rate 12 0 (increment by 1) */
		/* 5*/      { 4,4, 4,3, 4,4, 4,3 },  /* rate 12 1 */
		/* 6*/      { 4,3, 4,3, 4,3, 4,3 },  /* rate 12 2 */
		/* 7*/      { 4,3, 3,3, 4,3, 3,3 },  /* rate 12 3 */
		/* 8*/      { 3,3, 3,3, 3,3, 3,3 },  /* rate 13 0 (increment by 2) */
		/* 9*/      { 3,3, 3,2, 3,3, 3,2 },  /* rate 13 1 */
		/*10*/      { 3,2, 3,2, 3,2, 3,2 },  /* rate 13 2 */
		/*11*/      { 3,2, 2,2, 3,2, 2,2 },  /* rate 13 3 */
		/*12*/      { 2,2, 2,2, 2,2, 2,2 },  /* rate 14 0 (increment by 4) */
		/*13*/      { 2,2, 2,1, 2,2, 2,1 },  /* rate 14 1 */
		/*14*/      { 2,8, 2,1, 2,1, 2,1 },  /* rate 14 2 */
		/*15*/      { 2,1, 1,1, 2,1, 1,1 },  /* rate 14 3 */
		/*16*/      { 1,1, 1,1, 1,1, 1,1 },  /* rates 15 0, 15 1, 15 2, 15 3 (increment by 8) */
		/*17*/      { 0,0, 0,0, 0,0, 0,0 }   /* infinity rates for attack and decay(s) */
	};
	// EG table selector.
	int eg_table_selector[128]; // 128 = 64 rates + 32 ks-rates + 32 dummies for dr,sr=0
	// EG timer step.
	int eg_timer_steps[128]; // 128 = 64 rates + 32 ks-rates + 32 dummies for dr,sr=0
	// EG table to calculate EG level tables.
	int eg_level_tables[7][1 << ENV_BITS];
	// EG table from sgg_type to eg_level_tables index.
	int eg_ssg_table_index[10][2][3] = {
		// [w/ ar], [w/o ar]
		{  {3,3,3}, {1,3,3}  },   // ssgec=8
		{  {1,6,6}, {1,6,6}  },   // ssgec=9
		{  {2,1,2}, {1,2,1}  },   // ssgec=10
		{  {2,5,5}, {1,5,5}  },   // ssgec=11
		{  {4,4,4}, {2,4,4}  },   // ssgec=12
		{  {2,5,5}, {2,5,5}  },   // ssgec=13
		{  {1,2,1}, {2,1,2}  },   // ssgec=14
		{  {1,6,6}, {2,6,6}  },   // ssgec=15
		{  {1,1,1}, {1,1,1}  },   // ssgec=8+
		{  {2,2,2}, {2,2,2}  }   // ssgec=12+
	};
	// EG sustain level table from 15 to 1024.
	int eg_sustain_level_table[16];
	// EG total level table from volume to tl.
	int eg_total_level_tables[VM_MAX][TL_TABLE_SIZE];
	// EG conversion table from linear volume to total level.
	int eg_linear_to_total_level_table[129];

	// Panning volume table.
	double pan_table[129];

	// Low frequency oscillator.

	// LFO timer step.
	int lfo_timer_steps[LFO_TABLE_SIZE];
	// LFO modulation table.
	int lfo_wave_tables[LFO_WAVE_MAX][LFO_TABLE_SIZE];
	// LFO modulation table for chorus.
	int lfo_chorus_tables[LFO_TABLE_SIZE];

	// Filter.

	// FILTER cutoff.
	double filter_cutoff_table[129];
	// FILTER resonance.
	double filter_feedback_table[129];
	// FILTER envlope rate.
	int filter_eg_rate[64];

	// Pulse generator.

	// PG MIDI note number to FM key code.
	int note_number_to_key_code[NOTE_TABLE_SIZE];

	// PG pitch table.
	Vector<Vector<int>> pitch_table;
	// PG pitch wave length (in samples) table.
	double pitch_wave_length[PITCH_TABLE_SIZE];
	// PG phase step shift filter.
	int phase_step_shift_filter[PT_MAX];
	// PG sound reference table.
	HashMap<String, Variant> sound_reference;

	// Table for dt1 (from fmgen.cpp).
	int dt1_table[8][KEY_CODE_TABLE_SIZE];
	// Table for dt2 (from MAME's opm source).
	int dt2_table[4] = { 0, 384, 500, 608 };
	// PG log table.
	int log_table[LOG_TABLE_SIZE * 3]; // 2 extra units of size for zero-filling; 16*256*2*3 = 24576

	// Wave samples.

	// PG wave tables.
	Vector<SiOPMWaveTable *> wave_tables;
	// PG wave tables without any waves.
	SiOPMWaveTable *no_wave_table = nullptr;
	SiOPMWaveTable *no_wave_table_opm = nullptr;
	// PG sampler table.
	Vector<SiOPMWaveSamplerTable *> sampler_tables;

	void reset_all_user_tables();
	void register_wave_table(int p_index, SiOPMWaveTable *p_table);
	SiOPMWaveSamplerData *register_sampler_data(int p_index, const Variant &p_data, bool p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count);

	SiOPMWaveTable *get_wave_table(int p_index);
	SiOPMWavePCMTable *get_pcm_data(int p_index);

	Ref<SiMMLVoice> get_global_pcm_voice(int p_index);
	Ref<SiMMLVoice> set_global_pcm_voice(int p_index, const Ref<SiMMLVoice> &p_from_voice);

	void set_stencil_custom_wave_tables(Vector<SiOPMWaveTable *> p_tables) { _stencil_custom_wave_tables = p_tables; }
	void set_stencil_pcm_voices(Vector<Ref<SiMMLVoice>> p_tables) { _stencil_pcm_voices = p_tables; }

	//

	// TODO: Define parameters as constants?
	SiOPMRefTable(int p_fm_clock = 3580000, double p_psg_clock = 1789772.5, int p_sampling_rate = 44100);
	~SiOPMRefTable() {}
};

#endif // SIOPM_REF_TABLE_H

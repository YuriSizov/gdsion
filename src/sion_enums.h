/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_ENUMS_H
#define SION_ENUMS_H

#include <godot_cpp/core/binder_common.hpp>

enum SiONModuleType : unsigned int {
	MT_PSG     = 0,  // PSG(DCSG)
	MT_APU     = 1,  // FC pAPU
	MT_NOISE   = 2,  // noise wave
	MT_MA3     = 3,  // MA3 wave form
	MT_CUSTOM  = 4,  // SCC / custom wave table
	MT_ALL     = 5,  // all pgTypes
	MT_FM      = 6,  // FM sound module
	MT_PCM     = 7,  // PCM
	MT_PULSE   = 8,  // pulse wave
	MT_RAMP    = 9,  // ramp wave
	MT_SAMPLE  = 10, // sampler
	MT_KS      = 11, // karplus strong
	MT_GB      = 12, // gameboy
	MT_VRC6    = 13, // vrc6
	MT_SID     = 14, // sid
	MT_FM_OPM  = 15, // YM2151
	MT_FM_OPN  = 16, // YM2203
	MT_FM_OPNA = 17, // YM2608
	MT_FM_OPLL = 18, // YM2413
	MT_FM_OPL3 = 19, // YM3812
	MT_FM_MA3  = 20, // YMU762
	MT_MAX     = 21
};

enum SiONPitchTableType : unsigned int {
	PT_OPM = 0,
	PT_PCM = 1,
	PT_PSG = 2,
	PT_OPM_NOISE = 3,
	PT_PSG_NOISE = 4,
	PT_APU_NOISE = 5,
	PT_GB_NOISE = 6,
	PT_MAX = 7
};

enum SiONPulseGeneratorType : int {
	PG_SINE				= 0,    // sine wave
	PG_SAW_UP			= 1,    // upward saw wave
	PG_SAW_DOWN			= 2,    // downward saw wave
	PG_TRIANGLE_FC		= 3,    // triangle wave quantized by 4bit
	PG_TRIANGLE			= 4,    // triangle wave
	PG_SQUARE			= 5,    // square wave
	PG_NOISE			= 6,    // 32k white noise
	PG_KNMBSMM			= 7,    // knmbsmm wave
	PG_SYNC_LOW			= 8,    // pseudo sync (low freq.)
	PG_SYNC_HIGH		= 9,    // pseudo sync (high freq.)
	PG_OFFSET			= 10,   // offset
	PG_SAW_VC6			= 11,   // vc6 saw (32 samples saw)
								// ( 12- 15) reserved
	PG_NOISE_WHITE		= 16,   // 16k white noise
	PG_NOISE_PULSE		= 17,   // 16k pulse noise
	PG_NOISE_SHORT		= 18,   // fc short noise
	PG_NOISE_HIPAS		= 19,   // high pass noise
	PG_NOISE_PINK		= 20,   // pink noise
	PG_NOISE_GB_SHORT	= 21,   // gb short noise
								// ( 22- 23) reserved
	PG_PC_NZ_16BIT		= 24,   // pitch controlable periodic noise
	PG_PC_NZ_SHORT		= 25,   // pitch controlable 93byte noise
	PG_PC_NZ_OPM		= 26,   // pulse noise with OPM noise table
								// ( 27- 31) reserved
	PG_MA3_WAVE			= 32,   // ( 32- 63) MA3 waveforms.     PG_MA3_WAVE+[0,31]
	PG_PULSE			= 64,   // ( 64- 79) square pulse wave. PG_PULSE+[0,15]
	PG_PULSE_SPIKE		= 80,   // ( 80- 95) square pulse wave. PG_PULSE_SPIKE+[0,15]
								// ( 96-127) reserved
	PG_RAMP				= 128,  // (128-255) ramp wave.         PG_RAMP+[0,127]
	PG_CUSTOM			= 256,  // (256-383) custom wave table. PG_CUSTOM+[0,127]
	PG_PCM				= 384,  // (384-511) PCM data.          PG_PCM+[0,128]

	PG_USER_CUSTOM		= -1,   // User registered custom wave table.
	PG_USER_PCM			= -2,   // User registered PCM data.
};

VARIANT_ENUM_CAST(SiONModuleType);
VARIANT_ENUM_CAST(SiONPitchTableType);
VARIANT_ENUM_CAST(SiONPulseGeneratorType);

#endif // SION_ENUMS_H

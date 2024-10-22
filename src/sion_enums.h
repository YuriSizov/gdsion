/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_ENUMS_H
#define SION_ENUMS_H

#include <godot_cpp/core/binder_common.hpp>

enum SiONChipType : signed int {
	CHIP_AUTO        = -1,
	CHIP_SIOPM       = 0,
	CHIP_OPL         = 1,
	CHIP_OPM         = 2,
	CHIP_OPN         = 3,
	CHIP_OPX         = 4,
	CHIP_MA3         = 5,
	CHIP_PMS_GUITAR  = 6,
	CHIP_ANALOG_LIKE = 7,
	CHIP_MAX
};

enum SiONModuleType : unsigned int {
	MODULE_PSG        = 0,  // PSG (DCSG)
	MODULE_APU        = 1,  // FC pAPU
	MODULE_NOISE      = 2,  // Noise wave
	MODULE_MA3        = 3,  // MA-3 wave form
	MODULE_SCC        = 4,  // SCC-like wave table
	MODULE_GENERIC_PG = 5,  // Generic pulse generator
	MODULE_FM         = 6,  // FM sound module
	MODULE_PCM        = 7,  // PCM
	MODULE_PULSE      = 8,  // Pulse wave
	MODULE_RAMP       = 9,  // Ramp wave
	MODULE_SAMPLE     = 10, // Sampler
	MODULE_KS         = 11, // Karplus-Strong
	MODULE_GB         = 12, // GameBoy-like
	MODULE_VRC6       = 13, // VRC6
	MODULE_SID        = 14, // SID
	MODULE_FM_OPM     = 15, // YM2151
	MODULE_FM_OPN     = 16, // YM2203
	MODULE_FM_OPNA    = 17, // YM2608
	MODULE_FM_OPLL    = 18, // YM2413
	MODULE_FM_OPL3    = 19, // YM3812
	MODULE_FM_MA3     = 20, // YMU762
	MODULE_MAX
};

enum SiONPitchTableType : unsigned int {
	PITCH_TABLE_OPM = 0,
	PITCH_TABLE_PCM = 1,
	PITCH_TABLE_PSG = 2,
	PITCH_TABLE_OPM_NOISE = 3,
	PITCH_TABLE_PSG_NOISE = 4,
	PITCH_TABLE_APU_NOISE = 5,
	PITCH_TABLE_GB_NOISE = 6,
	PITCH_TABLE_MAX
};

// A.k.a. wave/waveform shapes.
enum SiONPulseGeneratorType : int {
	PULSE_SINE				= 0,    // sine wave.
	PULSE_SAW_UP			= 1,    // upwards saw wave.
	PULSE_SAW_DOWN			= 2,    // downwards saw wave.
	PULSE_TRIANGLE_FC		= 3,    // triangle wave quantized by 4 bits.
	PULSE_TRIANGLE			= 4,    // triangle wave.
	PULSE_SQUARE			= 5,    // square wave.
	PULSE_NOISE				= 6,    // 32k white noise.
	PULSE_KNM_BUBBLE		= 7,    // Konami bubble system wave.
	PULSE_SYNC_LOW			= 8,    // pseudo sync (low frequency).
	PULSE_SYNC_HIGH			= 9,    // pseudo sync (high frequency).
	PULSE_OFFSET			= 10,   // offset.
	PULSE_SAW_VC6			= 11,   // VC6 saw (32-sample saw).
									// ( 12- 15) reserved.

	PULSE_NOISE_WHITE		= 16,   // 16k white noise.
	PULSE_NOISE_PULSE		= 17,   // 16k pulse noise.
	PULSE_NOISE_SHORT		= 18,   // fc short noise.
	PULSE_NOISE_HIPASS		= 19,   // high pass noise.
	PULSE_NOISE_PINK		= 20,   // pink noise.
	PULSE_NOISE_GB_SHORT	= 21,   // GameBoy-like short noise.
									// ( 22- 23) reserved.

	PULSE_PC_NZ_16BIT		= 24,   // pitch controllable periodic noise
	PULSE_PC_NZ_SHORT		= 25,   // pitch controllable 93-byte noise
	PULSE_PC_NZ_OPM			= 26,   // pulse noise with OPM noise table
									// ( 27- 31) reserved.

	PULSE_MA3_SINE						= 32,   // MA-3 sine.
	PULSE_MA3_SINE_HALF					= 33,   // MA-3 half sine.
	PULSE_MA3_SINE_HALF_DOUBLE			= 34,   // MA-3 half sine doubled.
	PULSE_MA3_SINE_QUART_DOUBLE			= 35,   // MA-3 quarter sine doubled.
	PULSE_MA3_SINE_X2					= 36,   // MA-3 sine sped up (half phase).
	PULSE_MA3_SINE_HALF_DOUBLE_X2		= 37,   // MA-3 half sine doubled sped up (half phase).
	PULSE_MA3_SQUARE					= 38,   // MA-3 square.
	PULSE_MA3_SAW_SINE					= 39,   // MA-3 downwards saw with sine flattening.
	PULSE_MA3_TRI_SINE					= 40,   // MA-3 triangle modulated sine.
	PULSE_MA3_TRI_SINE_HALF				= 41,   // MA-3 half triangle modulated sine.
	PULSE_MA3_TRI_SINE_HALF_DOUBLE		= 42,   // MA-3 half triangle modulated sine doubled.
	PULSE_MA3_TRI_SINE_QUART_DOUBLE		= 43,   // MA-3 quarter triangle modulated sine doubled.
	PULSE_MA3_TRI_SINE_X2				= 44,   // MA-3 triangle modulated sine sped up (half phase).
	PULSE_MA3_TRI_SINE_HALF_DOUBLE_X2	= 45,   // MA-3 half triangle modulated sine doubled sped up (half phase).
	PULSE_MA3_SQUARE_HALF				= 46,   // MA-3 half square.
	PULSE_MA3_USER1						= 47,   // MA-3 user defined wave.
	PULSE_MA3_TRI						= 48,   // MA-3 triangle.
	PULSE_MA3_TRI_HALF					= 49,   // MA-3 half triangle.
	PULSE_MA3_TRI_HALF_DOUBLE			= 50,   // MA-3 half triangle doubled.
	PULSE_MA3_TRI_QUART_DOUBLE			= 51,   // MA-3 quarter triangle doubled.
	PULSE_MA3_TRI_X2					= 52,   // MA-3 triangle sped up (half phase).
	PULSE_MA3_TRI_HALF_DOUBLE_X2		= 53,   // MA-3 half triangle doubled sped up (half phase).
	PULSE_MA3_SQUARE_QUART_DOUBLE		= 54,   // MA-3 quarter square doubled.
	PULSE_MA3_USER2						= 55,   // MA-3 user defined wave.
	PULSE_MA3_SAW						= 56,   // MA-3 upwards saw.
	PULSE_MA3_SAW_HALF					= 57,   // MA-3 half upwards saw.
	PULSE_MA3_SAW_HALF_DOUBLE			= 58,   // MA-3 half upwards saw doubled.
	PULSE_MA3_SAW_QUART_DOUBLE			= 59,   // MA-3 quarter upwards saw doubled.
	PULSE_MA3_SAW_X2					= 60,   // MA-3 upwards saw sped up (half phase).
	PULSE_MA3_SAW_HALF_DOUBLE_X2		= 61,   // MA-3 half upwards saw doubled sped up (half phase).
	PULSE_MA3_SQUARE_QUART				= 62,   // MA-3 quarter square.
	PULSE_MA3_USER3						= 63,   // MA-3 user defined wave.

	PULSE_PULSE				= 64,   // ( 64- 79) square pulse wave. PULSE_PULSE+[0,15]
	PULSE_PULSE_SPIKE		= 80,   // ( 80- 95) square pulse wave. PULSE_PULSE_SPIKE+[0,15]

									// ( 96-127) reserved.

	PULSE_RAMP				= 128,  // (128-255) ramp waves.        PULSE_RAMP+[0,127]
									// ~ [0-4]     = upwards saw wave.
									// ~ [64]      = triangle wave.
									// ~ [124-127] = downwards saw wave.

	PULSE_CUSTOM			= 256,  // (256-383) custom wave table. PULSE_CUSTOM+[0,127]
	PULSE_PCM				= 384,  // (384-511) PCM data.          PULSE_PCM+[0,127]

	PULSE_USER_CUSTOM		= -1,   // User registered custom wave table.
	PULSE_USER_PCM			= -2,   // User registered PCM data.
};

VARIANT_ENUM_CAST(SiONChipType);
VARIANT_ENUM_CAST(SiONModuleType);
VARIANT_ENUM_CAST(SiONPitchTableType);
VARIANT_ENUM_CAST(SiONPulseGeneratorType);

#endif // SION_ENUMS_H

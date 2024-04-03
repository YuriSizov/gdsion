/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_VOICE_PRESET_UTIL_H
#define SION_VOICE_PRESET_UTIL_H

#include <godot_cpp/core/binder_common.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class SiONVoice;
class SiOPMWaveTable;

// A utility class for generating voice presets.
// 650 voices are available at this time (default:16, valsound:258, GM:128x2, GMdrum:60x2).
class SiONVoicePresetUtil : public Object {
	GDCLASS(SiONVoicePresetUtil, Object)

	HashMap<String, SiONVoice *> _voice_map;

	List<SiONVoice *> _current_category;
	HashMap<String, List<SiONVoice *>> _category_map;

	List<SiOPMWaveTable *> _wave_tables;
	List<String> _wave_table_hexes;

	void _generate_default_voices();
	void _generate_valsound_voices();
	void _generate_midi_voices();
	void _generate_mididrum_voices();
	void _generate_wavetable_voices();
	void _generate_single_drum_voices();

	void _create_basic_voice(const String &p_key, const String &p_name, int p_channel_num);
	void _create_percussive_voice(const String &p_key, const String &p_name, int p_wave_shape, int p_attack_rate, int p_release_rate, int p_release_sweep, int p_cutoff = 128, int p_resonance = 0);
	void _create_analog_voice(const String &p_key, const String &p_name, int p_connection_type, int p_wave_shape1 = 0, int p_wave_shape2 = 0, int p_balance = 0, int p_pitch_diff = 0);
	void _create_opn_voice(const String &p_key, const String &p_name, Vector<int> p_params);
	void _create_ma3_voice(const String &p_key, const String &p_name, Vector<int> p_params);
	void _create_wave_table_voice(const String &p_key, const String &p_name, int p_wave_shape, int p_attack_rate, int p_decay_rate, int p_sustain_rate, int p_release_rate, int p_sustain_level, int p_total_level, int p_multiple = 1);
	void _create_single_drum_voice(const String &p_key, const String &p_name, int p_wave_shape, int p_attack_rate, int p_decay_rate, int p_sustain_rate, int p_release_rate, int p_sustain_level, int p_total_level, int p_release_sweep = 0, double p_fine_multiple = 1);

	void _begin_category(const String &p_key);
	void _register_voice(const String &p_key, SiONVoice *p_voice);
	void _register_wave_table(const String &p_hex, Vector<int> p_wavelet);

protected:
	static void _bind_methods();

public:
	enum GeneratorFlags {
		INCLUDE_DEFAULT = 1,      // Include default voices.
		INCLUDE_VALSOUND = 2,     // Include voices set [valsound] (http://www.valsound.com/).
		INCLUDE_MIDI = 4,         // Include General MIDI voices.
		INCLUDE_MIDIDRUM = 8,     // Include General MIDI drum set voices.
		INCLUDE_WAVETABLE = 16,   // Include wave table voices.
		INCLUDE_SINGLE_DRUM = 32, // Include single voice drum voices.

		INCLUDE_ALL = 0xffff      // Include all voices.
	};

	void generate_voices(uint32_t p_flags = GeneratorFlags::INCLUDE_ALL);
	SiONVoice *get_voice_preset(const String &p_key) const;

	SiONVoicePresetUtil(uint32_t p_flags = GeneratorFlags::INCLUDE_ALL);
	~SiONVoicePresetUtil() {}
};

VARIANT_ENUM_CAST(SiONVoicePresetUtil::GeneratorFlags);

#endif // SION_VOICE_PRESET_UTIL_H

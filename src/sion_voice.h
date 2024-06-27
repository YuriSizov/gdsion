/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SION_VOICE_H
#define SION_VOICE_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include "sequencer/simml_voice.h"

using namespace godot;

class SiOPMWaveTable;
class SiOPMWavePCMData;
class SiOPMWaveSamplerData;
class SiOPMWaveSamplerTable;
enum SiONChipType : signed int;
enum SiONModuleType : unsigned int;

// Provides all of voice setting parameters of SiON.
class SiONVoice : public SiMMLVoice {
	GDCLASS(SiONVoice, SiMMLVoice)

	String _name;

protected:
	static void _bind_methods();

public:
	// NOTE: Godot doesn't support exposing constructors to the API, so we make do with a static factory method. Hopefully this can be fixed at some point.
	static Ref<SiONVoice> create(SiONModuleType p_module_type = (SiONModuleType)5, int p_channel_num = 0, int p_attack_rate = 63, int p_release_rate = 63, int p_pitch_shift = 0, int p_connection_type = -1, int p_wave_shape2 = 0, int p_pitch_shift2 = 0);

	//

	String get_name() const { return _name; }
	void set_name(const String &p_name) { _name = p_name; }

	void set_params(TypedArray<int> p_args);
	void set_params_opl(TypedArray<int> p_args);
	void set_params_opm(TypedArray<int> p_args);
	void set_params_opn(TypedArray<int> p_args);
	void set_params_opx(TypedArray<int> p_args);
	void set_params_ma3(TypedArray<int> p_args);
	void set_params_al(TypedArray<int> p_args);

	TypedArray<int> get_params() const;
	TypedArray<int> get_params_opl() const;
	TypedArray<int> get_params_opm() const;
	TypedArray<int> get_params_opn() const;
	TypedArray<int> get_params_opx() const;
	TypedArray<int> get_params_ma3() const;
	TypedArray<int> get_params_al() const;

	String get_mml(int p_index, SiONChipType p_chip_type = (SiONChipType)-1, bool p_append_postfix = true) const;
	int set_by_mml(String p_mml);

	SiOPMWaveTable *set_wave_table(Vector<double> *p_data);

	SiOPMWavePCMData *set_pcm_voice(const Variant &p_data, int p_sampling_note = 69, int p_src_channel_count = 2, int p_channel_count = 0);
	SiOPMWaveSamplerData *set_mp3_voice(Object *p_wave, bool p_ignore_note_off = false, int p_channel_count = 2);

	SiOPMWavePCMData *set_pcm_wave(int p_index, const Variant &p_data, int p_sampling_note = 69, int p_key_range_from = 0, int p_key_range_to = 127, int p_src_channel_count = 2, int p_channel_count = 0);
	SiOPMWaveSamplerData *set_sampler_wave(int p_index, const Variant &p_data, bool p_ignore_note_off = false, int p_pan = 0, int p_src_channel_count = 2, int p_channel_count = 0);

	void set_sampler_table(SiOPMWaveSamplerTable *p_table);

	void set_pms_guitar(int p_attack_rate = 48, int p_decay_rate = 48, int p_total_level = 0, int p_fixed_pitch = 69, int p_wave_shape = 20, int p_tension = 8);
	void set_analog_like(int p_connection_type, int p_wave_shape1 = 1, int p_wave_shape2 = 1, int p_balance = 0, int p_pitch_difference = 0);

	void set_envelope(int p_attack_rate, int p_decay_rate, int p_sustain_rate, int p_release_rate, int p_sustain_level, int p_total_level);
	void set_filter_envelope(int p_filter_type = 0, int p_cutoff = 128, int p_resonance = 0, int p_attack_rate = 0, int p_decay_rate1 = 0, int p_decay_rate2 = 0, int p_release_rate = 0, int p_decay_cutoff1 = 128, int p_decay_cutoff2 = 64, int p_sustain_cutoff = 32, int p_release_cutoff = 128);
	void set_amplitude_modulation(int p_depth = 0, int p_end_depth = 0, int p_delay = 0, int p_term = 0);
	void set_pitch_modulation(int p_depth = 0, int p_end_depth = 0, int p_delay = 0, int p_term = 0);

	Ref<SiONVoice> clone();
	virtual void reset() override;

	SiONVoice(SiONModuleType p_module_type = (SiONModuleType)5, int p_channel_num = 0, int p_attack_rate = 63, int p_release_rate = 63, int p_pitch_shift = 0, int p_connection_type = -1, int p_wave_shape2 = 0, int p_pitch_shift2 = 0);
	~SiONVoice() {}
};

#endif // SION_VOICE_H

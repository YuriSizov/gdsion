/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_VOWEL_H
#define SI_FILTER_VOWEL_H

#include <godot_cpp/templates/vector.hpp>
#include "effector/si_effect_base.h"

using namespace godot;

class SiFilterVowel : public SiEffectBase {
	GDCLASS(SiFilterVowel, SiEffectBase)

	static const int FORMANT_COUNT = 6;
	static const int BAND_TABLE_MAX = 8;
	static const int FREQ_TABLE_MAX = 1024;
	static const int GAIN_TABLE_MAX = 128;

	double _output_level = 1;

	// Formants.

	struct Formant {
		static bool _initialized;
		static double _alpha_table[BAND_TABLE_MAX][FREQ_TABLE_MAX];
		static double _cos_table[FREQ_TABLE_MAX];
		static double _gain_table[GAIN_TABLE_MAX];
		static double _band_list[BAND_TABLE_MAX];

		double ab1 = 0;
		double a2 = 0;
		double b0 = 1;
		double b2 = 0;

		static void initialize();
		static int calculate_freq_index(double p_frequency);

		void update(int p_freq_index, int p_gain, int p_band_index);
	};

	Vector<Formant> _formants;

	void _set_formant_band(int p_index, double p_frequency,  int p_gain, int p_band_index);

	struct FormantTap {
		double in0 = 0;
		double in1 = 0;
		double out0 = 0;
		double out1 = 0;

		void clear() {
			in0 = 0;
			in1 = 0;
			out0 = 0;
			out1 = 0;
		}
	};

	// Tap matrix.
	FormantTap _tap0;
	FormantTap _tap1;
	FormantTap _tap2;
	FormantTap _tap3;
	FormantTap _tap4;
	FormantTap _tap5;

	// Event chain.

	struct FormantEvent {
		FormantEvent *next = nullptr;

		int frequency1 = 0;
		int gain1 = 0;
		int frequency2 = 0;
		int gain2 = 0;

		double output_level = 0;
		int time = 0;

		FormantEvent *insert_to(FormantEvent *p_chain);
		int update_time(int p_delta);

		FormantEvent(int p_time, double p_output_level, int p_frequency1, int p_gain1, int p_frequency2, int p_gain2);
	};

	FormantEvent *_event_queue = nullptr;

	int _update_event(int p_time);

	//

	double _process_lfo_formant(Formant p_formant, FormantTap p_tap, double *r_input);
	void _process_lfo(Vector<double> *r_buffer, int p_start_index, int p_length);

protected:
	static void _bind_methods();

public:
	// Formants.

	void set_vowel_formants(double p_output_level, double p_frequency1, int p_gain1, double p_frequency2, int p_gain2, int p_delay = 0);
	void set_formant_band1(double p_frequency = 800,  int p_gain = 36, int p_band_index = 3) { _set_formant_band(0, p_frequency, p_gain, p_band_index); }
	void set_formant_band2(double p_frequency = 1300, int p_gain = 24, int p_band_index = 3) { _set_formant_band(1, p_frequency, p_gain, p_band_index); }
	void set_formant_band3(double p_frequency = 2200, int p_gain = 12, int p_band_index = 3) { _set_formant_band(2, p_frequency, p_gain, p_band_index); }
	void set_formant_band4(double p_frequency = 3500, int p_gain = 9,  int p_band_index = 3) { _set_formant_band(3, p_frequency, p_gain, p_band_index); }
	void set_formant_band5(double p_frequency = 4500, int p_gain = 6,  int p_band_index = 3) { _set_formant_band(4, p_frequency, p_gain, p_band_index); }
	void set_formant_band6(double p_frequency = 5500, int p_gain = 3,  int p_band_index = 3) { _set_formant_band(5, p_frequency, p_gain, p_band_index); }

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;

	SiFilterVowel();
	~SiFilterVowel() {}
};

#endif // SI_FILTER_VOWEL_H

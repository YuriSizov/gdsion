/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_filter_vowel.h"

// Formants.

bool SiFilterVowel::Formant::_initialized = false;
double SiFilterVowel::Formant::_alpha_table[BAND_TABLE_MAX][FREQ_TABLE_MAX];
double SiFilterVowel::Formant::_cos_table[FREQ_TABLE_MAX];
double SiFilterVowel::Formant::_gain_table[GAIN_TABLE_MAX];
double SiFilterVowel::Formant::_band_list[BAND_TABLE_MAX] = { 0.25, 0.5, 0.75, 1, 1.5, 2, 3, 4 };

void SiFilterVowel::Formant::initialize() {
	if (_initialized) {
		return;
	}
	_initialized = true;

	// Generate alpha table.
	for (int i = 0; i < BAND_TABLE_MAX; i++) {
		double band = _band_list[i];
		double frequency = 50.0;
		for (int j = 0; j < FREQ_TABLE_MAX; j++) {
			// TODO: Pick better names for these variables, maybe?

			double omg  = frequency * 0.00014247585730565955; // 2*pi/44100
			double sin  = Math::sin(omg);
			double ang = 0.34657359027997264 * band * omg / sin; // log(2)*0.5

			_alpha_table[i][j] = sin * Math::sinh(ang);

			frequency *= 1.0218971486541166; // 2^(1/32)
		}
	}

	// Generate cos table.
	{
		double frequency = 50.0;
		for (int j = 0; j < FREQ_TABLE_MAX; j++) {
			_cos_table[j] = Math::cos(frequency * 0.00014247585730565955);

			frequency *= 1.0218971486541166; // 2^(1/32)
		}
	}

	// Generate gain table.
	for (int i = 0; i < GAIN_TABLE_MAX; i++) {
		_gain_table[i] = Math::pow(10, (i - 32) * 0.025);
	}
}

int SiFilterVowel::Formant::calculate_freq_index(double p_frequency) {
	int freq_index = (Math::log(p_frequency) * 1.4426950408889633 - 5.643856189774724) * 32; // * 1/loge(2) - log2(50)
	return CLAMP(freq_index, 0, 1023);
}

void SiFilterVowel::Formant::update(int p_freq_index, int p_gain, int p_band_index) {
	int gain_index = CLAMP(p_gain + 32, 0, 127);
	double alpha = _alpha_table[p_band_index][p_freq_index];
	double gain = _gain_table[gain_index];

	double alpA  = alpha * gain;
	double alpiA = alpha / gain;
	double ia0   = 1.0 / (1.0 + alpiA);

	ab1 = -2 * _cos_table[p_freq_index] * ia0;
	a2 = (1 - alpiA) * ia0;
	b0 = (1 + alpA) * ia0;
	b2 = (1 - alpA) * ia0;
}

void SiFilterVowel::set_vowel_formants(double p_output_level, double p_frequency1, int p_gain1, double p_frequency2, int p_gain2, int p_delay) {
	int freq_index1 = Formant::calculate_freq_index(p_frequency1);
	int freq_index2 = Formant::calculate_freq_index(p_frequency2);

	FormantEvent *event = memnew(FormantEvent(p_delay, p_output_level, p_frequency1, p_gain1, p_frequency2, p_gain2));
	_event_queue = event->insert_to(_event_queue);
}

void SiFilterVowel::_set_formant_band(int p_index, double p_frequency,  int p_gain, int p_band_index) {
	ERR_FAIL_INDEX(p_index, _formants.size());

	int freq_index = Formant::calculate_freq_index(p_frequency);
	_formants.write[p_index].update(freq_index, p_gain, p_band_index);
}

// Event chain.

SiFilterVowel::FormantEvent *SiFilterVowel::FormantEvent::insert_to(FormantEvent *p_chain) {
	// The chain is empty, assume its place.
	if (p_chain == nullptr) {
		return this;
	}

	// The first event is later, put yourself in front.
	if (time < p_chain->time) {
		next = p_chain;
		return this;
	}

	// Find the your place in the chain and insert yourself.
	FormantEvent *event = p_chain;
	while (event->next) {
		if (event->time <= time && time < event->next->time) {
			next = event->next;
			event->next = this;
			break;
		}

		event = event->next;
	}
	event->next = this;

	return p_chain;
}

int SiFilterVowel::FormantEvent::update_time(int p_delta) {
	int delta = MIN(p_delta, time);

	for (FormantEvent *event = this; event; event = event->next) {
		event->time -= delta;
	}

	return delta;
}

SiFilterVowel::FormantEvent::FormantEvent(int p_time, double p_output_level, int p_frequency1, int p_gain1, int p_frequency2, int p_gain2) {
	time = p_time;
	output_level = p_output_level;

	frequency1 = p_frequency1;
	gain1 = p_gain1;
	frequency2 = p_frequency2;
	gain2 = p_gain2;
}

int SiFilterVowel::_update_event(int p_time) {
	while (_event_queue && _event_queue->time == 0) {
		_formants.write[0].update(_event_queue->frequency1, _event_queue->gain1, 3);
		_formants.write[1].update(_event_queue->frequency2, _event_queue->gain2, 2);

		_output_level = _event_queue->output_level;

		FormantEvent *next_event = _event_queue->next;
		memdelete(_event_queue);
		_event_queue = next_event;
	}

	return _event_queue ? _event_queue->update_time(p_time) : p_time;
}

//

int SiFilterVowel::prepare_process() {
	_tap0.clear();
	_tap1.clear();
	_tap2.clear();
	_tap3.clear();
	_tap4.clear();
	_tap5.clear();

	return 1;
}

double SiFilterVowel::_process_lfo_formant(Formant p_formant, FormantTap p_tap, double *r_input) {
	double fab1 = p_formant.ab1;
	double fa2 = p_formant.a2;
	double fb0 = p_formant.b0;
	double fb2 = p_formant.b2;

	double output = fb0 * (*r_input) + fab1 * p_tap.in0 + fb2 * p_tap.in1 - fab1 * p_tap.out0 - fa2 * p_tap.out1;
	p_tap.in1 = p_tap.in0;
	p_tap.in0 = (*r_input);
	p_tap.out1 = p_tap.out0;
	p_tap.out0 = output;
	*r_input = output;

	return output;
}

void SiFilterVowel::_process_lfo(Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	for (int i = start_index; i < (start_index + length); i += 2) {
		double input = (*r_buffer)[i];

		_process_lfo_formant(_formants[0], _tap0, &input);
		_process_lfo_formant(_formants[1], _tap1, &input);
		_process_lfo_formant(_formants[2], _tap2, &input);
		_process_lfo_formant(_formants[3], _tap3, &input);
		_process_lfo_formant(_formants[4], _tap4, &input);

		double output = _process_lfo_formant(_formants[5], _tap5, &input);
		output = CLAMP(output * _output_level, -1, 1);

		r_buffer->write[i] = output;
		r_buffer->write[i + 1] = output;
	}
}

int SiFilterVowel::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int length = p_length;
	for (int i = p_start_index; i < (p_start_index + p_length); ) {
		int step = _update_event(length);
		_process_lfo(r_buffer, i, step);

		i += step;
		length -= step;
	}

	return 1;
}

void SiFilterVowel::set_by_mml(Vector<double> p_args) {
	_output_level = _get_mml_arg(p_args, 0, 100) / 100.0;

	double frequency1 = _get_mml_arg(p_args, 1, 800);
	int gain1         = _get_mml_arg(p_args, 2, 36);
	set_formant_band1(frequency1, gain1);

	double frequency2 = _get_mml_arg(p_args, 3, 1300);
	int gain2         = _get_mml_arg(p_args, 4, 24);
	set_formant_band1(frequency2, gain2);

	double frequency3 = _get_mml_arg(p_args, 5, 2200);
	int gain3         = _get_mml_arg(p_args, 6, 12);
	set_formant_band1(frequency3, gain3);

	double frequency4 = _get_mml_arg(p_args, 7, 3500);
	int gain4         = _get_mml_arg(p_args, 8, 9);
	set_formant_band1(frequency4, gain4);

	double frequency5 = _get_mml_arg(p_args, 9, 4500);
	int gain5         = _get_mml_arg(p_args, 10, 6);
	set_formant_band1(frequency5, gain5);

	double frequency6 = _get_mml_arg(p_args, 11, 5500);
	int gain6         = _get_mml_arg(p_args, 12, 3);
	set_formant_band1(frequency6, gain6);
}

void SiFilterVowel::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_vowel_formants", "output_level", "frequency1", "gain1", "frequency2", "gain2", "delay"), &SiFilterVowel::set_vowel_formants, DEFVAL(0));

	ClassDB::bind_method(D_METHOD("set_formant_band1", "frequency", "gain", "band_index"), &SiFilterVowel::set_formant_band1, DEFVAL(800),  DEFVAL(36), DEFVAL(3));
	ClassDB::bind_method(D_METHOD("set_formant_band2", "frequency", "gain", "band_index"), &SiFilterVowel::set_formant_band2, DEFVAL(1300), DEFVAL(24), DEFVAL(3));
	ClassDB::bind_method(D_METHOD("set_formant_band3", "frequency", "gain", "band_index"), &SiFilterVowel::set_formant_band3, DEFVAL(2200), DEFVAL(12), DEFVAL(3));
	ClassDB::bind_method(D_METHOD("set_formant_band4", "frequency", "gain", "band_index"), &SiFilterVowel::set_formant_band4, DEFVAL(3500), DEFVAL(9),  DEFVAL(3));
	ClassDB::bind_method(D_METHOD("set_formant_band5", "frequency", "gain", "band_index"), &SiFilterVowel::set_formant_band5, DEFVAL(4500), DEFVAL(6),  DEFVAL(3));
	ClassDB::bind_method(D_METHOD("set_formant_band6", "frequency", "gain", "band_index"), &SiFilterVowel::set_formant_band6, DEFVAL(5500), DEFVAL(3),  DEFVAL(3));
}

SiFilterVowel::SiFilterVowel() :
		SiEffectBase() {
	Formant::initialize();

	_formants.resize_zeroed(FORMANT_COUNT);
	for (int i = 0; i < _formants.size(); i++) {
		Formant formant;
		_formants.write[i] = formant;
	}

	set_formant_band1();
	set_formant_band2();
	set_formant_band3();
	set_formant_band4();
	set_formant_band5();
	set_formant_band6();
}

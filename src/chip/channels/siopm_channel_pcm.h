/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_PCM_H
#define SIOPM_CHANNEL_PCM_H

#include "chip/channels/siopm_channel_base.h"
#include "templates/singly_linked_list.h"

class SiOPMOperator;

class SiOPMChannelPCM : public SiOPMChannelBase {
	GDCLASS(SiOPMChannelPCM, SiOPMChannelBase)

	static const int IDLING_THRESHOLD = 5120; // = 256(resolution)*10(2^10=1024)*2(p/n) = volume<1/1024

	SiOPMOperator *_operator = nullptr;
	Ref<SiOPMWavePCMTable> _pcm_table;
	// Second set of variables for stereo.
	double _filter_variables2[3] = { 0, 0, 0 };

	int _amplitude_modulation_depth = 0; // = chip.amd << (ams - 1)
	int _amplitude_modulation_output_level = 0;
	int _pitch_modulation_depth = 0; // = chip.pmd << (pms - 1)
	int _pitch_modulation_output_level = 0;

	int _eg_timer_initial = 0; // ENV_TIMER_INITIAL * freq_ratio
	int _lfo_timer_initial = 0; // LFO_TIMER_INITIAL * freq_ratio

	int _sample_pitch_shift = 0;
	double _sample_volume = 1;
	int _sample_pan = 0;

	// Second output pipe for stereo.
	SinglyLinkedList<int> *_out_pipe2 = nullptr;

	// LFO control.

	void _set_lfo_state(bool p_enabled);
	void _set_lfo_timer(int p_value);

	// Processing.

	void _no_process(int p_length);

	void _update_lfo();
	void _process_operator_mono(int p_length, bool p_mix);
	void _process_operator_stereo(int p_length, bool p_mix);

	void _write_stream_mono(SinglyLinkedList<int>::Element *p_output, int p_length);
	void _write_stream_stereo(SinglyLinkedList<int>::Element *p_output_left, SinglyLinkedList<int>::Element *p_output_right, int p_length);

protected:
	static void _bind_methods();

	String _to_string() const;

public:
	virtual void get_channel_params(const Ref<SiOPMChannelParams> &p_params) const override;
	virtual void set_channel_params(const Ref<SiOPMChannelParams> &p_params, bool p_with_volume, bool p_with_modulation = true) override;
	void set_params_by_value(int p_ar, int p_dr, int p_sr, int p_rr, int p_sl, int p_tl, int p_ksr, int p_ksl, int p_mul, int p_dt1, int p_detune, int p_ams, int p_phase, int p_fix_note);

	virtual void set_wave_data(const Ref<SiOPMWaveBase> &p_wave_data) override;

	virtual void set_parameters(Vector<int> p_params) override;
	virtual void set_types(int p_pg_type, SiONPitchTableType p_pt_type) override;
	virtual void set_all_attack_rate(int p_value) override;
	virtual void set_all_release_rate(int p_value) override;

	virtual int get_pitch() const override;
	virtual void set_pitch(int p_value) override;

	virtual void set_release_rate(int p_value) override;
	virtual void set_total_level(int p_value) override;
	virtual void set_fine_multiple(int p_value) override;
	virtual void set_phase(int p_value) override;
	virtual void set_detune(int p_value) override;
	virtual void set_fixed_pitch(int p_value) override;
	virtual void set_ssg_envelope_control(int p_value) override;
	virtual void set_envelope_reset(bool p_reset) override;

	// Volume control.

	virtual void offset_volume(int p_expression, int p_velocity) override;

	// LFO control.

	virtual void set_frequency_ratio(int p_ratio) override;
	virtual void initialize_lfo(int p_waveform, Vector<int> p_custom_wave_table = Vector<int>()) override;
	virtual void set_amplitude_modulation(int p_depth) override;
	virtual void set_pitch_modulation(int p_depth) override;

	// Processing.

	virtual void note_on() override;
	virtual void note_off() override;

	virtual void reset_channel_buffer_status() override;
	virtual void buffer(int p_length) override;
	virtual void buffer_no_process(int p_length) override;

	//

	virtual void initialize(SiOPMChannelBase *p_prev, int p_buffer_index) override;
	virtual void reset() override;

	SiOPMChannelPCM(SiOPMSoundChip *p_chip = nullptr);
	~SiOPMChannelPCM();
};

#endif // SIOPM_CHANNEL_PCM_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_FM_H
#define SIOPM_CHANNEL_FM_H

#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "chip/channels/siopm_channel_base.h"
#include "templates/singly_linked_list.h"

using namespace godot;

class SiOPMOperator;

// FM sound channel.
// This implementation is based on OPM emulation, referenced from sources of mame, fmgen, and x68sound.
// It also introduces a few extensions to simulate several other FM sound modules (OPNA, OPLL, OPL2, OPL3, OPX, MA-3, MA-5, MA-7, TSS, and DX7), including:
//   - stereo output (from TSS, DX7)
//   - key scale level (from OPL3, OPX, MA-x)
//   - phase selection (from TSS)
//   - fixed frequency (from MA-x)
//   - SSG envelope control (from OPNA)
//   - wave shape selection (from OPX, MA-x, TSS)
//   - custom wave shapes (from MA-x)
//   - more supported algorithms (from OPLx, OPX, MA-x, DX7)
//   - decimal multiple (from p-TSS)
//   - feedback from operators 1-3 (from DX7)
//   - channel independent LFO (from TSS)
//   - low-pass filter envelope (from MA-x)
//   - flexible FM connections (from TSS)
//   - ring modulation (from C64?)
class SiOPMChannelFM : public SiOPMChannelBase {
	GDCLASS(SiOPMChannelFM, SiOPMChannelBase)

	static const int IDLING_THRESHOLD = 5120; // = 256(resolution)*10(2^10=1024)*2(p/n) = volume<1/1024

	static List<SiOPMOperator *> _operator_pool;

	SiOPMOperator *_alloc_operator();
	void _release_operator(SiOPMOperator *p_operator);

	int _algorithm = 0;

	enum ProcessType {
		PROCESS_OP1 = 0,
		PROCESS_OP2 = 1,
		PROCESS_OP3 = 2,
		PROCESS_OP4 = 3,
		PROCESS_ANALOG_LIKE = 4,
		PROCESS_RING = 5,
		PROCESS_SYNC = 6,
		PROCESS_AFM = 7, // ???
		PROCESS_PCM = 8,
	};

	Vector<Vector<Callable>> _process_function_list;
	ProcessType _process_function_type = PROCESS_OP1;

	void _update_process_function();
	void _update_operator_count(int p_count);

	SinglyLinkedList<int> *_pipe0 = nullptr;
	SinglyLinkedList<int> *_pipe1 = nullptr;

	enum RegisterType {
		REGISTER_OPM = 0,
		REGISTER_2A03 = 1, // Not actually implemented.
	};

	RegisterType _register_map_type = REGISTER_OPM;
	int _register_map_channel = 0;

	void _set_by_opm_register(int p_address, int p_data);

	void _set_algorithm_operator1(int p_algorithm);
	void _set_algorithm_operator2(int p_algorithm);
	void _set_algorithm_operator3(int p_algorithm);
	void _set_algorithm_operator4(int p_algorithm);
	void _set_algorithm_analog_like(int p_algorithm);

	// LFO control.

	virtual void _set_lfo_state(bool p_enabled);
	void _set_lfo_timer(int p_value);

	// Processing.

	void _update_lfo(int p_op_count);

	void _process_operator1_lfo_off(int p_length);
	void _process_operator1_lfo_on(int p_length);
	void _process_operator2(int p_length);
	void _process_operator3(int p_length);
	void _process_operator4(int p_length);
	void _process_pcm_lfo_off(int p_length);
	void _process_pcm_lfo_on(int p_length);
	void _process_analog_like(int p_length);
	void _process_ring(int p_length);
	void _process_sync(int p_length);

protected:
	static void _bind_methods();

	String _to_string() const;

	Vector<SiOPMOperator *> _operators;
	SiOPMOperator *_active_operator = nullptr;
	int _operator_count = 0;

	int _amplitude_modulation_depth = 0; // = chip.amd << (ams - 1)
	int _amplitude_modulation_output_level = 0;
	int _pitch_modulation_depth = 0; // = chip.pmd << (pms - 1)
	int _pitch_modulation_output_level = 0;

	int _eg_timer_initial = 0; // ENV_TIMER_INITIAL * freq_ratio
	int _lfo_timer_initial = 0; // LFO_TIMER_INITIAL * freq_ratio

public:
	static void finalize_pool();

	virtual void get_channel_params(const Ref<SiOPMChannelParams> &p_params) const override;
	virtual void set_channel_params(const Ref<SiOPMChannelParams> &p_params, bool p_with_volume, bool p_with_modulation = true) override;
	void set_params_by_value(int p_ar, int p_dr, int p_sr, int p_rr, int p_sl, int p_tl, int p_ksr, int p_ksl, int p_mul, int p_dt1, int p_detune, int p_ams, int p_phase, int p_fix_note);

	virtual void set_wave_data(const Ref<SiOPMWaveBase> &p_wave_data) override;
	virtual void set_channel_number(int p_value) override;
	virtual void set_register(int p_address, int p_data) override;

	virtual void set_algorithm(int p_operator_count, bool p_analog_like, int p_algorithm) override;
	virtual void set_feedback(int p_level, int p_connection) override;
	virtual void set_parameters(Vector<int> p_params) override;
	virtual void set_types(int p_pg_type, SiONPitchTableType p_pt_type) override;
	virtual void set_all_attack_rate(int p_value) override;
	virtual void set_all_release_rate(int p_value) override;

	virtual int get_pitch() const override;
	virtual void set_pitch(int p_value) override;

	virtual void set_active_operator_index(int p_value) override;
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

	//

	virtual void initialize(SiOPMChannelBase *p_prev, int p_buffer_index) override;
	virtual void reset() override;

	SiOPMChannelFM(SiOPMSoundChip *p_chip = nullptr);
	~SiOPMChannelFM();
};

#endif // SIOPM_CHANNEL_FM_H

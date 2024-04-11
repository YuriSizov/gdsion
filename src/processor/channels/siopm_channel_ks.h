/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_KS_H
#define SIOPM_CHANNEL_KS_H

#include <godot_cpp/templates/vector.hpp>
#include "processor/channels/siopm_channel_fm.h"
#include "templates/singly_linked_list.h"

using namespace godot;

enum SiONPitchTableType : int;
class SiOPMModule;

// Karplus-Strong algorithm with FM synth.
class SiOPMChannelKS : public SiOPMChannelFM {
	GDCLASS(SiOPMChannelKS, SiOPMChannelFM)

	static const int KS_BUFFER_SIZE = 5400; // 5394 = sampling count of MIDI note number=0

	enum KSSeedType {
		KS_SEED_DEFAULT = 0,
		KS_SEED_FM = 1,
		KS_SEED_PCM = 2,
	};

	KSSeedType _ks_seed_type = KS_SEED_DEFAULT;
	int _ks_seed_index = 0;

	Vector<int> _ks_delay_buffer;
	double _ks_delay_buffer_index = 0;
	int _ks_pitch_index = 0;

	double _ks_decay_lpf = 0.875;
	double _ks_decay = 0.98;
	double _ks_mute_decay_lpf = 0.5;
	double _ks_mute_decay = 0.75;

	double _output = 0;
	double _decay_lpf = 0.5;
	double _decay = 0.75;
	double _expression = 1;

	// LFO control.

	virtual void _set_lfo_state(bool p_enabled) override;

	// Processing.

	void _apply_karplus_strong(SinglyLinkedList<int> *p_target, int p_length);

protected:
	static void _bind_methods() {}

public:
	void set_karplus_strong_params(int p_attack_rate = 48, int p_decay_rate = 48, int p_total_level = 0, int p_fixed_pitch = 0, int p_wave_shape = -1, int p_tension = 8);

	virtual void set_parameters(Vector<int> p_params) override;
	virtual void set_types(int p_pg_type, SiONPitchTableType p_pt_type) override;
	virtual void set_all_attack_rate(int p_value) override;
	virtual void set_all_release_rate(int p_value) override;

	virtual int get_pitch() const override { return _ks_pitch_index; }
	virtual void set_pitch(int p_value) override { _ks_pitch_index = p_value; }

	virtual void set_release_rate(int p_value) override;
	virtual void set_fixed_pitch(int p_value) override;

	// Volume control.

	virtual void offset_volume(int p_expression, int p_velocity) override;

	// Processing.

	virtual void note_on() override;
	virtual void note_off() override;

	virtual void reset_channel_buffer_status() override;
	virtual void buffer(int p_length) override;

	//

	virtual String to_string() const override;

	virtual void initialize(SiOPMChannelBase *p_prev, int p_buffer_index) override;
	virtual void reset() override;

	SiOPMChannelKS(SiOPMModule *p_chip = nullptr);
};

#endif // SIOPM_CHANNEL_KS_H

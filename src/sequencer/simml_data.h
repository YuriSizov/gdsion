/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_DATA_H
#define SIMML_DATA_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "sequencer/base/mml_data.h"

using namespace godot;

class SiMMLEnvelopeTable;
class SiMMLVoice;
class SiOPMChannelParams;
class SiOPMWaveSamplerTable;
class SiOPMWaveTable;

class SiMMLData : public MMLData {
	GDCLASS(SiMMLData, MMLData)

protected:
	static void _bind_methods() {}

	Vector<SiMMLEnvelopeTable *> _envelope_tables;
	Vector<SiOPMWaveTable *> _wave_tables;
	Vector<SiOPMWaveSamplerTable *> _sampler_tables;

	Vector<Ref<SiMMLVoice>> _fm_voices;
	Vector<Ref<SiMMLVoice>> _pcm_voices;

public:
	void register_all();

	// Tables.

	Vector<SiMMLEnvelopeTable *> get_envelope_tables() const { return _envelope_tables; }

	SiMMLEnvelopeTable *get_envelope_table(int p_index) const;
	void set_envelope_table(int p_index, SiMMLEnvelopeTable *p_envelope);
	SiOPMWaveTable *get_wave_table(int p_index) const;
	SiOPMWaveTable *set_wave_table(int p_index, Vector<double> p_data);
	SiOPMWaveSamplerTable *get_sampler_table(int p_index) const;
	void set_sampler_table(int p_index, SiOPMWaveSamplerTable *p_sampler);

	// Voices.

	void set_voice(int p_index, const Ref<SiMMLVoice> &p_voice);
	SiOPMChannelParams *get_channel_params(int p_index);
	Ref<SiMMLVoice> get_pcm_voice(int p_index);

	//

	virtual void clear() override;

	SiMMLData();
	~SiMMLData() {}
};

#endif // SIMML_DATA_H

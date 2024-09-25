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

	Vector<Ref<SiMMLEnvelopeTable>> _envelope_tables;
	Vector<Ref<SiOPMWaveTable>> _wave_tables;
	Vector<Ref<SiOPMWaveSamplerTable>> _sampler_tables;

	Vector<Ref<SiMMLVoice>> _fm_voices;
	Vector<Ref<SiMMLVoice>> _pcm_voices;

public:
	// Static so it can be called when there is no SiMMLData instance available.
	static void clear_ref_stencils();
	void register_ref_stencils();

	// Tables.

	Vector<Ref<SiMMLEnvelopeTable>> get_envelope_tables() const { return _envelope_tables; }

	Ref<SiMMLEnvelopeTable> get_envelope_table(int p_index) const;
	void set_envelope_table(int p_index, const Ref<SiMMLEnvelopeTable> &p_envelope);
	Ref<SiOPMWaveTable> get_wave_table(int p_index) const;
	Ref<SiOPMWaveTable> set_wave_table(int p_index, Vector<double> *p_data);
	Ref<SiOPMWaveSamplerTable> get_sampler_table(int p_index) const;
	void set_sampler_table(int p_index, const Ref<SiOPMWaveSamplerTable> &p_sampler);

	// Voices.

	Ref<SiMMLVoice> initialize_voice(int p_index);
	void set_voice(int p_index, const Ref<SiMMLVoice> &p_voice);
	Ref<SiMMLVoice> get_pcm_voice(int p_index);

	//

	virtual void clear() override;

	SiMMLData();
	~SiMMLData();
};

#endif // SIMML_DATA_H

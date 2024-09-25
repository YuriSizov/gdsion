/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_data.h"

#include "chip/siopm_ref_table.h"
#include "chip/wave/siopm_wave_pcm_table.h"
#include "chip/wave/siopm_wave_sampler_table.h"
#include "chip/wave/siopm_wave_table.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_voice.h"

using namespace godot;

void SiMMLData::clear_ref_stencils() {
	// Bank 2 and 3 are not available at this time.
	SiOPMRefTable::get_instance()->clear_sampler_table_stencil(0);
	SiOPMRefTable::get_instance()->clear_sampler_table_stencil(1);

	SiOPMRefTable::get_instance()->clear_stencil_custom_wave_tables();
	SiOPMRefTable::get_instance()->clear_stencil_pcm_voices();

	SiMMLRefTable::get_instance()->clear_stencil_envelopes();
	SiMMLRefTable::get_instance()->clear_stencil_voices();
}

void SiMMLData::register_ref_stencils() {
	// Bank 2 and 3 are not available at this time.
	SiOPMRefTable::get_instance()->set_sampler_table_stencil(0, _sampler_tables[0]);
	SiOPMRefTable::get_instance()->set_sampler_table_stencil(1, _sampler_tables[1]);

	SiOPMRefTable::get_instance()->set_stencil_custom_wave_tables(_wave_tables);
	SiOPMRefTable::get_instance()->set_stencil_pcm_voices(_pcm_voices);

	SiMMLRefTable::get_instance()->set_stencil_envelopes(_envelope_tables);
	SiMMLRefTable::get_instance()->set_stencil_voices(_fm_voices);
}

// Tables.

Ref<SiMMLEnvelopeTable> SiMMLData::get_envelope_table(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, SiMMLRefTable::ENVELOPE_TABLE_MAX, nullptr);

	return _envelope_tables[p_index];
}

void SiMMLData::set_envelope_table(int p_index, const Ref<SiMMLEnvelopeTable> &p_envelope) {
	ERR_FAIL_INDEX(p_index, SiMMLRefTable::ENVELOPE_TABLE_MAX);

	_envelope_tables.write[p_index] = p_envelope;
}

Ref<SiOPMWaveTable> SiMMLData::get_wave_table(int p_index) const {
	int index = p_index & (SiOPMRefTable::WAVE_TABLE_MAX - 1);
	return _wave_tables[index];
}

Ref<SiOPMWaveTable> SiMMLData::set_wave_table(int p_index, Vector<double> *p_data) {
	int index = p_index & (SiOPMRefTable::WAVE_TABLE_MAX - 1);

	Vector<int> log_table;
	log_table.resize_zeroed(p_data->size());
	for (int i = 0; i < p_data->size(); i++) {
		log_table.write[i] = SiOPMRefTable::calculate_log_table_index((*p_data)[i]);
	}

	_wave_tables.write[index] = Ref<SiOPMWaveTable>(memnew(SiOPMWaveTable(log_table)));
	return _wave_tables[index];
}

Ref<SiOPMWaveSamplerTable> SiMMLData::get_sampler_table(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, SiOPMRefTable::SAMPLER_TABLE_MAX, nullptr);

	return _sampler_tables[p_index];
}

void SiMMLData::set_sampler_table(int p_index, const Ref<SiOPMWaveSamplerTable> &p_sampler) {
	ERR_FAIL_INDEX(p_index, SiOPMRefTable::SAMPLER_TABLE_MAX);

	_sampler_tables.write[p_index] = p_sampler;
}

// Voices.

Ref<SiMMLVoice> SiMMLData::initialize_voice(int p_index) {
	ERR_FAIL_INDEX_V(p_index, SiMMLRefTable::VOICE_MAX, Ref<SiMMLVoice>());

	Ref<SiMMLVoice> voice;
	voice.instantiate();
	_fm_voices.write[p_index] = voice;

	return voice;
}

void SiMMLData::set_voice(int p_index, const Ref<SiMMLVoice> &p_voice) {
	ERR_FAIL_INDEX(p_index, SiMMLRefTable::VOICE_MAX);
	ERR_FAIL_COND_MSG(!p_voice->is_suitable_for_fm_voice(), "SiMMLData: Cannot set voice data which is not suitable for FM voices.");

	_fm_voices.write[p_index] = p_voice;
}

Ref<SiMMLVoice> SiMMLData::get_pcm_voice(int p_index) {
	int index = p_index & (SiOPMRefTable::PCM_DATA_MAX - 1);
	if (_pcm_voices[index].is_null()) {
		Ref<SiMMLVoice> voice = SiMMLVoice::create_blank_pcm_voice(index);
		_pcm_voices.write[index] = voice;
	}

	return _pcm_voices[index];
}

//

void SiMMLData::clear() {
	MMLData::clear();

	for (int i = 0; i < SiMMLRefTable::ENVELOPE_TABLE_MAX; i++) {
		_envelope_tables.write[i] = Ref<SiMMLEnvelopeTable>();
	}

	for (int i = 0; i < SiMMLRefTable::VOICE_MAX; i++) {
		_fm_voices.write[i] = Ref<SiMMLVoice>();
	}

	for (int i = 0; i < SiOPMRefTable::WAVE_TABLE_MAX; i++) {
		_wave_tables.write[i] = Ref<SiOPMWaveTable>();
	}

	for (int i = 0; i < SiOPMRefTable::PCM_DATA_MAX; i++) {
		if (_pcm_voices[i].is_valid()) {
			Ref<SiOPMWavePCMTable> pcm_table = _pcm_voices[i]->get_wave_data();
			if (pcm_table.is_valid()) {
				pcm_table->clear();
			}

			_pcm_voices.write[i] = Ref<SiMMLVoice>();
		}
	}

	for (int i = 0; i < SiOPMRefTable::SAMPLER_TABLE_MAX; i++) {
		_sampler_tables.write[i] = Ref<SiOPMWaveSamplerTable>();
	}
}

SiMMLData::SiMMLData() {
	_envelope_tables.resize_zeroed(SiMMLRefTable::ENVELOPE_TABLE_MAX);
	_wave_tables.resize_zeroed(SiOPMRefTable::WAVE_TABLE_MAX);
	_sampler_tables.resize_zeroed(SiOPMRefTable::SAMPLER_TABLE_MAX);

	_fm_voices.resize_zeroed(SiMMLRefTable::VOICE_MAX);
	_pcm_voices.resize_zeroed(SiOPMRefTable::PCM_DATA_MAX);

	for (int i = 0; i < SiOPMRefTable::SAMPLER_TABLE_MAX; i++) {
		_sampler_tables.write[i] = Ref<SiOPMWaveSamplerTable>(memnew(SiOPMWaveSamplerTable));
	}
}

SiMMLData::~SiMMLData() {
	_envelope_tables.clear();
	_wave_tables.clear();
	_sampler_tables.clear();
}

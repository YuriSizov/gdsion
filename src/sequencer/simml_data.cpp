/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "simml_data.h"

#include "processor/siopm_ref_table.h"
#include "processor/wave/siopm_wave_pcm_table.h"
#include "processor/wave/siopm_wave_sampler_table.h"
#include "processor/wave/siopm_wave_table.h"
#include "sequencer/simml_envelope_table.h"
#include "sequencer/simml_ref_table.h"
#include "sequencer/simml_voice.h"

using namespace godot;

void SiMMLData::register_all() {
	// Bank 2 and 3 are not available at this time.
	SiOPMRefTable::get_instance()->sampler_tables[0]->set_stencil(_sampler_tables[0]);
	SiOPMRefTable::get_instance()->sampler_tables[1]->set_stencil(_sampler_tables[1]);

	SiOPMRefTable::get_instance()->set_stencil_custom_wave_tables(_wave_tables);
	SiOPMRefTable::get_instance()->set_stencil_pcm_voices(_pcm_voices);

	SiMMLRefTable::get_instance()->set_stencil_envelopes(_envelope_tables);
	SiMMLRefTable::get_instance()->set_stencil_voices(_fm_voices);
}

// Tables.

SiMMLEnvelopeTable *SiMMLData::get_envelope_table(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, SiMMLRefTable::ENVELOPE_TABLE_MAX, nullptr);

	return _envelope_tables[p_index];
}

void SiMMLData::set_envelope_table(int p_index, SiMMLEnvelopeTable *p_envelope) {
	ERR_FAIL_INDEX(p_index, SiMMLRefTable::ENVELOPE_TABLE_MAX);

	_envelope_tables.write[p_index] = p_envelope;
}

SiOPMWaveTable *SiMMLData::get_wave_table(int p_index) const {
	int index = p_index & (SiOPMRefTable::WAVE_TABLE_MAX - 1);
	return _wave_tables[index];
}

SiOPMWaveTable *SiMMLData::set_wave_table(int p_index, Vector<double> p_data) {
	int index = p_index & (SiOPMRefTable::WAVE_TABLE_MAX - 1);

	Vector<int> log_table;
	log_table.resize_zeroed(p_data.size());
	for (int i = 0; i < p_data.size(); i++) {
		log_table.write[i] = SiOPMRefTable::calculate_log_table_index(p_data[i]);
	}

	_wave_tables.write[index] = SiOPMWaveTable::alloc(log_table);
	return _wave_tables[index];
}

SiOPMWaveSamplerTable *SiMMLData::get_sampler_table(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, SiOPMRefTable::SAMPLER_TABLE_MAX, nullptr);

	return _sampler_tables[p_index];
}

void SiMMLData::set_sampler_table(int p_index, SiOPMWaveSamplerTable *p_sampler) {
	ERR_FAIL_INDEX(p_index, SiOPMRefTable::SAMPLER_TABLE_MAX);

	_sampler_tables.write[p_index] = p_sampler;
}

// Voices.

SiOPMChannelParams *SiMMLData::get_channel_params(int p_index) {
	SiMMLVoice *voice = memnew(SiMMLVoice);
	_fm_voices.write[p_index] = voice;

	return voice->get_channel_params();
}

void SiMMLData::set_voice(int p_index, SiMMLVoice *p_voice) {
	ERR_FAIL_INDEX(p_index, SiMMLRefTable::VOICE_MAX);
	ERR_FAIL_COND_MSG(!p_voice->is_suitable_for_fm_voice(), "SiMMLData: Cannot set voice data which is not suitable for FM voices.");

	_fm_voices.write[p_index] = p_voice;
}

SiMMLVoice *SiMMLData::get_pcm_voice(int p_index) {
	int index = p_index & (SiOPMRefTable::PCM_DATA_MAX - 1);
	if (!_pcm_voices[index]) {
		SiMMLVoice *voice = SiMMLVoice::create_blank_pcm_voice(index);
		_pcm_voices.write[index] = voice;
	}

	return _pcm_voices[index];
}

//

void SiMMLData::clear() {
	MMLData::clear();

	for (int i = 0; i < SiMMLRefTable::ENVELOPE_TABLE_MAX; i++) {
		_envelope_tables.write[i] = nullptr;
	}
	for (int i = 0; i < SiMMLRefTable::VOICE_MAX; i++) {
		_fm_voices.write[i] = nullptr;
	}

	for (int i = 0; i < SiOPMRefTable::WAVE_TABLE_MAX; i++) {
		if (_wave_tables[i]) {
			_wave_tables[i]->free();
			_wave_tables.write[i] = nullptr;
		}
	}
	for (int i = 0; i < SiOPMRefTable::PCM_DATA_MAX; i++) {
		if (_pcm_voices[i]) {
			SiOPMWavePCMTable *pcm_table = Object::cast_to<SiOPMWavePCMTable>(_pcm_voices[i]->get_wave_data());
			if (pcm_table) {
				pcm_table->free();
			}

			_pcm_voices.write[i] = nullptr;
		}
	}

	for (int i = 0; i < SiOPMRefTable::SAMPLER_TABLE_MAX; i++) {
		_sampler_tables[i]->free();
	}
}

SiMMLData::SiMMLData() {
	_envelope_tables.resize_zeroed(SiMMLRefTable::ENVELOPE_TABLE_MAX);
	_wave_tables.resize_zeroed(SiOPMRefTable::WAVE_TABLE_MAX);
	_sampler_tables.resize_zeroed(SiOPMRefTable::SAMPLER_TABLE_MAX);

	_fm_voices.resize_zeroed(SiMMLRefTable::VOICE_MAX);
	_pcm_voices.resize_zeroed(SiOPMRefTable::PCM_DATA_MAX);

	for (int i = 0; i < SiOPMRefTable::SAMPLER_TABLE_MAX; i++) {
		_sampler_tables.write[i] = memnew(SiOPMWaveSamplerTable);
	}
}

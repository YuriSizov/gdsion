/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "sion_data.h"

#include "sion_voice.h"
#include "chip/siopm_ref_table.h"
#include "chip/wave/siopm_wave_pcm_data.h"
#include "chip/wave/siopm_wave_pcm_table.h"
#include "chip/wave/siopm_wave_sampler_data.h"
#include "chip/wave/siopm_wave_sampler_table.h"

Ref<SiOPMWavePCMData> SiONData::set_pcm_wave(int p_index, const Variant &p_data, double p_sampling_note, int p_key_range_from, int p_key_range_to, int p_src_channel_count, int p_channel_count) {
	Ref<SiOPMWavePCMTable> pcm_table = get_pcm_voice(p_index)->get_wave_data();
	if (pcm_table.is_valid()) {
		Ref<SiOPMWavePCMData> pcm_data = memnew(SiOPMWavePCMData(p_data, (int)(p_sampling_note * 64), p_src_channel_count, p_channel_count));
		pcm_table->set_key_range_data(pcm_data, p_key_range_from, p_key_range_to);
		return pcm_data;
	}

	return nullptr;
}

void SiONData::set_pcm_voice(int p_index, const Ref<SiONVoice> &p_voice) {
	// Size is expected to be power of 2.
	_pcm_voices.write[p_index & (_pcm_voices.size() - 1)] = p_voice;
}

Ref<SiOPMWaveSamplerData> SiONData::set_sampler_wave(int p_index, const Variant &p_data, bool p_ignore_note_off, int p_pan, int p_src_channel_count, int p_channel_count) {
	int bank = (p_index >> SiOPMRefTable::NOTE_BITS) & (SiOPMRefTable::SAMPLER_TABLE_MAX - 1);
	Ref<SiOPMWaveSamplerData> sampler_data = memnew(SiOPMWaveSamplerData(p_data, p_ignore_note_off, p_pan, p_src_channel_count, p_channel_count));
	_sampler_tables[bank]->set_sample(sampler_data, p_index & (SiOPMRefTable::NOTE_TABLE_SIZE - 1));
	return sampler_data;
}

void SiONData::set_sampler_table(int p_bank, const Ref<SiOPMWaveSamplerTable> &p_table) {
	// Size is expected to be power of 2.
	_sampler_tables.write[p_bank & (_sampler_tables.size() - 1)] = p_table;
}

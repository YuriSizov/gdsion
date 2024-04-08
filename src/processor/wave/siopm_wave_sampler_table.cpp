/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_sampler_table.h"

#include "sion_enums.h"
#include "processor/siopm_ref_table.h"

SiOPMWaveSamplerData *SiOPMWaveSamplerTable::get_sample(int p_sample_number) const {
	if (_stencil && _stencil->_table[p_sample_number]) {
		return _stencil->_table[p_sample_number];
	}

	return _table[p_sample_number];
}

void SiOPMWaveSamplerTable::set_sample(SiOPMWaveSamplerData *p_sample, int p_key_range_from, int p_key_range_to) {
	int key_from = MAX(0, p_key_range_from);
	int key_to = MIN(SiOPMRefTable::SAMPLER_DATA_MAX - 1, p_key_range_to);

	if (key_to == -1) {
		key_to = key_from;
	}

	ERR_FAIL_COND_MSG(key_from > (SiOPMRefTable::SAMPLER_DATA_MAX - 1), vformat("SiOPMWaveSamplerTable: Invalid sample key range, left boundary cannot be greater than %d but %d was given.", (SiOPMRefTable::SAMPLER_DATA_MAX - 1), key_from));
	ERR_FAIL_COND_MSG(key_to < 0, vformat("SiOPMWaveSamplerTable: Invalid sample key range, right boundary cannot be less than 0 (except -1) but %d was given.", key_to));
	ERR_FAIL_COND_MSG(key_to < key_from, vformat("SiOPMWaveSamplerTable: Invalid sample key range, left boundary cannot be greater than right boundary (%d > %d).", key_from, key_to));

	for (int i = key_from; i <= key_to; i++) {
		_table.write[i] = p_sample;
	}
}

void SiOPMWaveSamplerTable::free() {
	for (int i = 0; i < SiOPMRefTable::SAMPLER_DATA_MAX; i++) {
		_table.write[i] = nullptr;
	}
}

void SiOPMWaveSamplerTable::clear(SiOPMWaveSamplerData *p_sample) {
	for (int i = 0; i < SiOPMRefTable::SAMPLER_DATA_MAX; i++) {
		_table.write[i] = p_sample;
	}
}

SiOPMWaveSamplerTable::SiOPMWaveSamplerTable() :
		SiOPMWaveBase(MT_SAMPLE) {
	_table.resize_zeroed(SiOPMRefTable::SAMPLER_DATA_MAX);
	clear();
}

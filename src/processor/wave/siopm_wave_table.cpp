/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_table.h"

#include <godot_cpp/core/memory.hpp>
#include "sion_enums.h"
#include "processor/siopm_ref_table.h"

using namespace godot;

List<SiOPMWaveTable *> SiOPMWaveTable::_free_list;

SiOPMWaveTable *SiOPMWaveTable::alloc(Vector<int> p_wavelet, SiONPitchTableType p_default_pt_type) {
	SiOPMWaveTable *instance = nullptr;
	if (!_free_list.is_empty()) {
		instance = _free_list.back()->get();
		_free_list.pop_back();
	} else {
		instance = memnew(SiOPMWaveTable);
	}
	instance->initialize(p_wavelet, p_default_pt_type);

	return instance;
}

void SiOPMWaveTable::initialize(Vector<int> p_wavelet, SiONPitchTableType p_default_pitch_table_type) {
	_wavelet = p_wavelet;
	_default_pitch_table_type = p_default_pitch_table_type;

	int bits = 0;
	for (int len = _wavelet.size() >> 1; len != 0; len >>= 1) {
		bits++;
	}
	_fixed_bits = SiOPMRefTable::PHASE_BITS - bits;
}

void SiOPMWaveTable::copy_from(SiOPMWaveTable *p_source) {
	_fixed_bits = p_source->_fixed_bits;
	_default_pitch_table_type = p_source->_default_pitch_table_type;
	_wavelet.clear();

	int wavelet_size = p_source->_wavelet.size();
	for (int i = 0; i < wavelet_size; i++) {
		_wavelet.append(p_source->_wavelet[i]);
	}
}

void SiOPMWaveTable::free() {
	_free_list.push_back(this);
}

SiOPMWaveTable::SiOPMWaveTable() :
		SiOPMWaveBase(MT_CUSTOM) {
	// Empty.
}

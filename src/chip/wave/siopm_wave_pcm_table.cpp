/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_wave_pcm_table.h"

#include "sion_enums.h"
#include "chip/siopm_ref_table.h"

Ref<SiOPMWavePCMData> SiOPMWavePCMTable::get_note_data(int p_note) const {
	ERR_FAIL_INDEX_V_MSG(p_note, _note_data_map.size(), nullptr, vformat("SiOPMWavePCMData: Trying to access note data for a note that doesn't exist (%d).", p_note));
	return _note_data_map[p_note];
}

double SiOPMWavePCMTable::get_note_volume(int p_note) const {
	ERR_FAIL_INDEX_V_MSG(p_note, _note_volume_map.size(), 0, vformat("SiOPMWavePCMData: Trying to access note volume for a note that doesn't exist (%d).", p_note));
	return _note_volume_map[p_note];
}

int SiOPMWavePCMTable::get_note_pan(int p_note) const {
	ERR_FAIL_INDEX_V_MSG(p_note, _note_pan_map.size(), 0, vformat("SiOPMWavePCMData: Trying to access note pan for a note that doesn't exist (%d).", p_note));
	return _note_pan_map[p_note];
}

void SiOPMWavePCMTable::set_key_range_data(const Ref<SiOPMWavePCMData> &p_pcm_data, int p_key_range_from, int p_key_range_to) {
	int key_from = MAX(0, p_key_range_from);
	int key_to = MIN(SiOPMRefTable::NOTE_TABLE_SIZE - 1, p_key_range_to);

	if (key_to == -1) {
		key_to = key_from;
	}

	ERR_FAIL_COND_MSG(key_from > (SiOPMRefTable::NOTE_TABLE_SIZE - 1), vformat("SiOPMWavePCMTable: Invalid sample key range, left boundary cannot be greater than %d but %d was given.", (SiOPMRefTable::NOTE_TABLE_SIZE - 1), key_from));
	ERR_FAIL_COND_MSG(key_to < 0, vformat("SiOPMWavePCMTable: Invalid sample key range, right boundary cannot be less than 0 (except -1) but %d was given.", key_to));
	ERR_FAIL_COND_MSG(key_to < key_from, vformat("SiOPMWavePCMTable: Invalid sample key range, left boundary cannot be greater than right boundary (%d > %d).", key_from, key_to));

	for (int i = key_from; i <= key_to; i++) {
		_note_data_map.write[i] = p_pcm_data;
	}
}

void SiOPMWavePCMTable::set_key_scale_volume(int p_center_note, double p_key_range, double p_volume_range) {
	double volume_range = p_volume_range * 0.0078125;

	int min_range = p_center_note - p_key_range * 0.5;
	int max_range = p_center_note + p_key_range * 0.5;
	double delta_value = (p_key_range == 0 ? volume_range : (volume_range / p_key_range));

	if (volume_range > 0) {
		double value = 1 - volume_range;

		int i = 0;
		for (; i < min_range; i++) {
			_note_volume_map.write[i] = value;
		}
		for (; i < max_range; i++) {
			_note_volume_map.write[i] = value;
			value += delta_value;
		}
		for (; i < SiOPMRefTable::NOTE_TABLE_SIZE; i++) {
			_note_volume_map.write[i] = 1;
		}
	} else {
		double value = 1;

		int i = 0;
		for (; i < min_range; i++) {
			_note_volume_map.write[i] = 1;
		}
		for (; i < max_range; i++) {
			_note_volume_map.write[i] = value;
			value += delta_value;
		}

		value = 1 + volume_range;
		for (; i < SiOPMRefTable::NOTE_TABLE_SIZE; i++) {
			_note_volume_map.write[i] = value;
		}
	}
}

void SiOPMWavePCMTable::set_key_scale_pan(int p_center_note, double p_key_range, double p_pan_width) {
	int min_range = p_center_note - p_key_range * 0.5;
	int max_range = p_center_note + p_key_range * 0.5;
	double delta_value = (p_key_range == 0 ? p_pan_width : (p_pan_width / p_key_range));
	double value = -p_pan_width * 0.5;

	int i = 0;
	for (; i < min_range; i++) {
		_note_pan_map.write[i] = value;
	}
	for (; i < max_range; i++) {
		_note_pan_map.write[i] = value;
		value += delta_value;
	}

	value = p_pan_width * 0.5;
	for (; i < SiOPMRefTable::NOTE_TABLE_SIZE; i++) {
		_note_pan_map.write[i] = value;
	}
}

void SiOPMWavePCMTable::clear() {
	for (int i = 0; i < SiOPMRefTable::NOTE_TABLE_SIZE; i++) {
		_note_data_map.write[i] = Ref<SiOPMWavePCMData>();
		_note_volume_map.write[i] = 1;
		_note_pan_map.write[i] = 0;
	}
}

SiOPMWavePCMTable::SiOPMWavePCMTable() :
		SiOPMWaveBase(SiONModuleType::MODULE_PCM) {
	_note_data_map.resize_zeroed(SiOPMRefTable::NOTE_TABLE_SIZE);
	_note_volume_map.resize_zeroed(SiOPMRefTable::NOTE_TABLE_SIZE);
	_note_pan_map.resize_zeroed(SiOPMRefTable::NOTE_TABLE_SIZE);

	clear();
}

SiOPMWavePCMTable::~SiOPMWavePCMTable() {
	_note_data_map.clear();
}

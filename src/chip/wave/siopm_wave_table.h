/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_TABLE_H
#define SIOPM_WAVE_TABLE_H

#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "sion_enums.h"
#include "chip/wave/siopm_wave_base.h"

using namespace godot;

class SiOPMWaveTable : public SiOPMWaveBase {
	GDCLASS(SiOPMWaveTable, SiOPMWaveBase)

	Vector<int> _wavelet;
	int _fixed_bits = 0;
	SiONPitchTableType _default_pitch_table_type = SiONPitchTableType::PITCH_TABLE_OPM;

protected:
	static void _bind_methods() {}

public:
	Vector<int> get_wavelet() const { return _wavelet; }
	int get_fixed_bits() const { return _fixed_bits; }
	SiONPitchTableType get_default_pitch_table_type() const { return _default_pitch_table_type; }

	//

	void initialize(Vector<int> p_wavelet, SiONPitchTableType p_default_pt_type = SiONPitchTableType::PITCH_TABLE_OPM);
	void copy_from(const Ref<SiOPMWaveTable> &p_source);

	SiOPMWaveTable(Vector<int> p_wavelet = Vector<int>(), SiONPitchTableType p_default_pitch_table_type = SiONPitchTableType::PITCH_TABLE_OPM);
	~SiOPMWaveTable() {}
};

#endif // SIOPM_WAVE_TABLE_H

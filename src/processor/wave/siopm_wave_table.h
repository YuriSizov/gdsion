/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_TABLE_H
#define SIOPM_WAVE_TABLE_H

#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "processor/wave/siopm_wave_base.h"

using namespace godot;

class SiOPMWaveTable : public SiOPMWaveBase {
	GDCLASS(SiOPMWaveTable, SiOPMWaveBase)

	static List<SiOPMWaveTable *> _free_list;

	Vector<int> _wavelet;
	int _fixed_bits = 0;
	int _default_pitch_table_type = 0;

protected:
	static void _bind_methods() {}

public:
	static SiOPMWaveTable *alloc(Vector<int> p_wavelet, int p_default_pitch_table_type = 0);

	Vector<int> get_wavelet() const { return _wavelet; }
	int get_fixed_bits() const { return _fixed_bits; }
	int get_default_pitch_table_type() const { return _default_pitch_table_type; }

	//

	void initialize(Vector<int> p_wavelet, int p_default_pt_type = 0);
	void copy_from(SiOPMWaveTable *p_source);
	void free();

	SiOPMWaveTable();
	~SiOPMWaveTable() {}
};

#endif // SIOPM_WAVE_TABLE_H

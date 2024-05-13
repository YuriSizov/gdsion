/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_SAMPLER_TABLE_H
#define SIOPM_WAVE_SAMPLER_TABLE_H

#include <godot_cpp/templates/vector.hpp>
#include "chip/wave/siopm_wave_base.h"
#include "chip/wave/siopm_wave_sampler_data.h"

using namespace godot;

class SiOPMWaveSamplerTable : public SiOPMWaveBase {
	GDCLASS(SiOPMWaveSamplerTable, SiOPMWaveBase)

	// Stencil table; search sample in stencil table before seaching in this instance's own table.
	SiOPMWaveSamplerTable *_stencil = nullptr;

	Vector<SiOPMWaveSamplerData *> _table;

protected:
	static void _bind_methods() {}

public:
	SiOPMWaveSamplerTable *get_stencil() const { return _stencil; }
	void set_stencil(SiOPMWaveSamplerTable *p_table) { _stencil = p_table; }

	SiOPMWaveSamplerData *get_sample(int p_sample_number) const;
	void set_sample(SiOPMWaveSamplerData *p_sample, int p_key_range_from = 0, int p_key_range_to = -1);

	void free();
	void clear(SiOPMWaveSamplerData *p_sample = nullptr);

	SiOPMWaveSamplerTable();
};

#endif // SIOPM_WAVE_SAMPLER_TABLE_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_BASE_H
#define SIOPM_WAVE_BASE_H

#include <godot_cpp/core/object.hpp>
#include "sequencer/simml_table.h"

using namespace godot;

// Note that original classes also support Flash's native Sound objects.
// This is naturally not relevant here, but it may make sense to support Godot's
// native audio stream objects.

class SiOPMWaveBase : public Object {
	GDCLASS(SiOPMWaveBase, Object)

	SiMMLTable::ModuleType _module_type = SiMMLTable::MT_MAX;

protected:
	static void _bind_methods() {}

public:
	SiMMLTable::ModuleType get_module_type() const { return _module_type; }

	SiOPMWaveBase(SiMMLTable::ModuleType p_module_type = SiMMLTable::MT_MAX);
	~SiOPMWaveBase() {}
};

#endif // SIOPM_WAVE_BASE_H

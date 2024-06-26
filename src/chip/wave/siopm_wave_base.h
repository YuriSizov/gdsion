/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_WAVE_BASE_H
#define SIOPM_WAVE_BASE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include "sion_enums.h"

using namespace godot;

// Note that original classes also support Flash's native Sound objects.
// This is naturally not relevant here, but it may make sense to support Godot's
// native audio stream objects.

class SiOPMWaveBase : public RefCounted {
	GDCLASS(SiOPMWaveBase, RefCounted)

	SiONModuleType _module_type = SiONModuleType::MODULE_MAX;

protected:
	static void _bind_methods() {}

public:
	SiONModuleType get_module_type() const { return _module_type; }

	SiOPMWaveBase(SiONModuleType p_module_type = SiONModuleType::MODULE_MAX);
	~SiOPMWaveBase() {}
};

#endif // SIOPM_WAVE_BASE_H

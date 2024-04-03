/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_SAMPLER_H
#define SIOPM_CHANNEL_SAMPLER_H

#include "processor/channels/siopm_channel_base.h"

// FIXME: NOT IMPLEMENTED
class SiOPMChannelSampler : public SiOPMChannelBase {
	GDCLASS(SiOPMChannelSampler, SiOPMChannelBase)

protected:
	static void _bind_methods() {}

public:
	SiOPMChannelSampler(SiOPMModule *p_chip) : SiOPMChannelBase(p_chip) {}
};

#endif // SIOPM_CHANNEL_SAMPLER_H

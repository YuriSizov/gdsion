/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_PCM_H
#define SIOPM_CHANNEL_PCM_H

#include "processor/channels/siopm_channel_base.h"

// FIXME: NOT IMPLEMENTED
class SiOPMChannelPCM : public SiOPMChannelBase {
	GDCLASS(SiOPMChannelPCM, SiOPMChannelBase)

protected:
	static void _bind_methods() {}

public:
	SiOPMChannelPCM(SiOPMModule *p_chip) : SiOPMChannelBase(p_chip) {}
};

#endif // SIOPM_CHANNEL_PCM_H

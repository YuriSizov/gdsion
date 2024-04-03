/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_KS_H
#define SIOPM_CHANNEL_KS_H

#include "processor/channels/siopm_channel_fm.h"

// Karplus-Strong algorithm with FM synth.
// FIXME: NOT IMPLEMENTED
class SiOPMChannelKS : public SiOPMChannelFM {
	GDCLASS(SiOPMChannelKS, SiOPMChannelFM)

protected:
	static void _bind_methods() {}

public:
	SiOPMChannelKS(SiOPMModule *p_chip) : SiOPMChannelFM(p_chip) {}
};

#endif // SIOPM_CHANNEL_KS_H

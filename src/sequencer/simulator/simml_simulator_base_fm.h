/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_SIMULATOR_BASE_FM_H
#define SIMML_SIMULATOR_BASE_FM_H

#include "sequencer/simml_ref_table.h"
#include "sequencer/simulator/simml_simulator_base.h"

class SiMMLTrack;

// Base class of all FM sound chip simulators.
class SiMMLSimulatorBaseFM : public SiMMLSimulatorBase {

protected:
	MMLSequence *_select_fm_tone(SiMMLTrack *p_track, int p_voice_index);

public:
	virtual MMLSequence *select_tone(SiMMLTrack *p_track, int p_voice_index) override;

	SiMMLSimulatorBaseFM(SiONModuleType p_type, int p_channel_num);
};

#endif // SIMML_SIMULATOR_BASE_FM_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_FILTER_BASE_H
#define SI_FILTER_BASE_H

#include "effector/si_effect_base.h"

class SiFilterBase : public SiEffectBase {
	GDCLASS(SiFilterBase, SiEffectBase)

	static const double THRESHOLD;

	struct ChannelValues {
		double in1 = 0;
		double in2 = 0;
		double out1 = 0;
		double out2 = 0;

		void check_threshold() {
			if (out1 < THRESHOLD) {
				out1 = 0;
				out2 = 0;
			}
		}

		void clear() {
			in1 = 0;
			in2 = 0;
			out1 = 0;
			out2 = 0;
		}
	};

	ChannelValues _left;
	ChannelValues _right;

	double _process_channel(ChannelValues p_channel, double p_input);

protected:
	static void _bind_methods() {}

	double _a1 = 0;
	double _a2 = 0;
	double _b0 = 0;
	double _b1 = 0;
	double _b2 = 0;

public:
	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	SiFilterBase() : SiEffectBase() {}
	~SiFilterBase() {}
};

#endif // SI_FILTER_BASE_H

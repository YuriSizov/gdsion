/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECT_EQUALIZER_H
#define SI_EFFECT_EQUALIZER_H

#include "effector/si_effect_base.h"

class SiEffectEqualizer : public SiEffectBase {
	GDCLASS(SiEffectEqualizer, SiEffectBase)

	// Filter pipes.

	struct PipeChannel {
		double f1p0 = 0;
		double f1p1 = 0;
		double f1p2 = 0;
		double f1p3 = 0;

		double f2p0 = 0;
		double f2p1 = 0;
		double f2p2 = 0;
		double f2p3 = 0;

		double sdm1 = 0;
		double sdm2 = 0;
		double sdm3 = 0;

		void clear() {
			f1p0 = 0;
			f1p1 = 0;
			f1p2 = 0;
			f1p3 = 0;

			f2p0 = 0;
			f2p1 = 0;
			f2p2 = 0;
			f2p3 = 0;

			sdm1 = 0;
			sdm2 = 0;
			sdm3 = 0;
		}
	};

	PipeChannel _left;
	PipeChannel _right;

	// Controls.
	double _low_frequency = 0;
	double _high_frequency = 0;
	double _low_gain = 0;
	double _mid_gain = 0;
	double _high_gain = 0;

	double _process_channel(PipeChannel p_channel, double p_value);
	void _process_mono(Vector<double> *r_buffer, int p_start_index, int p_length);
	void _process_stereo(Vector<double> *r_buffer, int p_start_index, int p_length);

protected:
	static void _bind_methods();

public:
	void set_params(double p_low_gain = 1, double p_mid_gain = 1, double p_high_gain = 1, double p_low_frequency = 880, double p_high_frequency = 5000);

	//

	virtual int prepare_process() override;
	virtual int process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) override;

	virtual void set_by_mml(Vector<double> p_args) override;
	virtual void reset() override;

	SiEffectEqualizer(double p_low_gain = 1, double p_mid_gain = 1, double p_high_gain = 1, double p_low_frequency = 880, double p_high_frequency = 5000);
	~SiEffectEqualizer() {}
};

#endif // SI_EFFECT_EQUALIZER_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "si_effect_stereo_chorus.h"

void SiEffectStereoChorus::set_params(double p_delay_time, double p_feedback, double p_frequency, double p_depth, double p_wet, bool p_invert_phase) {
	ERR_FAIL_COND_MSG(p_delay_time == 0, "SiEffectStereoChorus: Delay cannot be zero.");
	ERR_FAIL_COND_MSG(p_frequency == 0, "SiEffectStereoChorus: Frequency cannot be zero.");
	ERR_FAIL_COND_MSG(p_depth == 0, "SiEffectStereoChorus: Depth cannot be zero.");

	int offset = (int)(p_delay_time * 44.1);
	if (offset > DELAY_BUFFER_FILTER) {
		offset = DELAY_BUFFER_FILTER;
	}

	_pointer_write = (_pointer_read + offset) & DELAY_BUFFER_FILTER;
	_depth = MIN(p_depth, offset - 4);

	_feedback = p_feedback;
	if (_feedback >= 1) {
		_feedback = 0.9990234375;
	} else if (_feedback <= -1) {
		_feedback = -0.9990234375;
	}

	int table_size = (int)(_depth * 6.283185307179586);
	if ((table_size * p_frequency) > 11025) {
		table_size = 11025 / p_frequency;
	}
	_phase_table.resize_zeroed(table_size);

	double depth_step = 6.283185307179586 / table_size;
	double depth_value = 0;
	for (int i = 0; i < table_size; i++) {
		_phase_table.write[i] = (int)(Math::sin(depth_value) * _depth + 0.5);
		depth_value += depth_step;
	}

	_lfo_step = (int)(44100 / (table_size * p_frequency));
	if (_lfo_step < 4) {
		_lfo_step = 4;
	}
	_lfo_residue_step = _lfo_step << 1;

	_wet = p_wet;
	_phase_invert = (p_invert_phase ? -1 : 1);
}

int SiEffectStereoChorus::prepare_process() {
	_lfo_phase = 0;
	_lfo_residue_step = 0;
	_pointer_read = 0;

	_delay_buffer_left.fill(0);
	_delay_buffer_right.fill(0);

	return 2;
}

void SiEffectStereoChorus::_process_channel(Vector<double> *r_buffer, int p_buffer_index, Vector<double> *r_delay_buffer, int p_delay) {
	int delay_index = (_pointer_read + p_delay) & DELAY_BUFFER_FILTER;
	double value = (*r_delay_buffer)[delay_index];
	double next_value = (*r_buffer)[p_buffer_index] - value * _feedback;

	r_delay_buffer->write[_pointer_write] = next_value;
	r_buffer->write[p_buffer_index] *= (1 - _wet);
	r_buffer->write[p_buffer_index] += value * _wet;
}

void SiEffectStereoChorus::_process_lfo(Vector<double> *r_buffer, int p_start_index, int p_length) {
	int delay_left = _phase_table[_lfo_phase];
	int delay_right = _phase_table[_lfo_phase] * _phase_invert;

	for (int i = p_start_index; i < (p_start_index + p_length); i += 2) {
		_process_channel(r_buffer, i, &_delay_buffer_left, delay_left);
		_process_channel(r_buffer, i + 1, &_delay_buffer_right, delay_right);

		_pointer_write = (_pointer_write + 1) & DELAY_BUFFER_FILTER;
		_pointer_read  = (_pointer_read  + 1) & DELAY_BUFFER_FILTER;
	}
}

int SiEffectStereoChorus::process(int p_channels, Vector<double> *r_buffer, int p_start_index, int p_length) {
	int start_index = p_start_index << 1;
	int length = p_length << 1;

	int step = _lfo_residue_step;
	int max = start_index + length;
	int i = start_index;
	while (i < (max - step)) {
		_process_lfo(r_buffer, i, step);

		_lfo_phase++;
		if (_lfo_phase == _phase_table.size()) {
			_lfo_phase = 0;
		}

		i += step;
		step = _lfo_step << 1;
	}

	_process_lfo(r_buffer, i, max - i);
	_lfo_residue_step = step - (max - i);

	return p_channels;
}

void SiEffectStereoChorus::set_by_mml(Vector<double> p_args) {
	double delay_time = _get_mml_arg(p_args, 0, 20);
	double feedback   = _get_mml_arg(p_args, 1, 20) / 100.0;
	double frequency  = _get_mml_arg(p_args, 2, 4);
	double depth      = _get_mml_arg(p_args, 3, 20);
	double wet        = _get_mml_arg(p_args, 4, 50) / 100.0;
	int invert_phase  = _get_mml_arg(p_args, 5, 0);

	set_params(delay_time, feedback, frequency, depth, wet, invert_phase != 0);
}

void SiEffectStereoChorus::reset() {
	set_params();
}

SiEffectStereoChorus::SiEffectStereoChorus(double p_delay_time, double p_feedback, double p_frequency, double p_depth, double p_wet, bool p_invert_phase) :
		SiEffectBase() {
	_delay_buffer_left.resize_zeroed(1 << DELAY_BUFFER_BITS);
	_delay_buffer_right.resize_zeroed(1 << DELAY_BUFFER_BITS);

	set_params(p_delay_time, p_feedback, p_frequency, p_depth, p_wet, p_invert_phase);
}

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_SOUND_CHIP_H
#define SIOPM_SOUND_CHIP_H

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "templates/singly_linked_list.h"

using namespace godot;

class SiOPMOperatorParams;
class SiOPMStream;

class SiOPMSoundChip : public Object {
	GDCLASS(SiOPMSoundChip, Object)

	SiOPMOperatorParams *init_operator_params = nullptr;
	SinglyLinkedList<int> *zero_buffer = nullptr;

	SiOPMStream *output_stream = nullptr;
	// Expected to be of STREAM_SEND_SIZE size.
	Vector<SiOPMStream *> stream_slot;
	double pcm_volume = 4;
	double sampler_volume = 2;

	int _buffer_length = 0;
	int _bitrate = 0;

	// Both expected to be of PIPE_SIZE size.
	Vector<SinglyLinkedList<int> *> _pipe_buffer;
	Vector<List<SinglyLinkedList<int> *>> _pipe_buffer_pager;

protected:
	static void _bind_methods() {}

public:
	static const int STREAM_SEND_SIZE = 8;
	static const int PIPE_SIZE = 5;

	SiOPMOperatorParams *get_init_operator_params() const { return init_operator_params; }
	SinglyLinkedList<int> *get_zero_buffer() const { return zero_buffer; }

	SiOPMStream *get_output_stream() const { return output_stream; }
	Vector<double> get_output_buffer() const;
	void set_output_buffer(Vector<double> p_buffer);
	int get_channel_count() const;

	SiOPMStream *get_stream_slot(int p_slot) const { return stream_slot[p_slot]; }
	void set_stream_slot(int p_slot, SiOPMStream *p_value) { stream_slot.write[p_slot] = p_value; }

	double get_pcm_volume() const { return pcm_volume; }
	double get_sampler_volume() const { return sampler_volume; }

	int get_buffer_length() const { return _buffer_length; }
	int get_bitrate() const { return _bitrate; }

	SinglyLinkedList<int> *get_pipe(int p_pipe_num, int p_index = 0);

	void begin_process();
	void end_process();

	// Initialize module and all tone generators.
	void initialize(int p_channel_count, int p_bitrate, int p_buffer_length);
	void reset();

	SiOPMSoundChip();
	~SiOPMSoundChip() {}
};

#endif // SIOPM_SOUND_CHIP_H

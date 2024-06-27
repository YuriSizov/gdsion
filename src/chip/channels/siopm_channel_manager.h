/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIOPM_CHANNEL_MANAGER_H
#define SIOPM_CHANNEL_MANAGER_H

#include <godot_cpp/templates/hash_map.hpp>

using namespace godot;

class SiOPMChannelBase;
class SiOPMSoundChip;

class SiOPMChannelManager {

public:
	enum ChannelType {
		CHANNEL_FM = 0,
		CHANNEL_PCM = 1,
		CHANNEL_SAMPLER = 2,
		CHANNEL_KS = 3,
		CHANNEL_MAX
	};

private:
	static SiOPMSoundChip *_sound_chip;
	static HashMap<ChannelType, SiOPMChannelManager *> _channel_managers;

	ChannelType _channel_type = ChannelType::CHANNEL_MAX;
	SiOPMChannelBase *_terminator;
	int _length = 0;

	// Returns null when the channel count is overflown.
	SiOPMChannelBase *_create_channel(SiOPMChannelBase *p_prev, int p_buffer_index);
	void _delete_channel(SiOPMChannelBase *p_channel);
	void _initialize_all();
	void _reset_all();

public:
	static void initialize(SiOPMSoundChip *p_chip);
	static void finalize();

	static void initialize_all_channels();
	static void reset_all_channels();

	static SiOPMChannelBase *create_channel(ChannelType p_type, SiOPMChannelBase *p_prev, int p_buffer_index);
	static void delete_channel(SiOPMChannelBase *p_channel);

	int get_length() const { return _length; }

	SiOPMChannelManager(ChannelType p_channel_type);
	~SiOPMChannelManager();
};

#endif // SIOPM_CHANNEL_MANAGER_H

/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "siopm_channel_manager.h"

#include <godot_cpp/core/memory.hpp>
#include "chip/channels/siopm_channel_base.h"
#include "chip/channels/siopm_channel_fm.h"
#include "chip/channels/siopm_channel_ks.h"
#include "chip/channels/siopm_channel_pcm.h"
#include "chip/channels/siopm_channel_sampler.h"
#include "chip/siopm_sound_chip.h"

using namespace godot;

SiOPMSoundChip *SiOPMChannelManager::_sound_chip = nullptr;
HashMap<SiOPMChannelManager::ChannelType, SiOPMChannelManager *> SiOPMChannelManager::_channel_managers;

void SiOPMChannelManager::initialize(SiOPMSoundChip *p_chip) {
	_sound_chip = p_chip;

	_channel_managers[CT_CHANNEL_FM]      = memnew(SiOPMChannelManager(CT_CHANNEL_FM));
	_channel_managers[CT_CHANNEL_PCM]     = memnew(SiOPMChannelManager(CT_CHANNEL_PCM));
	_channel_managers[CT_CHANNEL_SAMPLER] = memnew(SiOPMChannelManager(CT_CHANNEL_SAMPLER));
	_channel_managers[CT_CHANNEL_KS]      = memnew(SiOPMChannelManager(CT_CHANNEL_KS));
}

void SiOPMChannelManager::finalize() {
	_sound_chip = nullptr;

	memdelete(_channel_managers[CT_CHANNEL_FM]);
	memdelete(_channel_managers[CT_CHANNEL_PCM]);
	memdelete(_channel_managers[CT_CHANNEL_SAMPLER]);
	memdelete(_channel_managers[CT_CHANNEL_KS]);
	_channel_managers.clear();
}

void SiOPMChannelManager::initialize_all_channels() {
	for (const KeyValue<ChannelType, SiOPMChannelManager *> &kv : _channel_managers) {
		kv.value->_initialize_all();
	}
}

void SiOPMChannelManager::reset_all_channels() {
	for (const KeyValue<ChannelType, SiOPMChannelManager *> &kv : _channel_managers) {
		kv.value->_reset_all();
	}
}

SiOPMChannelBase *SiOPMChannelManager::create_channel(ChannelType p_type, SiOPMChannelBase *p_prev, int p_buffer_index) {
	return _channel_managers[p_type]->_create_channel(p_prev, p_buffer_index);
}

void SiOPMChannelManager::delete_channel(SiOPMChannelBase *p_channel) {
	_channel_managers[(ChannelType)p_channel->_channel_type]->_delete_channel(p_channel);
}


SiOPMChannelBase *SiOPMChannelManager::_create_channel(SiOPMChannelBase *p_prev, int p_buffer_index) {
	SiOPMChannelBase *new_channel = nullptr;

	if (_terminator->_next->_is_free) {
		// The head channel is free -> The head will be a new channel.
		new_channel = _terminator->_next;
		new_channel->_prev->_next = new_channel->_next;
		new_channel->_next->_prev = new_channel->_prev;
	} else {
		// The head channel is active -> channel overflow.
		// Create new channel.

		switch (_channel_type) {
			case CT_CHANNEL_FM: {
				new_channel = memnew(SiOPMChannelFM(_sound_chip));
			} break;
			case CT_CHANNEL_PCM: {
				new_channel = memnew(SiOPMChannelPCM(_sound_chip));
			} break;
			case CT_CHANNEL_SAMPLER: {
				new_channel = memnew(SiOPMChannelSampler(_sound_chip));
			} break;
			case CT_CHANNEL_KS: {
				new_channel = memnew(SiOPMChannelKS(_sound_chip));
			} break;

			default: break; // Silences enum warnings.
		}

		ERR_FAIL_NULL_V(new_channel, nullptr);
		new_channel->_channel_type = _channel_type;
		_length++;
	}

	// Set new channel to the tail and activate.
	new_channel->_is_free = false;
	new_channel->_prev = _terminator->_prev;
	new_channel->_next = _terminator;
	new_channel->_prev->_next = new_channel;
	new_channel->_next->_prev = new_channel;

	// initialize
	new_channel->initialize(p_prev, p_buffer_index);

	return new_channel;
}

void SiOPMChannelManager::_delete_channel(SiOPMChannelBase *p_channel) {
	p_channel->_is_free = true;
	p_channel->_prev->_next = p_channel->_next;
	p_channel->_next->_prev = p_channel->_prev;
	p_channel->_prev = _terminator;
	p_channel->_next = _terminator->_next;
	p_channel->_prev->_next = p_channel;
	p_channel->_next->_prev = p_channel;
}

void SiOPMChannelManager::_initialize_all() {
	for (SiOPMChannelBase *channel = _terminator->_next; channel != _terminator; channel = channel->_next) {
		channel->_is_free = true;
		channel->initialize(nullptr, 0);
	}
}

void SiOPMChannelManager::_reset_all() {
	for (SiOPMChannelBase *channel = _terminator->_next; channel != _terminator; channel = channel->_next) {
		channel->_is_free = true;
		channel->reset();
	}
}

SiOPMChannelManager::SiOPMChannelManager(ChannelType p_channel_type) {
	_channel_type = p_channel_type;

	_terminator = memnew(SiOPMChannelBase(_sound_chip));
	_terminator->_is_free = false;
	_terminator->_next = _terminator;
	_terminator->_prev = _terminator;
	_length = 0;
}

SiOPMChannelManager::~SiOPMChannelManager() {
	SiOPMChannelBase *channel = _terminator->_next;
	while (channel && channel != _terminator) {
		SiOPMChannelBase *next_channel = channel->_next;
		memdelete(channel);
		channel = next_channel;
	}

	memdelete(_terminator);
}

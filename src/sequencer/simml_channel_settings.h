/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SIMML_CHANNEL_SETTINGS_H
#define SIMML_CHANNEL_SETTINGS_H

#include <godot_cpp/templates/vector.hpp>
#include "sion_enums.h"
#include "chip/channels/siopm_channel_manager.h"

using namespace godot;

class MMLSequence;
class SiMMLTrack;
class SiOPMRefTable;

class SiMMLChannelSettings {

public:
	enum SelectToneType {
		SELECT_TONE_NOP    = 0,
		SELECT_TONE_NORMAL = 1,
		SELECT_TONE_FM     = 2,
	};

private:
	SiOPMRefTable *_table = nullptr;

	int _type = 0;
	SelectToneType _select_tone_type = SELECT_TONE_NORMAL;
	SiOPMChannelManager::ChannelType _channel_type = SiOPMChannelManager::CT_CHANNEL_FM;
	bool _is_suitable_for_fm_voice = true;
	int _default_operator_count = 1;

	Vector<int> _pg_type_list;
	Vector<SiONPitchTableType> _pt_type_list;
	Vector<int> _voice_index_table;
	int _initial_voice_index = 0;

public:
	SelectToneType get_select_tone() const { return _select_tone_type; }
	bool is_select_tone_type(SelectToneType p_type) const { return _select_tone_type == p_type; }
	void set_select_tone_type(SelectToneType p_type) { _select_tone_type = p_type; }

	SiOPMChannelManager::ChannelType get_channel_type() const { return _channel_type; }
	void set_channel_type(SiOPMChannelManager::ChannelType p_type) { _channel_type = p_type; }

	bool is_suitable_for_fm_voice() const { return _is_suitable_for_fm_voice; }
	void set_suitable_for_fm_voice(bool p_suitable) { _is_suitable_for_fm_voice = p_suitable; }

	int initialize_tone(SiMMLTrack *p_track, int p_channel_num, int p_buffer_index);
	MMLSequence *select_tone(SiMMLTrack *p_track, int p_voice_index);

	//

	Vector<int> get_pg_type_list() const { return _pg_type_list; }
	int get_pg_type(int p_index) const;
	void set_pg_type(int p_index, int p_type);

	Vector<SiONPitchTableType> get_pt_type_list() const { return _pt_type_list; }
	SiONPitchTableType get_pt_type(int p_index) const;
	void set_pt_type(int p_index, SiONPitchTableType p_type);

	Vector<int> get_voice_index_table() const { return _voice_index_table; }
	int get_voice_index(int p_index) const;
	void set_voice_index(int p_index, int p_value);
	int get_initial_voice_index() const { return _initial_voice_index; }
	void set_initial_voice_index(int p_value) { _initial_voice_index = p_value; }

	//

	SiMMLChannelSettings(int p_module_type, int p_pg_type, int p_length, int p_step, int p_channel_count);
};

#endif // SIMML_CHANNEL_SETTINGS_H

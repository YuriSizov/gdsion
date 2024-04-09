/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef SI_EFFECTOR_MODULE_H
#define SI_EFFECTOR_MODULE_H

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "effector/si_effect_base.h"

using namespace godot;

class SiEffectStream;
class SiOPMModule;

class SiEffectorModule {

	static HashMap<String, List<Ref<SiEffectBase>>> _effect_instances;

	SiOPMModule *_module = nullptr;

	SiEffectStream *_master_effect = nullptr;;
	List<SiEffectStream *> _free_effect_streams;
	Vector<SiEffectStream *> _local_effects;
	// Expected to be SiOPMModule::STREAM_SEND_SIZE at most.
	Vector<SiEffectStream *> _global_effects;
	int _global_effect_count = 0;

	SiEffectStream *_get_global_effector(int p_slot);
	SiEffectStream *_alloc_stream(int p_depth);

public:
	// Effects.

	int get_global_effect_count() const { return _global_effect_count; }

	template <class T>
	static void register_effector(const String &p_name);
	static Ref<SiEffectBase> get_effector_instance(const String &p_name);
	template <class T>
	static Ref<T> create_effector_instance();

	// Slots and connections.

	void set_slot0(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(0, p_effects); }
	void set_slot1(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(1, p_effects); }
	void set_slot2(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(2, p_effects); }
	void set_slot3(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(3, p_effects); }
	void set_slot4(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(4, p_effects); }
	void set_slot5(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(5, p_effects); }
	void set_slot6(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(6, p_effects); }
	void set_slot7(List<Ref<SiEffectBase>> p_effects) { set_slot_effect_list(7, p_effects); }

	List<Ref<SiEffectBase>> get_slot_effect_list(int p_slot) const;
	void set_slot_effect_list(int p_slot, List<Ref<SiEffectBase>> p_effects);
	void clear_slot(int p_slot);

	void connect_effector(int p_slot, const Ref<SiEffectBase> &p_effector);

	SiEffectStream *create_local_effect(int p_depth, List<Ref<SiEffectBase>> p_effects);
	void delete_local_effect(SiEffectStream *p_effect);

	void parse_global_effect_mml(int p_slot, String p_mml, String p_postfix);

	// Processing.

	void prepare_process();
	void begin_process();
	void end_process();
	void reset();

	//

	void initialize();

	SiEffectorModule(SiOPMModule *p_module);
	~SiEffectorModule() {}
};

#endif // SI_EFFECTOR_MODULE_H

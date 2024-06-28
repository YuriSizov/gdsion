/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#include "mml_executor_connector.h"

#include <godot_cpp/classes/reg_ex.hpp>
#include <godot_cpp/classes/reg_ex_match.hpp>
#include "chip/channels/siopm_channel_base.h"
#include "sequencer/base/mml_event.h"
#include "sequencer/base/mml_sequence.h"
#include "sequencer/base/mml_sequence_group.h"

List<MMLExecutorConnector::MECElement *> MMLExecutorConnector::_free_list;

void MMLExecutorConnector::_free_element(MECElement *p_element) {
	if (p_element->first_child) {
		_free_element(p_element->first_child);
	}
	if (p_element->next) {
		_free_element(p_element->next);
	}

	_free_list.push_back(p_element);
}

MMLExecutorConnector::MECElement *MMLExecutorConnector::_alloc_element(int p_number) {
	MECElement *element = nullptr;
	if (!_free_list.is_empty()) {
		element = _free_list.back()->get();
		_free_list.pop_back();
	} else {
		element = memnew(MECElement);
	}

	element->initialize(p_number);
	return element;
}

void MMLExecutorConnector::_connect(MECElement *p_element, bool p_first_oscillator, int p_out_pipe) {
	// Modulator before carrior.
	int in_pipe = 0;
	if (p_element->first_child) {
		in_pipe = p_out_pipe + (p_first_oscillator ? 0 : 1);
		_connect(p_element->first_child, true, in_pipe);
	}

	// Preprocess and assign sequence.
	MMLSequence *prep_sequence = _connecting_sequence_group->get_new_sequence();
	prep_sequence->initialize();

	// Out pipe.
	if (p_out_pipe != -1) {
		prep_sequence->append_new_event(MMLEvent::OUTPUT_PIPE, (p_first_oscillator ? SiOPMChannelBase::OUTPUT_OVERWRITE : SiOPMChannelBase::OUTPUT_ADD));
		prep_sequence->append_new_event(MMLEvent::PARAMETER,   p_out_pipe);
	} else {
		prep_sequence->append_new_event(MMLEvent::OUTPUT_PIPE, SiOPMChannelBase::OUTPUT_STANDARD);
		prep_sequence->append_new_event(MMLEvent::PARAMETER,   0);
	}

	// In pipe.
	if (p_element->first_child) {
		prep_sequence->append_new_event(MMLEvent::INPUT_PIPE, p_element->modulation);
		prep_sequence->append_new_event(MMLEvent::PARAMETER,  in_pipe);
	} else {
		prep_sequence->append_new_event(MMLEvent::INPUT_PIPE, 0);
		prep_sequence->append_new_event(MMLEvent::PARAMETER,  0);
	}

	prep_sequence->connect_before(_connecting_sequence_list[p_element->number]->get_head_event()->get_next());
	prep_sequence->insert_after(_connecting_sequence);
	_connecting_sequence = prep_sequence;

	// Move to the next oscillator.
	if (p_element->next) {
		_connect(p_element->next, false, p_out_pipe);
	}
}

MMLSequence *MMLExecutorConnector::connect(MMLSequenceGroup *p_seq_group, MMLSequence *p_sequence) {
	_connecting_sequence_group = p_seq_group;
	_connecting_sequence = p_sequence;
	_connecting_sequence_list.clear();

	for (int i = 0; i < _sequence_count; i++) {
		ERR_FAIL_COND_V_MSG(!_connecting_sequence->get_next_sequence(), nullptr, "MMLExecutorConnector: Not enough sequences to connect.");

		_connecting_sequence_list.push_back(_connecting_sequence->get_next_sequence());
		_connecting_sequence->get_next_sequence()->remove_from_chain();
	}

	_connect(_first_element, false, -1);
	return _connecting_sequence;
}

void MMLExecutorConnector::parse(String p_formula) {
	clear();

	MECElement *last_elem = nullptr;

	Ref<RegEx> re_formula = RegEx::create_from_string("(\\()?([a-zA-Z])([0-7])?(\\)+)?");
	TypedArray<RegExMatch> matches = re_formula->search_all(p_formula);
	for (int i = 0; i < matches.size(); i++) {
		Ref<RegExMatch> res = matches[i];

		// We want to have a 0-based index for letters from A to Z.
		String osc_key = res->get_string(2);
		int osc_idx = osc_key.to_lower().unicode_at(0) - 'a';
		ERR_FAIL_INDEX_MSG(osc_idx, 26, vformat("MMLExecutorConnector: Invalid oscillator key '%s' in formula: '%s'", osc_key, p_formula));

		if (_sequence_count <= osc_idx) {
			_sequence_count = osc_idx + 1;
		}
		_executor_count++;

		MECElement *elem = _alloc_element(osc_idx);

		if (!res->get_string(3).is_empty()) {
			elem->modulation = res->get_string(3).to_int();
		} else {
			elem->modulation = 5;
		}

		// Modulation start "(".

		if (!res->get_string(1).is_empty()) {
			ERR_FAIL_COND_MSG(!last_elem, vformat("MMLExecutorConnector: Invalid modulation start '(' in formula: '%s'", p_formula));
			last_elem->first_child = elem;
			elem->parent = last_elem;
		} else {
			if (last_elem) {
				last_elem->next = elem;
				elem->parent = last_elem->parent;
			} else {
				_first_element = elem;
			}
		}

		// Modulation end ")+".
		if (!res->get_string(4).is_empty()) {
			String end_string = res->get_string(4);

			for (int j = 0; j < end_string.length(); j++) {
				ERR_FAIL_COND_MSG(!elem->parent, vformat("MMLExecutorConnector: Invalid modulation end ')' in formula: '%s'", p_formula));
				elem = elem->parent;
			}
		}

		last_elem = elem;
	}

	ERR_FAIL_COND_MSG(!last_elem || last_elem->parent, vformat("MMLExecutorConnector: Invalid formula: '%s'", p_formula));
}

void MMLExecutorConnector::clear() {
	if (_first_element) {
		_free_element(_first_element);
		_first_element = nullptr;
	}

	_executor_count = 0;
	_sequence_count = 0;
}

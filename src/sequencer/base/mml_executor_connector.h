/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_EXECUTOR_CONNECTOR_H
#define MML_EXECUTOR_CONNECTOR_H

#include <godot_cpp/templates/list.hpp>

using namespace godot;

class MMLSequence;
class MMLSequenceGroup;

// Used for #FM connection.
class MMLExecutorConnector {

	struct MECElement {
		int number = 0;
		int modulation = 3;

		MECElement *parent = nullptr;
		MECElement *next = nullptr;
		MECElement *first_child = nullptr;

		void initialize(int p_number) {
			number = p_number;
			modulation = 3;

			parent = nullptr;
			next = nullptr;
			first_child = nullptr;
		}

		MECElement() {}
		~MECElement() {}
	};

	static List<MECElement *> _free_list;

	static void _free_element(MECElement *p_element);
	static MECElement *_alloc_element(int p_number);

	MECElement *_first_element = nullptr;
	int _executor_count = 0;
	int _sequence_count = 0;

	MMLSequenceGroup *_connecting_sequence_group = nullptr;
	MMLSequence *_connecting_sequence = nullptr;
	List<MMLSequence *> _connecting_sequence_list;

	void _connect(MECElement *p_element, bool p_first_oscillator, int p_out_pipe);

public:
	int get_executor_count() const { return _executor_count; }
	int get_sequence_count() const { return _sequence_count; }

	MMLSequence *connect(MMLSequenceGroup *p_seq_group, MMLSequence *p_sequence);
	void parse(String p_formula);
	void clear();

	MMLExecutorConnector() {}
	~MMLExecutorConnector() {}
};

#endif // MML_EXECUTOR_CONNECTOR_H

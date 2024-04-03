/***************************************************/
/* Part of GDSiON software synthesizer             */
/* Copyright (c) 2024 Yuri Sizov and contributors  */
/* Provided under MIT                              */
/***************************************************/

#ifndef MML_SYSTEM_COMMAND_H
#define MML_SYSTEM_COMMAND_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class MMLSystemCommand : public RefCounted {
	GDCLASS(MMLSystemCommand, RefCounted)

protected:
	static void _bind_methods() {}

public:
	// For the given MML string "#ABC5{def}ghi;"...

	String command; // Command name; always starts with "#", e.g. command = "#ABC"
	int number = 0; // Number after command, e.g. number = 5
	String content; // String inside {..}, e.g. content = "def"
	String postfix; // String at the end of the command, e.g. postfix = "ghi"

	MMLSystemCommand() {}
	~MMLSystemCommand() {}
};

#endif // MML_SYSTEM_COMMAND_H

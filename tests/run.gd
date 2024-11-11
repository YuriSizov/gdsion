###################################################
# Part of GDSiON tests                            #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends SceneTree

const TestBase := preload("res://TestBase.gd")
const RUN_ROOT := "./run"

var tests_total: int = 0
var tests_success: int = 0
var asserts_total: int = 0
var asserts_success: int = 0

var start_time: int = -1
var start_memory: int = -1


func _init():
	# Delay everything by one frame so the initialization is complete before we run tests.
	await process_frame

	print("")
	print_rich("[color=gray]=========== RUNNING GDSION TESTS ===========[/color]")
	print("")

	var fs := DirAccess.open(RUN_ROOT)
	if not fs:
		_quit_fatal("Fatal Error: Unable to open the run root at '%s' (code %d)." % [ RUN_ROOT, DirAccess.get_open_error() ])
		return

	var error := fs.list_dir_begin()
	if error != OK:
		_quit_fatal("Fatal Error: Unable to list files at '%s' (code %d)." % [ RUN_ROOT, DirAccess.get_open_error() ])
		return

	# Passing `-- --case script_filename_sans_ext` should only run that one script.
	# Multiple `--case script_filename_sans_ext` entries are also allowed.
	var selected_scripts := _get_args_scripts()

	start_time = Time.get_ticks_msec()
	start_memory = OS.get_static_memory_usage()

	var file_name := fs.get_next()
	while not file_name.is_empty():
		if fs.dir_exists(file_name):
			file_name = fs.get_next() # Advance.
			continue # This is a directory, skipping.

		var script_base := file_name.get_basename()
		var script_name := RUN_ROOT.path_join(file_name)
		file_name = fs.get_next() # Advance.

		if not selected_scripts.is_empty() && not selected_scripts.has(script_base):
			continue

		tests_total += 1

		var script: GDScript = load(script_name)
		if not script:
			printerr("Warning: Failed to load script at '%s'." % [ script_name ])
			continue

		var script_instance: TestBase = script.new()
		script_instance.prepare()
		await script_instance.run(self)

		script_instance.print_output()
		print_verbose("")

		asserts_total += script_instance.asserts_total
		asserts_success += script_instance.asserts_success
		if script_instance.asserts_total == script_instance.asserts_success:
			tests_success += 1

	fs.list_dir_end()
	_quit_with_status()


func _get_args_scripts() -> PackedStringArray:
	var scripts := PackedStringArray()

	var args := OS.get_cmdline_user_args()
	var i := 0
	while i < args.size():
		var arg_key := args[i]

		if arg_key == "--case":
			i += 1

			var arg_value := args[i]
			if not arg_value.begins_with("--"):
				scripts.push_back(arg_value)

		i += 1

	return scripts


func _quit_fatal(message: String) -> void:
	print("")
	print_rich("[color=gray]=========== FAILED GDSION TESTS ===========[/color]")
	printerr(message)
	print("")

	quit(2)


func _quit_with_status() -> void:
	print("")
	print_rich("[color=gray]=========== FINISHED GDSION TESTS ===========[/color]")
	print("")

	print_rich("[color=blue]Tests run[/color]:\t\t[b]%d[/b]" % [ tests_total ])
	print_rich("[color=blue]Tests successful[/color]:\t[b]%d[/b]" % [ tests_success ])
	print_rich("[color=blue]Asserts made[/color]:\t\t[b]%d[/b]" % [ asserts_total ])
	print_rich("[color=blue]Asserts successful[/color]:\t[b]%d[/b]" % [ asserts_success ])
	print("")

	var success := tests_success == tests_total
	var execution_time := Time.get_ticks_msec() - start_time
	var final_memory := OS.get_static_memory_usage()

	print_rich("[color=blue]Status:[/color]\t\t\t[b]%s[/b]" % [ "[color=green]SUCCESS[/color]" if success else "[color=red]FAILURE[/color]" ])
	print_rich("[color=gray]Time:\t\t\t[b]%.3f sec[/b][/color]" % [ execution_time / 1000.0 ])
	print_rich("[color=gray]Memory:\t\t\t[b]%s[/b] / [b]%s[/b][/color]" % [ String.humanize_size(start_memory), String.humanize_size(final_memory) ])

	print("")
	quit(0 if success else 1)

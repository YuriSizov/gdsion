###################################################
# Part of GDSiON example project                  #
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


func _init():
	# Delay everything by one frame so the initialization is complete before we run tests.
	await process_frame

	print("")
	print("=========== RUNNING GDSION TESTS ===========")
	print("")

	var fs := DirAccess.open(RUN_ROOT)
	if not fs:
		_quit_fatal("Fatal Error: Unable to open the run root at '%s' (code %d)." % [ RUN_ROOT, DirAccess.get_open_error() ])
		return

	var error := fs.list_dir_begin()
	if error != OK:
		_quit_fatal("Fatal Error: Unable to list files at '%s' (code %d)." % [ RUN_ROOT, DirAccess.get_open_error() ])
		return

	var file_name := fs.get_next()
	while not file_name.is_empty():
		var script_name := RUN_ROOT.path_join(file_name)
		file_name = fs.get_next() # Advance.

		tests_total += 1

		var script: GDScript = load(script_name)
		if not script:
			printerr("Warning: Failed to load script at '%s'." % [ script_name ])
			continue

		var script_instance: TestBase = script.new()
		script_instance.prepare()
		await script_instance.run(self)

		asserts_total += script_instance.asserts_total
		asserts_success += script_instance.asserts_success
		if asserts_total == asserts_success:
			tests_success += 1

	fs.list_dir_end()
	_quit_with_status()


func _quit_fatal(message: String) -> void:
	print("")
	print("=========== FAILED GDSION TESTS ===========")
	printerr(message)
	print("")

	quit(2)


func _quit_with_status() -> void:
	print("")
	print("=========== FINISHED GDSION TESTS ===========")
	print("")

	var success := tests_success == tests_total

	print("Tests run:\t\t%d" % [ tests_total ])
	print("Tests successful:\t%d" % [ tests_success ])
	print("Asserts made:\t\t%d" % [ asserts_total ])
	print("Asserts successful:\t%d" % [ asserts_success ])

	print("Status:\t\t\t%s" % [ "SUCCESS" if success else "FAILURE" ])

	print("")
	quit(0 if success else 1)

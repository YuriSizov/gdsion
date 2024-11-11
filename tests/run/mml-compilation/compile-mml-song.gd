###################################################
# Part of GDSiON tests                            #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends SceneTree

const INPUTS_PATH := "./run/mml-compilation/inputs"


func _init():
	# Delay everything by one frame so the initialization is complete before we run tests.
	await process_frame

	var mml_string := _get_mml_string()

	var driver := SiONDriver.create()
	root.add_child(driver)
	await process_frame

	driver.compile(mml_string)

	driver.get_parent().remove_child(driver)
	driver.free()

	quit(0)


func _get_mml_string() -> String:
	var args := OS.get_cmdline_user_args()
	if args.is_empty():
		return ""

	var input_path := INPUTS_PATH.path_join("%s.mml" % [ args[0] ])
	var file := FileAccess.open(input_path, FileAccess.READ)
	if not file:
		return ""

	file.seek(0)
	return file.get_as_text(true)

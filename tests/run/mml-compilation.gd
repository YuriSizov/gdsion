###################################################
# Part of GDSiON tests                            #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends "res://TestBase.gd"

var group: String = "MML"
var name: String = "Compilation"

signal loading_completed()

const TUNES_URL := "https://raw.githubusercontent.com/YuriSizov/SiONMML/refs/heads/master/mmltalks_mml.json"
const OUTPUTS_PATH := "./run/mml-compilation/outputs"
const INPUTS_PATH := "./run/mml-compilation/inputs"

var _tunes: Array[TuneItem] = []


func run(scene_tree: SceneTree) -> void:
	var driver := SiONDriver.create()
	scene_tree.root.add_child(driver)
	var http_request := HTTPRequest.new()
	scene_tree.root.add_child(http_request)

	await scene_tree.process_frame

	http_request.request_completed.connect(_load_tunes_completed)

	var error := http_request.request(TUNES_URL)
	if _assert_equal("request sent", error, OK):
		await loading_completed

		# Ensure that the folder exists.
		var fs := DirAccess.open(".")
		fs.make_dir_recursive(INPUTS_PATH)

		for tune_item in _tunes:
			var tune_hash := tune_item.mml_string.hash()
			var input_path := INPUTS_PATH.path_join("%d.mml" % [ tune_hash ])
			var output_path := OUTPUTS_PATH.path_join("%d.txt" % [ tune_hash ])

			# Store the MML in a file to avoid any weirdness from passing it via CLI arguments.
			_store_tune_mml(input_path, tune_item.mml_string)

			# This is painfully slow, but allows us to read the errors and match them against the expected result.
			var received := _run_subscript("./run/mml-compilation/compile-mml-song.gd", [ tune_hash ])
			received = _extract_godot_errors(received)
			var expected := _load_tune_output(output_path)
			expected = _extract_godot_errors(expected)

			var errors_match := (received == expected)

			if expected.is_empty():
				_assert_equal("compiled w/o errors: %s - %s" % [ tune_item.title, tune_item.author ], errors_match, true)
			else:
				_assert_equal("compiled w/  errors: %s - %s" % [ tune_item.title, tune_item.author ], errors_match, true)

			if not errors_match:
				_append_extra_to_output("Expected:")
				_append_extra_to_output(expected)
				_append_extra_to_output("Got:")
				_append_extra_to_output(received)

	# Cleanup.

	driver.get_parent().remove_child(driver)
	driver.free()
	http_request.get_parent().remove_child(http_request)
	http_request.queue_free()


func _load_tunes_completed(result: int, response_code: int, _headers: PackedStringArray, body: PackedByteArray) -> void:
	if not _assert_equal("request received", result, HTTPRequest.RESULT_SUCCESS):
		loading_completed.emit()
		return

	if not _assert_equal("request code", response_code, 200):
		loading_completed.emit()
		return

	var raw_data := body.get_string_from_utf8()
	var data: Variant = JSON.parse_string(raw_data)
	if not _assert_equal("request data", typeof(data), TYPE_ARRAY):
		loading_completed.emit()
		return

	var mml_tunes: Array = data
	mml_tunes.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		if a["title"] == b["title"]:
			return a["author"] < b["author"]
		return a["title"] < b["title"]
	)

	for mml_tune: Dictionary in mml_tunes:
		var tune_item := TuneItem.new()
		tune_item.title = mml_tune["title"]
		tune_item.author = mml_tune["author"]
		tune_item.mml_string = mml_tune["mml"]

		_tunes.push_back(tune_item)

	loading_completed.emit()


func _store_tune_mml(file_name: String, mml: String) -> void:
	var file := FileAccess.open(file_name, FileAccess.WRITE)
	file.resize(0)

	file.store_string(mml)


func _store_tune_output(file_name: String, output: String) -> void:
	if output.is_empty():
		return

	var file := FileAccess.open(file_name, FileAccess.WRITE)
	file.resize(0)

	file.store_string(output)


func _load_tune_output(file_name: String) -> String:
	var file := FileAccess.open(file_name, FileAccess.READ)
	if not file:
		return ""

	file.seek(0)
	return file.get_as_text(true)


class TuneItem:
	var title: String = ""
	var author: String = ""
	var mml_string: String = ""

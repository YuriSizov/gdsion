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

		for tune_item in _tunes:
			# Currently there is no easy way to validate the actual results. We could possibly
			# read the sequences from the data object, serialize them somehow and compare just
			# like we compare the voice data. But for now the main purpose is to highlight the
			# errors raised by the translator util and MML parser.

			prints("Compiling", tune_item.title, "by", tune_item.author)
			driver.compile(tune_item.mml_string)

	# Cleanup.

	driver.get_parent().remove_child(driver)
	driver.free()
	http_request.get_parent().remove_child(http_request)
	http_request.queue_free()


func _load_tunes_completed(result: int, response_code: int, _headers: PackedStringArray, body: PackedByteArray) -> void:
	if not _assert_equal("request success", result, HTTPRequest.RESULT_SUCCESS):
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


class TuneItem:
	var title: String = ""
	var author: String = ""
	var mml_string: String = ""

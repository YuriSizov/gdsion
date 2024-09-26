###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends VBoxContainer

const TUNES_URL := "https://raw.githubusercontent.com/YuriSizov/SiONMML/refs/heads/master/mmltalks_mml.json"

@onready var _tune_list: ItemList = %TuneList
@onready var _tune_status: Label = %TuneStatus
@onready var _play_button: Button = %PlayButton
@onready var _stop_button: Button = %StopButton

@onready var _http_request: HTTPRequest = %HTTPRequest

var _tunes: Array[TuneItem] = []


func _ready() -> void:
	_update_tunes()

	visibility_changed.connect(_set_view_active)

	_play_button.pressed.connect(_play_selected)
	_stop_button.pressed.connect(_stop_playback)

	_http_request.request_completed.connect(_load_tunes_completed)


func _set_view_active() -> void:
	if not visible:
		_tune_status.text = ""
		_tune_status.tooltip_text = ""
		return


# Playback.

func _play_selected() -> void:
	var selected_items := _tune_list.get_selected_items()
	if selected_items.is_empty():
		return

	var selected_tune := _tunes[selected_items[0]]
	Controller.music_player.play_tune(selected_tune.mml_string)
	if selected_tune.author.is_empty():
		_tune_status.text = "Now Playing: %s" % [ selected_tune.title ]
	else:
		_tune_status.text = "Now Playing: %s by %s" % [ selected_tune.title, selected_tune.author ]
	_tune_status.tooltip_text = _tune_status.text


func _stop_playback() -> void:
	Controller.music_player.stop()
	_tune_status.text = ""
	_tune_status.tooltip_text = ""


# Tune management.

func _update_tune_list() -> void:
	# Remember current selection.
	var restore_selected := -1
	var selected_items := _tune_list.get_selected_items()
	if not selected_items.is_empty():
		restore_selected = selected_items[0]

	# Rebuild the list.

	_tune_list.clear()

	for tune_item in _tunes:
		var item_index := _tune_list.add_item(tune_item.title)

		if tune_item.author.is_empty():
			_tune_list.set_item_tooltip(item_index, "Title: %s" % [ tune_item.title ])
		else:
			_tune_list.set_item_tooltip(item_index, "Title: %s\nBy: %s" % [ tune_item.title, tune_item.author ])

	# Restore old selection if available.
	if restore_selected != -1:
		_tune_list.select(restore_selected)
	else:
		_tune_list.select(0)


func _update_tunes() -> void:
	_tunes.clear()

	# Test command from the original implementation of SiON.
	var default_tune := TuneItem.new()
	default_tune.title = "SiON Test Tune"
	default_tune.mml_string = "t100 l8 [ ccggaag4 ffeeddc4 | [ggffeed4]2 ]2"
	_tunes.push_back(default_tune)

	_update_tune_list()
	_load_tunes()


func _load_tunes() -> void:
	var error := _http_request.request(TUNES_URL)
	if error != OK:
		printerr("TunesView: Failed to make a web request (code %d)." % [ error ])


func _load_tunes_completed(result: int, response_code: int, _headers: PackedStringArray, body: PackedByteArray) -> void:
	if result != HTTPRequest.RESULT_SUCCESS:
		printerr("TunesView: The web request failed (code %d)." % [ result ])
		return

	if response_code != 200:
		printerr("TunesView: The web request returned an error (status %d)." % [ response_code ])
		return

	var raw_data := body.get_string_from_utf8()
	var data: Variant = JSON.parse_string(raw_data)
	if typeof(data) != TYPE_ARRAY:
		printerr("TunesView: The data received from the web request has incorrect type (type %d)." % [ typeof(data) ])
		return

	var mml_tunes: Array = data
	mml_tunes.sort_custom(func(a: Dictionary, b: Dictionary) -> bool:
		if a["title"] == b["title"]:
			return a["author"] < b["author"]
		return a["title"] < b["title"]
	)

	print("TunesView: Loaded %d MML tunes." % [ mml_tunes.size() ])

	for mml_tune: Dictionary in mml_tunes:
		var tune_item := TuneItem.new()
		tune_item.title = mml_tune["title"]
		tune_item.author = mml_tune["author"]
		tune_item.mml_string = mml_tune["mml"]

		_tunes.push_back(tune_item)

	_update_tune_list()


class TuneItem:
	var title: String = ""
	var author: String = ""
	var mml_string: String = ""

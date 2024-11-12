###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

@tool
extends Control

const FONT := preload("res://fonts/Kenney Future Narrow.ttf")

var _keys: Array = []
var _notes: PackedStringArray = PackedStringArray()

var _normal_key_boxes: Array[KeyBox] = []
var _sharp_key_boxes: Array[KeyBox] = []
var _active_key: int = -1

var _mouse_pressed: bool = false

var is_drumkit: bool = false


func _init() -> void:
	_update_notes()
	_update_keys()

	resized.connect(_update_positions)
	resized.connect(queue_redraw)


func _ready() -> void:
	_update_instrument()
	_update_positions()

	if not Engine.is_editor_hint():
		Controller.music_player.instrument_changed.connect(_update_instrument)


func _draw() -> void:
	_draw_key_boxes(_normal_key_boxes, Color.ANTIQUE_WHITE, Color.BLACK, Color.BLACK, true)
	_draw_key_boxes(_sharp_key_boxes, Color.BLACK, Color.WEB_GRAY, Color.WHITE, false)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton:
		var mb: InputEventMouseButton = event
		if mb.pressed && not _mouse_pressed:
			_mouse_pressed = true
			_update_mouse_event(mb.position)
		elif not mb.pressed && _mouse_pressed:
			_mouse_pressed = false
			_clear_active_key()

	if event is InputEventMouseMotion && _mouse_pressed:
		var mm: InputEventMouseMotion = event
		_update_mouse_event(mm.position)


# Initialization.

func _update_notes() -> void:
	_notes.clear()

	for i in 144:
		_notes.push_back("")

	for i in 11:
		_notes[(i * 12) + 0] = "C";
		_notes[(i * 12) + 1] = "C#";
		_notes[(i * 12) + 2] = "D";
		_notes[(i * 12) + 3] = "D#";
		_notes[(i * 12) + 4] = "E";
		_notes[(i * 12) + 5] = "F";
		_notes[(i * 12) + 6] = "F#";
		_notes[(i * 12) + 7] = "G";
		_notes[(i * 12) + 8] = "G#";
		_notes[(i * 12) + 9] = "A";
		_notes[(i * 12) + 10] = "A#";
		_notes[(i * 12) + 11] = "B";


func _update_keys() -> void:
	if is_drumkit:
		var instrument := Controller.music_player.get_active_instrument()
		var active_drumkit := Controller.voice_manager.get_drumkit(instrument.type)

		_keys.resize(active_drumkit.size)
		for i in active_drumkit.size:
			_keys[i] = i
	else:
		_keys.resize(256)
		_keys.fill(-1)

		const scale_layout := [ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ]
		var scale_size := scale_layout.size()

		var key_count := 0
		var key_idx := 0
		var scale_idx := 0
		var last_note := 0

		while last_note < 104:
			_keys[key_idx] = last_note
			key_count += 1

			last_note += scale_layout[scale_idx]

			key_idx += 1
			scale_idx += 1
			if scale_idx >= scale_size:
				scale_idx -= scale_size

		_keys.resize(key_count)


func _update_instrument() -> void:
	if Engine.is_editor_hint():
		return

	var instrument := Controller.music_player.get_active_instrument()
	is_drumkit = instrument.type != 0

	_update_keys()
	_update_positions()
	queue_redraw()


# Layout and drawing.

func _update_positions() -> void:
	_normal_key_boxes.clear()
	_sharp_key_boxes.clear()

	var active_drumkit: Drumkit = null
	var normal_key_count := 0

	if is_drumkit:
		var instrument := Controller.music_player.get_active_instrument()
		active_drumkit = Controller.voice_manager.get_drumkit(instrument.type)
		normal_key_count = _keys.size()
	else:
		for i in _keys.size():
			var key: int = _keys[i]
			var note_name := _notes[key]
			if not note_name.ends_with("#"):
				normal_key_count += 1

	# Split available space evenly, but don't make resulting keys too wide.
	var key_width: int = min(size.x / normal_key_count, 120)
	var key_total_width := key_width * normal_key_count

	var offset_idx := -1
	for i in _keys.size():
		var key: int = _keys[i]
		var note_name := ""
		var sharp_note := false

		if is_drumkit:
			note_name = active_drumkit.voice_name[key]
		else:
			note_name = _notes[key]
			sharp_note = note_name.ends_with("#")

		if not sharp_note:
			offset_idx += 1

		var key_position := Vector2((size.x - key_total_width) / 2, 0) + Vector2(offset_idx * key_width, 0)
		var key_size := Vector2(key_width, size.y)

		if sharp_note:
			key_size.x *= 0.75
			key_size.y /= 2
			key_position.x += key_width - key_size.x / 2
			key_position.y += 2

		var key_box := KeyBox.new(Rect2(key_position, key_size), note_name, i)
		if sharp_note:
			_sharp_key_boxes.push_back(key_box)
		else:
			_normal_key_boxes.push_back(key_box)


func _draw_key_boxes(key_boxes: Array[KeyBox], main_color: Color, secondary_color: Color, font_color: Color, darken_on_active: bool) -> void:
	for key_box in key_boxes:
		var fill_color := main_color
		var border_color := secondary_color

		if key_box.index == _active_key:
			if darken_on_active:
				fill_color = fill_color.darkened(0.3)
			else:
				fill_color = fill_color.lightened(0.4)

		draw_rect(key_box.rect, fill_color)
		draw_rect(key_box.rect, border_color, false, 2)

		var label_position := Vector2(
			key_box.rect.position.x - key_box.rect.size.x / 2,
			key_box.rect.end.y - 6
		)
		draw_string(FONT, label_position, key_box.note_name, HORIZONTAL_ALIGNMENT_CENTER, key_box.rect.size.x * 2, 12, font_color)


# Input.

func _update_mouse_event(mouse_position: Vector2) -> void:
	# Check sharp keys first, since they are drawn on top.
	for key_box in _sharp_key_boxes:
		if key_box.rect.has_point(mouse_position):
			_update_active_key(key_box.index)
			return # Consume.

	# Then normal keys.
	for key_box in _normal_key_boxes:
		if key_box.rect.has_point(mouse_position):
			_update_active_key(key_box.index)
			return # Consume.


func _update_active_key(key: int) -> void:
	if _active_key == key:
		return

	if _active_key != -1:
		_clear_active_key()
	_active_key = key

	# Notes are played continuously.
	Controller.music_player.play_note(_active_key)
	queue_redraw()

func _clear_active_key() -> void:
	if _active_key != -1:
		var instrument := Controller.music_player.get_active_instrument()
		if instrument.voice_held:
			Controller.music_player.stop_note(_active_key)

	_active_key = -1
	queue_redraw()


class KeyBox:
	var rect: Rect2 = Rect2()
	var note_name: String = ""
	var index: int = -1


	func _init(rect_: Rect2, note_name_: String, index_: int) -> void:
		rect = rect_
		note_name = note_name_
		index = index_

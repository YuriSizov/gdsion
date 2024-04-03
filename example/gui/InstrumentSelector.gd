###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends HBoxContainer

@onready var _label := $Label
@onready var _prev_button := $LeftButton
@onready var _next_button := $RightButton


func _ready() -> void:
	_update_instrument_label()
	Controller.music_player.instrument_changed.connect(_update_instrument_label)

	_prev_button.pressed.connect(Controller.music_player.change_instrument.bind(-1))
	_next_button.pressed.connect(Controller.music_player.change_instrument.bind(1))


func _update_instrument_label() -> void:
	var instrument := Controller.music_player.get_active_instrument()
	_label.text = "%s: %s" % [ instrument.category, instrument.name ]

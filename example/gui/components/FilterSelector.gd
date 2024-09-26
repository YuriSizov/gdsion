###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends HBoxContainer

@onready var _label := $Label
@onready var _prev_button := $LeftButton
@onready var _next_button := $RightButton
@onready var _power_slider := $PowerSlider


func _ready() -> void:
	_update_filter_label()
	Controller.music_player.filter_changed.connect(_update_filter_label)

	_prev_button.pressed.connect(Controller.music_player.change_filter.bind(-1))
	_next_button.pressed.connect(Controller.music_player.change_filter.bind(1))
	_power_slider.value_changed.connect(Controller.music_player.change_filter_power)


func _update_filter_label() -> void:
	var filter_name := Controller.music_player.get_active_filter()
	_label.text = "%s" % [ filter_name ]

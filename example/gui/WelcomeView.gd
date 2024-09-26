###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends VBoxContainer

@onready var _piano_view_button: Button = %PianoButton
@onready var _tunes_view_button: Button = %TunesButton


func _ready() -> void:
	_piano_view_button.pressed.connect(Controller.change_app_view.bind(Controller.AppView.PIANO_VIEW))
	_tunes_view_button.pressed.connect(Controller.change_app_view.bind(Controller.AppView.TUNES_VIEW))

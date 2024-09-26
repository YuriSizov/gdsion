###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends Control

@onready var _view_selector: HBoxContainer = %ViewSelector
@onready var _view_selector_filler: Control = %ViewSelectorFiller
@onready var _piano_view_button: Button = %PianoButton
@onready var _tunes_view_button: Button = %TunesButton

@onready var _welcome_view: Control = %WelcomeView
@onready var _piano_view: Control = %PianoView
@onready var _tunes_view: Control = %TunesView

@onready var _kenney_label := %KenneyLabel

var _current_view: Controller.AppView = Controller.AppView.WELCOME_VIEW


func _ready() -> void:
	get_window().min_size = Vector2(640, 360)
	_update_active_view(Controller.AppView.WELCOME_VIEW, true)

	_piano_view_button.pressed.connect(Controller.change_app_view.bind(Controller.AppView.PIANO_VIEW))
	_tunes_view_button.pressed.connect(Controller.change_app_view.bind(Controller.AppView.TUNES_VIEW))
	_kenney_label.meta_clicked.connect(_handle_meta_clicked)

	Controller.app_view_changed.connect(_update_active_view.bind(false))


func _handle_meta_clicked(meta: Variant) -> void:
	OS.shell_open(str(meta))


func _update_active_view(next_view: Controller.AppView, force_update: bool) -> void:
	if _current_view == next_view && not force_update:
		return
	_current_view = next_view

	Controller.music_player.stop()

	if _current_view == Controller.AppView.WELCOME_VIEW:
		_view_selector.visible = false
		_view_selector_filler.visible = true

		_welcome_view.visible = true
		_piano_view.visible = false
		_tunes_view.visible = false
		return

	_view_selector.visible = true
	_view_selector_filler.visible = false

	_welcome_view.visible = false
	_piano_view.visible = (_current_view == Controller.AppView.PIANO_VIEW)
	_tunes_view.visible = (_current_view == Controller.AppView.TUNES_VIEW)

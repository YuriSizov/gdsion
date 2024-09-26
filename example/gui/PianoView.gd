###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends VBoxContainer


func _ready() -> void:
	visibility_changed.connect(_set_view_active)


func _set_view_active() -> void:
	if not visible:
		return

	Controller.music_player.start_streaming()

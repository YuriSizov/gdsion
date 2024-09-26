###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

## The main controller component, orchestrating (sic!) application's processes.
extends Node

signal app_view_changed(next_view: AppView)

enum AppView {
	WELCOME_VIEW,
	PIANO_VIEW,
	TUNES_VIEW,
}

var voice_manager: VoiceManager = null
var music_player: MusicPlayer = null

# Global settings.
var buffer_size: int = 2048
var bpm: int = 120


func _init() -> void:
	voice_manager = VoiceManager.new()
	music_player = MusicPlayer.new(self)


func _notification(what: int) -> void:
	if what == NOTIFICATION_PREDELETE:
		voice_manager.free()
		music_player.free()


func _ready() -> void:
	# Driver must be ready by this time.

	music_player.initialize()


# Navigation.

func change_app_view(view: AppView) -> void:
	app_view_changed.emit(view)

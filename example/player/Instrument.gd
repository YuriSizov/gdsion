###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

## An object representing a musical instrument.
class_name Instrument extends RefCounted

var category: String = ""
var name: String = ""
var type: int = 0

var volume: int = 0
var cutoff: int = 0
var resonance: int = 0

var voice: SiONVoice = null
var voice_held: bool = false


func _init() -> void:
	voice = SiONVoice.new()
	clear()


func clear() -> void:
	category = ""
	name = ""
	type = 0

	volume = 256
	cutoff = 128
	resonance = 0

	voice_held = false


func set_filter(cutoff_: int, resonance_: int) -> void:
	cutoff = cutoff_
	resonance = resonance_


func update_filter() -> void:
	if not voice:
		return

	if voice.velocity != volume:
		voice.update_volumes = true
		voice.velocity = volume

	if (voice.get_channel_params().filter_cutoff != cutoff || voice.get_channel_params().filter_resonance != resonance):
		voice.set_filter_envelope(0, cutoff, resonance)


func change_filter_to(cutoff_: int, resonance_: int, volume_: int) -> void:
	if not voice:
		return

	voice.update_volumes = true
	voice.velocity = volume_
	voice.set_filter_envelope(0, cutoff_, resonance_)


func change_volume_to(volume_: int) -> void:
	if not voice:
		return

	voice.update_volumes = true
	voice.velocity = volume_

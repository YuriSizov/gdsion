###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

## An object representing a drumkit.
class_name Drumkit extends Object

var kit_name: String = ""
var size: int = 0

var voice_list: Array[SiONVoice] = []
var voice_name: PackedStringArray
var voice_note: PackedInt32Array
var midi_voice: PackedInt32Array


func update_filter(cutoff: int, resonance: int) -> void:
	for i in size:
		var channel_params := voice_list[i].get_channel_params()
		if (channel_params.filter_cutoff != cutoff || channel_params.filter_resonance != resonance):
			voice_list[i].set_filter_envelope(0, cutoff, resonance)


func update_volume(volume: int) -> void:
	for i in size:
		var voice := voice_list[i]
		if voice.velocity != volume:
			voice.update_volumes = true
			voice.velocity = volume

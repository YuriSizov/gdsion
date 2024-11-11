###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends Control

const VU_COUNT := 24
const FREQ_MAX := 11050.0
const MIN_DB := 60

const RENDER_SIZE := Vector2(860, 320)
const SPACE_WIDTH := 8
const LOW_COLOR := Color.SEA_GREEN
const HIGH_COLOR := Color.ORANGE_RED
const RENDER_LERP := 0.2

var _audio_spectrum: AudioEffectSpectrumAnalyzerInstance

var _prev_heights := PackedFloat32Array()
var _prev_colors := PackedColorArray()


func _ready() -> void:
	_audio_spectrum = AudioServer.get_bus_effect_instance(0, 0)


func _process(_delta: float) -> void:
	queue_redraw()


func _draw() -> void:
	if not _audio_spectrum:
		return

	var offset := (size - RENDER_SIZE) / 2
	var w := RENDER_SIZE.x / VU_COUNT
	var prev_hz := 0.0

	var new_heights := PackedFloat32Array()
	var new_colors := PackedColorArray()

	for i in range(1, VU_COUNT + 1):
		var hz := i * FREQ_MAX / VU_COUNT;

		# FIXME: This method can return garbage data after the playback has ended.
		# This is because it uses some historic records to produce the result, and evidently
		# when the playback stops, this historic data never gets overridden, resulting in
		# ghost spikes in the spectrum. This is a Godot bug, but we can address it locally by
		# only updating the spectrum when we have playback going on. That's a good change
		# either way.
		var magnitude_stereo := _audio_spectrum.get_magnitude_for_frequency_range(prev_hz, hz)
		if not magnitude_stereo.is_finite(): # Edge case when something is wrong with the audio.
			magnitude_stereo = Vector2.ZERO

		var magnitude := magnitude_stereo.length()
		var height := _get_height(i - 1, magnitude)
		var color := _get_color(i - 1, height)

		@warning_ignore("integer_division")
		var bar_position := Vector2(
			offset.x + SPACE_WIDTH / 2 + w * (i - 1),
			offset.y + RENDER_SIZE.y - height
		)
		var bar_size := Vector2(w - SPACE_WIDTH, height)

		draw_rect(Rect2(bar_position, bar_size), color)

		new_heights.push_back(height)
		new_colors.push_back(color)

		prev_hz = hz

	_prev_heights = new_heights
	_prev_colors = new_colors


func _get_height(index: int, magnitude: float) -> float:
	var base_value: float = clamp((MIN_DB + linear_to_db(magnitude)) / MIN_DB, 0, 1) * RENDER_SIZE.y

	if _prev_heights.is_empty():
		return base_value

	return lerp(_prev_heights[index], base_value, RENDER_LERP)


func _get_color(index: int, value: float) -> Color:
	var low_value := 0
	var high_value := RENDER_SIZE.y
	var base_value := LOW_COLOR.lerp(HIGH_COLOR, (value - low_value) / (high_value - low_value))

	if _prev_colors.is_empty():
		return base_value

	return lerp(_prev_colors[index], base_value, RENDER_LERP)

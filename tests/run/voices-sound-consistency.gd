###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends "res://TestBase.gd"

var group: String = "SiONVoice"
var name: String = "Sound Consistency"

const SAMPLE_LENGTH := 2 # In 1/16th of a beat.
const SAMPLE_RUNS := 3
const INT16_MAX := 32767

const RUN_COLORS := [
	[ 255, 0, 0 ], [ 0, 255, 128 ], [ 0, 128, 255 ],
]

var _sample_data: PackedInt32Array = PackedInt32Array()
var _sample_runs: Array[PackedInt32Array] = []


func run(scene_tree: SceneTree) -> void:
	var driver := SiONDriver.create()
	driver.set_timer_interval(SAMPLE_LENGTH)
	scene_tree.root.add_child(driver)

	await scene_tree.process_frame

	var voice_preset_util := SiONVoicePresetUtil.new()
	var voice_list := voice_preset_util.get_voice_preset_keys()

	for voice_name in voice_list:

		var voice := voice_preset_util.get_voice_preset(voice_name)

		_sample_runs.clear()
		for i in SAMPLE_RUNS:
			await _sample_voice(driver, voice)
			await scene_tree.process_frame

			_sample_runs.push_back(_sample_data)

		_validate_sampled_voices(voice_name)

		# Uncomment to dump the first run into the data folder to use as a reference.
		#_create_voice_data(voice_name)

		# Uncomment for visual debugging, but be careful, as it takes a lot of time
		# to draw all of the voices (over 600)!
		#_create_voice_graph(voice_name)

	# Cleanup.

	voice_preset_util.free()
	driver.get_parent().remove_child(driver)
	driver.free()


func _sample_voice(driver: SiONDriver, voice: SiONVoice) -> void:
	_sample_data.clear()

	# Hardcoding for now, but some voices may need a different value and length.
	var note_value := 60
	var note_length := 1 * SAMPLE_LENGTH

	driver.play(null, false)
	driver.streaming.connect(_collect_streamed_data)
	driver.set_stream_event_enabled(true)

	# The first tick happens before streaming is fully started, so we need to compensate.
	await driver.timer_interval

	driver.note_on(note_value, voice, note_length)

	var time_remaining := note_length
	while time_remaining > 0:
		await driver.timer_interval
		time_remaining -= 1

	driver.set_stream_event_enabled(false)
	driver.streaming.disconnect(_collect_streamed_data)
	driver.stop()


func _validate_sampled_voices(voice_name: String) -> void:
	var file_name := "res://voice_data/%s.dat" % [ voice_name ]
	var file := FileAccess.open(file_name, FileAccess.READ)

	file.seek(0)
	var file_buffer := file.get_buffer(file.get_length())
	var reference_data := file_buffer.to_int32_array()

	for run_data in _sample_runs:
		_assert_equal("sample length - %s" % [ voice_name ], run_data.size(), reference_data.size())

		var misses_count := 0
		for i in reference_data.size():
			var run_sample := run_data[i]
			var ref_sample := reference_data[i]

			if run_sample != ref_sample:
				misses_count += 1

		_assert_equal("sample misses - %s" % [ voice_name ], misses_count, 0)


func _create_voice_data(voice_name: String) -> void:
	var file_name := "res://voice_data/%s.dat" % [ voice_name ]
	var file := FileAccess.open(file_name, FileAccess.WRITE)
	file.resize(0)

	var reference_data := _sample_runs[0]
	for sample in reference_data:
		file.store_32(sample)


func _create_voice_graph(voice_name: String) -> void:
	var image_data := PackedByteArray()
	var image_width := _sample_data.size()
	var image_height := 1024
	var image_data_length := image_width * image_height * 4

	image_data.resize(image_data_length)

	# Fill with the clear color.
	var i := 0
	while i < image_data_length:
		image_data[i + 0] = 0
		image_data[i + 1] = 0
		image_data[i + 2] = 0
		image_data[i + 3] = 255

		i += 4

	for pixel_x in image_width:
		var pixel_y := image_height / 2
		var pixel_i := (pixel_x + pixel_y * image_width) * 4

		image_data[pixel_i + 0] = 128
		image_data[pixel_i + 1] = 128
		image_data[pixel_i + 2] = 0
		image_data[pixel_i + 3] = 127

	# Calculate and draw points for the graph.

	for run_idx in _sample_runs.size():
		var run_data := _sample_runs[run_idx]
		var run_color: Array = RUN_COLORS[run_idx]

		for pixel_x in run_data.size():
			var sample := run_data[pixel_x]

			var pixel_y := roundi(sample * image_height) + image_height / 2 + run_idx
			var pixel_i := (pixel_x + pixel_y * image_width) * 4

			image_data[pixel_i + 0] = run_color[0]
			image_data[pixel_i + 1] = run_color[1]
			image_data[pixel_i + 2] = run_color[2]
			image_data[pixel_i + 3] = 255

	# Save the graph to the file.

	var image := Image.create_from_data(image_width, image_height, false, Image.FORMAT_RGBA8, image_data)
	var image_file := "res://voice_graphs/%s.graph.png" % [ voice_name ]

	var error := image.save_png(image_file)
	if error != OK:
		printerr("Failed to save the graph at '%s' (code %d)." % [ image_file, error ])


func _collect_streamed_data(event: SiONEvent) -> void:
	var data := event.get_stream_buffer()
	# The data is in stereo, but there is no panning, so we can use any single channel instead.
	for sample in data:
		_sample_data.push_back(int(sample.x * INT16_MAX))

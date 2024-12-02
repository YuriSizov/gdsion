###################################################
# Part of GDSiON tests                            #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends "res://TestBase.gd"

var group: String = "SiONDriver"
var name: String = "Driver Lifecycle"


func run(scene_tree: SceneTree) -> void:
	var driver := SiONDriver.create()
	_assert_equal("driver buffer",      driver.get_buffer_length(), 2048)
	_assert_equal("driver channels",    driver.get_channel_num(),   2)
	_assert_equal("driver sample rate", driver.get_sample_rate(),   44100)
	_assert_equal("driver bitrate",     driver.get_bitrate(),       0)

	driver.set_bpm(120)
	_assert_equal("driver bpm", driver.get_bpm(), 120)

	scene_tree.root.add_child(driver)
	await scene_tree.process_frame

	_assert_not_null("driver parent", driver.get_parent())
	_assert_not_null("audio player",  driver.get_audio_player())

	driver.stream()
	await scene_tree.process_frame

	_assert_equal("on play: audio playing",     driver.get_audio_player().is_playing(), true)
	_assert_not_null("on play: audio stream",   driver.get_audio_stream())
	_assert_not_null("on play: audio playback", driver.get_audio_playback())

	await scene_tree.process_frame
	driver.stop()
	await scene_tree.process_frame

	_assert_equal("on stop: audio playing",   driver.get_audio_player().is_playing(), false)
	_assert_not_null("on stop: audio stream", driver.get_audio_stream())
	_assert_null("on stop: audio playback",   driver.get_audio_playback())

	# Cleanup.

	driver.get_parent().remove_child(driver)
	driver.free()

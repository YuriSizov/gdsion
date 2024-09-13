###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

extends "res://TestBase.gd"

var group: String = "SiONDriver"
var name: String = "Driver Lifecycle"


func run(scene_tree: SceneTree) -> void:
	var driver := SiONDriver.create()
	_assert_equal(driver.get_buffer_length(), 2048)
	_assert_equal(driver.get_channel_num(),   2)
	_assert_equal(driver.get_sample_rate(),   44100)
	_assert_equal(driver.get_bitrate(),       0)

	scene_tree.root.add_child(driver)
	await scene_tree.process_frame

	_assert_not_null(driver.get_parent())
	_assert_not_null(driver.get_audio_player())

	driver.get_parent().remove_child(driver)
	driver.queue_free()

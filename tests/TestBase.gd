###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

class_name TestBase extends RefCounted

static var test_counter: int = 0

var asserts_total: int = 0
var asserts_success: int = 0


# Called automatically before run().
func prepare() -> void:
	test_counter += 1

	var test_group: String = get("group")
	if test_group.is_empty():
		test_group = "General"

	var test_name: String = get("name")
	if test_name.is_empty():
		test_name = "Unnamed"

	print("%d. [%s] %s" % [ test_counter, test_group, test_name ])


# Called automatically. Must be implemented by individual test scripts.
func run(_scene_tree: SceneTree) -> void:
	pass


# Assert helpers.

func _assert_equal(value: Variant, against: Variant) -> void:
	asserts_total += 1

	if value == against:
		asserts_success += 1
	else:
		printerr("Assert Failed: '%s' != '%s'" % [ value, against ])


func _assert_not_null(value: Object) -> void:
	asserts_total += 1

	if value != null:
		asserts_success += 1
	else:
		printerr("Assert Failed: '%s' is null" % [ value ])

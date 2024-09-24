###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

class_name TestBase extends RefCounted

static var test_counter: int = 0

var asserts_total: int = 0
var asserts_success: int = 0

var _start_time: int = -1

class OutputBufferRecord:
	var success: bool = true
	var label: String = ""
	var result: String = ""
	var tabs: int = 1

var _output_buffer: Array[OutputBufferRecord] = []


# Called automatically before run().
func prepare() -> void:
	test_counter += 1

	var test_group: String = get("group")
	if test_group.is_empty():
		test_group = "General"

	var test_name: String = get("name")
	if test_name.is_empty():
		test_name = "Unnamed"

	print_rich("[bgcolor=gray]%d. [%s] %s[/bgcolor]" % [ test_counter, test_group, test_name ])
	_start_time = Time.get_ticks_msec()


# Called automatically. Must be implemented by individual test scripts.
func run(_scene_tree: SceneTree) -> void:
	pass


# Called automatically after run().
func print_output() -> void:
	var execution_time := Time.get_ticks_msec() - _start_time

	var max_tabs := 0
	for record in _output_buffer:
		var plain_string := ""
		if record.success && OS.is_stdout_verbose():
			plain_string = "OK: [%s]" % [ record.label ]
		elif not record.success:
			plain_string = "FAIL: [%s]" % [ record.label ]

		# Tab size is always expected to be 8 characters long in any shell, because
		# ASCII rendering depends on it, so nobody dares to change it for compatibility's
		# sake.
		record.tabs = floori(plain_string.length() / 8.0)
		if record.tabs > max_tabs:
			max_tabs = record.tabs

	# Always at least one unit of padding added.
	max_tabs += 1

	for record in _output_buffer:
		var tabs_sep := "\t".repeat(max_tabs - record.tabs)

		if record.success && OS.is_stdout_verbose():
			print_rich("[color=green]OK[/color]: [[color=gray]%s[/color]]%s%s" % [ record.label, tabs_sep, record.result ])
		elif not record.success:
			print_rich("[color=red]FAIL[/color]: [[color=gray]%s[/color]]%s%s" % [ record.label, tabs_sep, record.result ])

	print_rich("[color=blue]TOTAL[/color]: [b]%d[/b] / [b]%d[/b]" % [ asserts_success, asserts_total ])
	print_rich("[color=gray]Finished in %.3f sec." % [ execution_time / 1000.0 ])


func _print_ok(label: String, result: String) -> void:
	var record := OutputBufferRecord.new()
	record.success = true
	record.label = label
	record.result = result

	_output_buffer.push_back(record)


func _print_fail(label: String, result: String) -> void:
	var record := OutputBufferRecord.new()
	record.success = false
	record.label = label
	record.result = result

	_output_buffer.push_back(record)


# Assert helpers.

func _assert_equal(label: String, value: Variant, against: Variant) -> void:
	asserts_total += 1

	if value == against:
		asserts_success += 1
		_print_ok(label, "'%s' == '%s'" % [ value, against ])
	else:
		_print_fail(label, "'%s' != '%s'" % [ value, against ])


func _assert_null(label: String, value: Object) -> void:
	asserts_total += 1

	if value == null:
		asserts_success += 1
		_print_ok(label, "value is null")
	else:
		_print_fail(label, "value is %s" % [ value ])


func _assert_not_null(label: String, value: Object) -> void:
	asserts_total += 1

	if value != null:
		asserts_success += 1
		_print_ok(label, "value is %s" % [ value ])
	else:
		_print_fail(label, "value is null")

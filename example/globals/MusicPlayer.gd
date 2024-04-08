###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

## The music player component, responsible for producing sounds using SiON and
## input data.
class_name MusicPlayer extends Object

signal instrument_changed()

const AVAILABLE_INSTRUMENTS := [
	{ "category": "MIDI", "name": "Grand Piano" },
	{ "category": "MIDI", "name": "Glockenspiel" },
	{ "category": "MIDI", "name": "Xylophone" },
	{ "category": "MIDI", "name": "Rock Organ" },
	{ "category": "MIDI", "name": "Accordion" },
	{ "category": "MIDI", "name": "Nylon Guitar" },
	{ "category": "MIDI", "name": "Electric Guitar" },
	{ "category": "MIDI", "name": "Trumpet" },
	{ "category": "MIDI", "name": "Oboe" },

	{ "category": "CHIPTUNE", "name": "Square Wave" },
	{ "category": "CHIPTUNE", "name": "Triangle LO-FI" },

	{ "category": "DRUMKIT", "name": "Midi Drumkit" },
]

var _driver: SiONDriver = null
var _active_instrument: Instrument = null
var _active_instrument_index: int = 0


func _init(controller: Node) -> void:
	_driver = SiONDriver.create(controller.buffer_size)
	controller.add_child(_driver)


# Initialization.

func initialize() -> void:
	_driver.set_timer_interval(1)
	_driver.set_bpm(Controller.bpm)

	_active_instrument = Instrument.new()
	_update_instrument()
	instrument_changed.emit()


func _update_instrument() -> void:
	_active_instrument.clear()

	var preset: Dictionary = AVAILABLE_INSTRUMENTS[_active_instrument_index]
	var voice_data := Controller.voice_manager.get_voice_data(preset["category"], preset["name"])
	if not voice_data:
		return

	_active_instrument.type = voice_data.type
	_active_instrument.category = voice_data.category
	_active_instrument.name = voice_data.name
	_active_instrument.palette = voice_data.palette

	if _active_instrument.type == 0: # Drumkits don't use presets.
		_active_instrument.voice = Controller.voice_manager.get_voice_preset(voice_data.preset)
	else:
		_active_instrument.voice = null

	_active_instrument.update_filter()


# Configuration.

func get_active_instrument() -> Instrument:
	return _active_instrument


func change_instrument(shift: int) -> void:
	_active_instrument_index += shift
	if _active_instrument_index < 0:
		_active_instrument_index = AVAILABLE_INSTRUMENTS.size() - 1
	elif _active_instrument_index >= AVAILABLE_INSTRUMENTS.size():
		_active_instrument_index = 0

	_update_instrument()
	instrument_changed.emit()


# Output and streaming control.

func start_driver() -> void:
	# Driver is always playing/streaming. We decide when to feed it notes on our side.
	_driver.play(null, false)
	print("Driver is streaming.")


func play_tune(mml_command: String) -> void:
	print("Playing MML tune: '%s'" % mml_command)
	_driver.play(mml_command)


func play_note(note: int, length: int) -> void:
	# All instruments, except drumkits.
	if _active_instrument.type == 0:
		_active_instrument.update_filter()
		_driver.note_on(note, _active_instrument.voice, length)

	# Drumkits.
	else:
		var active_drumkit := Controller.voice_manager.get_drumkit(_active_instrument.type)
		if note >= active_drumkit.size:
			return

		active_drumkit.update_filter(_active_instrument.cutoff, _active_instrument.resonance)
		active_drumkit.update_volume(_active_instrument.volume)
		_driver.note_on(active_drumkit.voice_note[note], active_drumkit.voice_list[note], length)

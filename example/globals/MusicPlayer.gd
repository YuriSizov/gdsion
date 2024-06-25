###################################################
# Part of GDSiON example project                  #
# Copyright (c) 2024 Yuri Sizov and contributors  #
# Provided under MIT                              #
###################################################

## The music player component, responsible for producing sounds using SiON and
## input data.
class_name MusicPlayer extends Object

signal instrument_changed()
signal filter_changed()

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

	{ "category": "BELL", "name": "Glocken 1" },

	{ "category": "CHIPTUNE", "name": "Square Wave" },
	{ "category": "CHIPTUNE", "name": "Triangle LO-FI" },

	{ "category": "DRUMKIT", "name": "Simple Drumkit" },
]

const AVAILABLE_FILTERS := [
	"DELAY",
	"CHORUS",
	"REVERB",
	"DISTORTION",
	"LOW BOOST",
	"COMPRESSOR",
	"HIGH PASS",
]

var _driver: SiONDriver = null
var _active_instrument: Instrument = null
var _active_instrument_index: int = 0

var _active_filter_index: int = 0
var _active_filter_power: int = 0


func _init(controller: Node) -> void:
	_driver = SiONDriver.create(controller.buffer_size)
	controller.add_child(_driver)

	print("Created synthesizer driver (v%s-%s)" % [ SiONDriver.get_version(), SiONDriver.get_version_flavor() ])


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


func _update_filter() -> void:
	_driver.get_effector().clear_slot_effects(0)
	if _active_filter_power <= 5:
		return

	match _active_filter_index:
		0:
			var effect_delay := SiEffectStereoDelay.new()
			effect_delay.set_params((300.0 * _active_filter_power) / 100.0, 0.1, false)
			_driver.get_effector().add_slot_effect(0, effect_delay);

		1:
			var effect_chorus := SiEffectStereoChorus.new()
			effect_chorus.set_params(20, 0.2, 4, 10 + ((50.0 * _active_filter_power) / 100.0))
			_driver.get_effector().add_slot_effect(0, effect_chorus);

		2:
			var effect_reverb := SiEffectStereoReverb.new()
			effect_reverb.set_params(0.7, 0.4 + ((0.5 * _active_filter_power) / 100.0), 0.8, 0.3)
			_driver.get_effector().add_slot_effect(0, effect_reverb);

		3:
			var effect_distortion := SiEffectDistortion.new()
			effect_distortion.set_params(-20 - ((80.0 * _active_filter_power) / 100.0), 18, 2400, 1)
			_driver.get_effector().add_slot_effect(0, effect_distortion);

		4:
			var effect_lowboost := SiFilterLowBoost.new()
			effect_lowboost.set_params(3000, 1, 4 + ((6.0 * _active_filter_power) / 100.0))
			_driver.get_effector().add_slot_effect(0, effect_lowboost);

		5:
			var effect_compressor := SiEffectCompressor.new()
			effect_compressor.set_params(0.7, 50, 20, 20, -6, 0.2 + ((0.6 * _active_filter_power) / 100.0))
			_driver.get_effector().add_slot_effect(0, effect_compressor);

		6:
			var effect_highpass := SiControllableFilterHighPass.new()
			effect_highpass.set_params_manually(((1.0 * _active_filter_power) / 100.0), 0.9)
			_driver.get_effector().add_slot_effect(0, effect_highpass);


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


func get_active_filter() -> String:
	return AVAILABLE_FILTERS[_active_filter_index]


func change_filter(shift: int) -> void:
	_active_filter_index += shift
	if _active_filter_index < 0:
		_active_filter_index = AVAILABLE_FILTERS.size() - 1
	elif _active_filter_index >= AVAILABLE_FILTERS.size():
		_active_filter_index = 0

	_update_filter()
	filter_changed.emit()


func change_filter_power(value: int) -> void:
	_active_filter_power = clampi(value, 0, 100)

	_update_filter()
	filter_changed.emit()


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

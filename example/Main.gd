extends Control

# Test command from the original implementation of SiON.
const TEST_TUNE := "t100 l8 [ ccggaag4 ffeeddc4 | [ggffeed4]2 ]2"

@onready var _play_button := $PlayButton
@onready var _kenney_label := $KenneyLabel


func _ready() -> void:
	get_window().min_size = Vector2(640, 360)
	
	_play_button.pressed.connect(Controller.music_player.play_tune.bind(TEST_TUNE))
	_kenney_label.meta_clicked.connect(_handle_meta_clicked)


func _handle_meta_clicked(meta: Variant) -> void:
	OS.shell_open(str(meta))

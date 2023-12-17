extends SceneTree

func _initialize():
    print("_initialize")

    var pc = PanelContainer.new()
    root.add_child(pc)
    pc.set_anchors_and_margins_preset(Control.PRESET_WIDE)

    var vb = VBoxContainer.new()
    pc.add_child(vb)

    var b = Button.new()
    vb.add_child(b)
    b.text = "test"





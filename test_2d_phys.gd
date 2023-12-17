extends SceneTree

func _initialize():
    print("_initialize")

    debug_collisions_hint = true

    # Ground
    var static_body = StaticBody2D.new()
    root.add_child(static_body)
    static_body.transform.origin = Vector2(500, 621)

    var cs = CollisionShape2D.new()
    static_body.add_child(cs)
    cs.shape = RectangleShape2D.new()
    cs.shape.extents = Vector2(1000, 40)

    # "player"
    var rigid_body = RigidBody2D.new()
    root.add_child(rigid_body)
    rigid_body.transform.origin = Vector2(500, 0)
    rigid_body.angular_velocity = 0.1

    cs = CollisionShape2D.new()
    rigid_body.add_child(cs)
    cs.shape = CapsuleShape2D.new()
    cs.shape.radius = 145
    cs.shape.height = 108

    var s = Sprite.new()
    rigid_body.add_child(s)

    var img = Image.new()
    img.load("icon.png")
    print(img)
    print(img.get_size())

    var tex = ImageTexture.new()
    tex.create_from_image(img)
    s.texture = tex



    

    






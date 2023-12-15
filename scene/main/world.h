#ifndef WORLD_H
#define WORLD_H

#include "core/containers/vector.h"
#include "core/object/reference.h"

#include "node.h"

class World2D;

class World : public Node {
	GDCLASS(World, Node);

public:
	Ref<World2D> get_world_2d() const;
	void set_world_2d(const Ref<World2D> &p_world_2d);

	bool get_override_in_parent_viewport();
	void set_override_in_parent_viewport(const bool value);

	//If a World has an override these are overridden
	Ref<World2D> find_world_2d() const;
	Ref<World2D> find_world_2d_no_override() const;

	World *get_override_world();
	World *get_override_world_or_this();
	void set_override_world(World *p_world);
	void set_override_world_bind(Node *p_world);

	// If a world has an override the override defers these to the overridden
	// As these are called by the child Nodes of the overriding world

	// Viewport root
	// -- World w1
	// ---- PanelContainer pc

	// w1 is set as override for the root.
	// pc's CanvasItem will use the canvas in w1, and will likely call gui_reset_canvas_sort_index() in w1 at least some point.
	// What we want, is to defer this call to the root viewport, as that is the class that is able to deal with it.
	// This is what makes overrides transparent to nodes like the Camera, as they can just use the world that they are a child of
	// and overrides can be handled for them in the background.

	virtual void gui_reset_canvas_sort_index();
	virtual int gui_get_canvas_sort_index();

	virtual void enable_canvas_transform_override(bool p_enable);
	virtual bool is_canvas_transform_override_enbled() const;

	virtual void set_canvas_transform_override(const Transform2D &p_transform);
	virtual Transform2D get_canvas_transform_override() const;

	virtual void set_canvas_transform(const Transform2D &p_transform);
	virtual Transform2D get_canvas_transform() const;

	virtual void set_global_canvas_transform(const Transform2D &p_transform);
	virtual Transform2D get_global_canvas_transform() const;

	virtual Transform2D get_final_transform() const;

	virtual Rect2 get_visible_rect() const;

	virtual RID get_viewport_rid() const;

	Vector2 get_camera_coords(const Vector2 &p_viewport_coords) const;
	Vector2 get_camera_rect_size() const;

	void update_worlds();

	World();
	~World();

	virtual void _update_listener_2d();

protected:
	virtual void _on_set_world_2d(const Ref<World2D> &p_old_world_2d);

	virtual void _on_before_world_override_changed();
	virtual void _on_after_world_override_changed();

	void _propagate_enter_world(Node *p_node);
	void _propagate_exit_world(Node *p_node);

	void _notification(int p_what);

	static void _bind_methods();

	Ref<World2D> world_2d;

	World *_parent_world;

	bool _override_in_parent_viewport;
	World *_override_world;
	World *_overriding_world;

	//override canvas layers vec remove /add on override set, and unset

	Size2 size;
};

#endif

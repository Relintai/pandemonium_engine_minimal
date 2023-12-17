#ifndef COLLISION_SHAPE_2D_H
#define COLLISION_SHAPE_2D_H

/*  collision_shape_2d.h                                                 */


#include "core/object/reference.h"
#include "scene/main/node_2d.h"

class Shape2D;
class CollisionObject2D;

class CollisionShape2D : public Node2D {
	GDCLASS(CollisionShape2D, Node2D);
	Ref<Shape2D> shape;
	Rect2 rect;
	uint32_t owner_id;
	CollisionObject2D *parent;
	void _shape_changed();
	bool disabled;
	bool one_way_collision;
	float one_way_collision_margin;

	void _update_in_shape_owner(bool p_xform_only = false);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;

	void set_shape(const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_shape() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_one_way_collision(bool p_enable);
	bool is_one_way_collision_enabled() const;

	void set_one_way_collision_margin(float p_margin);
	float get_one_way_collision_margin() const;

	virtual String get_configuration_warning() const;

	CollisionShape2D();
};

#endif // COLLISION_SHAPE_2D_H

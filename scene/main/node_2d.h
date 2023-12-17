#ifndef NODE2D_H
#define NODE2D_H

/*  node_2d.h                                                            */


#include "scene/main/canvas_item.h"

class Node2D : public CanvasItem {
	GDCLASS(Node2D, CanvasItem);

	Point2 pos;
	float angle;
	Size2 _scale;
	int z_index;
	bool z_relative;

	Transform2D _mat;

	bool _xform_dirty;

	void _update_transform();

	void _update_xform_values();

protected:
	void _notification(int p_notification);
	static void _bind_methods();

public:
	void set_position(const Point2 &p_pos);
	void set_rotation(float p_radians);
	void set_rotation_degrees(float p_degrees);
	void set_scale(const Size2 &p_scale);

	void rotate(float p_radians);
	void move_x(float p_delta, bool p_scaled = false);
	void move_y(float p_delta, bool p_scaled = false);
	void translate(const Vector2 &p_amount);
	void global_translate(const Vector2 &p_amount);
	void apply_scale(const Size2 &p_amount);

	Point2 get_position() const;
	float get_rotation() const;
	float get_rotation_degrees() const;
	Size2 get_scale() const;

	Point2 get_global_position() const;
	float get_global_rotation() const;
	float get_global_rotation_degrees() const;
	Size2 get_global_scale() const;

	void set_transform(const Transform2D &p_transform);
	void set_global_transform(const Transform2D &p_transform);
	void set_global_position(const Point2 &p_pos);
	void set_global_rotation(float p_radians);
	void set_global_rotation_degrees(float p_degrees);
	void set_global_scale(const Size2 &p_scale);

	void set_z_index(int p_z);
	int get_z_index() const;

	void look_at(const Vector2 &p_pos);
	float get_angle_to(const Vector2 &p_pos) const;

	Point2 to_local(Point2 p_global) const;
	Point2 to_global(Point2 p_local) const;

	void set_z_as_relative(bool p_enabled);
	bool is_z_relative() const;

	Transform2D get_relative_transform_to_parent(const Node *p_parent) const;

	Transform2D get_transform() const;

#ifdef TOOLS_ENABLED
	virtual StringName get_property_store_alias(const StringName &p_property) const;
#endif

	Node2D();
};

#endif // NODE2D_H

#ifndef CAPSULE_SHAPE_2D_H
#define CAPSULE_SHAPE_2D_H

/*  capsule_shape_2d.h                                                   */


#include "scene/resources/shapes_2d/shape_2d.h"

class CapsuleShape2D : public Shape2D {
	GDCLASS(CapsuleShape2D, Shape2D);

	real_t height;
	real_t radius;

	void _update_shape();

protected:
	static void _bind_methods();

public:
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;

	void set_height(real_t p_height);
	real_t get_height() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const;

	virtual void draw(const RID &p_to_rid, const Color &p_color);
	virtual Rect2 get_rect() const;
	virtual real_t get_enclosing_radius() const;

	Vector<Vector2> get_points() const;

	CapsuleShape2D();
};

#endif // CAPSULE_SHAPE_2D_H

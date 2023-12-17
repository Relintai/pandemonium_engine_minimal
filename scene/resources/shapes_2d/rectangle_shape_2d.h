#ifndef RECTANGLE_SHAPE_2D_H
#define RECTANGLE_SHAPE_2D_H

/*  rectangle_shape_2d.h                                                 */


#include "scene/resources/shapes_2d/shape_2d.h"

class RectangleShape2D : public Shape2D {
	GDCLASS(RectangleShape2D, Shape2D);

	Vector2 extents;
	void _update_shape();

protected:
	static void _bind_methods();

public:
	void set_extents(const Vector2 &p_extents);
	Vector2 get_extents() const;

	virtual void draw(const RID &p_to_rid, const Color &p_color);
	virtual Rect2 get_rect() const;
	virtual real_t get_enclosing_radius() const;

	RectangleShape2D();
};

#endif // RECTANGLE_SHAPE_2D_H

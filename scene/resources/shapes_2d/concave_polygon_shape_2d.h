#ifndef CONCAVE_POLYGON_SHAPE_2D_H
#define CONCAVE_POLYGON_SHAPE_2D_H

/*  concave_polygon_shape_2d.h                                           */


#include "scene/resources/shapes_2d/shape_2d.h"

class ConcavePolygonShape2D : public Shape2D {
	GDCLASS(ConcavePolygonShape2D, Shape2D);

protected:
	static void _bind_methods();

public:
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;

	void set_segments(const PoolVector<Vector2> &p_segments);
	PoolVector<Vector2> get_segments() const;

	virtual void draw(const RID &p_to_rid, const Color &p_color);
	virtual Rect2 get_rect() const;
	virtual real_t get_enclosing_radius() const;

	ConcavePolygonShape2D();
};

#endif

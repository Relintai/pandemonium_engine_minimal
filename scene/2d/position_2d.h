#ifndef POSITION_2D_H
#define POSITION_2D_H

/*  position_2d.h                                                        */


#include "scene/main/node_2d.h"

class Position2D : public Node2D {
	GDCLASS(Position2D, Node2D);

	void _draw_cross();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
#ifdef TOOLS_ENABLED
	virtual Rect2 _edit_get_rect() const;
	virtual bool _edit_use_rect() const;
#endif

	void set_gizmo_extents(float p_extents);
	float get_gizmo_extents() const;

	Position2D();
};

#endif // POSITION_2D_H

#ifndef PATH_2D_H
#define PATH_2D_H

/*  path_2d.h                                                            */


#include "scene/main/node_2d.h"

class Curve2D;

class Path2D : public Node2D {
	GDCLASS(Path2D, Node2D);

	Ref<Curve2D> curve;
	Vector<Vector2> _cached_draw_pts;

	void _curve_changed();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_curve(const Ref<Curve2D> &p_curve);
	Ref<Curve2D> get_curve() const;

	Path2D();
};

class PathFollow2D : public Node2D {
	GDCLASS(PathFollow2D, Node2D);

public:
private:
	Path2D *path;
	real_t offset;
	real_t h_offset;
	real_t v_offset;
	real_t lookahead;
	bool cubic;
	bool loop;
	bool rotate;

	void _update_transform();

protected:
	virtual void _validate_property(PropertyInfo &property) const;

	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_offset(float p_offset);
	float get_offset() const;

	void set_h_offset(float p_h_offset);
	float get_h_offset() const;

	void set_v_offset(float p_v_offset);
	float get_v_offset() const;

	void set_unit_offset(float p_unit_offset);
	float get_unit_offset() const;

	void set_lookahead(float p_lookahead);
	float get_lookahead() const;

	void set_loop(bool p_loop);
	bool has_loop() const;

	void set_rotate(bool p_rotate);
	bool is_rotating() const;

	void set_cubic_interpolation(bool p_enable);
	bool get_cubic_interpolation() const;

	String get_configuration_warning() const;

	PathFollow2D();
};

#endif // PATH_2D_H

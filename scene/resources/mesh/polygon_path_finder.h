#ifndef POLYGON_PATH_FINDER_H
#define POLYGON_PATH_FINDER_H

/*  polygon_path_finder.h                                                */


#include "core/object/resource.h"

class PolygonPathFinder : public Resource {
	GDCLASS(PolygonPathFinder, Resource);

	struct Point {
		Vector2 pos;
		RBSet<int> connections;
		float distance;
		float penalty;
		int prev;
	};

	struct Edge {
		int points[2];

		_FORCE_INLINE_ bool operator<(const Edge &p_edge) const {
			if (points[0] == p_edge.points[0]) {
				return points[1] < p_edge.points[1];
			} else {
				return points[0] < p_edge.points[0];
			}
		}

		Edge(int a = 0, int b = 0) {
			if (a > b) {
				SWAP(a, b);
			}
			points[0] = a;
			points[1] = b;
		}
	};

	Vector2 outside_point;
	Rect2 bounds;

	Vector<Point> points;
	RBSet<Edge> edges;

	bool _is_point_inside(const Vector2 &p_point) const;

	void _set_data(const Dictionary &p_data);
	Dictionary _get_data() const;

protected:
	static void _bind_methods();

public:
	void setup(const Vector<Vector2> &p_points, const Vector<int> &p_connections);
	Vector<Vector2> find_path(const Vector2 &p_from, const Vector2 &p_to);

	void set_point_penalty(int p_point, float p_penalty);
	float get_point_penalty(int p_point) const;

	bool is_point_inside(const Vector2 &p_point) const;
	Vector2 get_closest_point(const Vector2 &p_point) const;
	Vector<Vector2> get_intersections(const Vector2 &p_from, const Vector2 &p_to) const;
	Rect2 get_bounds() const;

	PolygonPathFinder();
};

#endif // POLYGON_PATH_FINDER_H

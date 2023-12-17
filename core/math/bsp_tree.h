#ifndef BSP_TREE_H
#define BSP_TREE_H

/*  bsp_tree.h                                                           */


#include "core/math/aabb.h"
#include "core/math/face3.h"
#include "core/math/plane.h"
#include "core/variant/method_ptrcall.h"
#include "core/containers/pool_vector.h"
#include "core/variant/variant.h"
#include "core/containers/vector.h"

class BSP_Tree {
public:
	enum {

		UNDER_LEAF = 0xFFFF,
		OVER_LEAF = 0xFFFE,
		MAX_NODES = 0xFFFE,
		MAX_PLANES = (1 << 16)
	};

	struct Node {
		uint16_t plane;
		uint16_t under;
		uint16_t over;
	};

private:
	// thanks to the properties of Vector,
	// this class can be assigned and passed around between threads
	// with no cost.

	Vector<Node> nodes;
	Vector<Plane> planes;
	AABB aabb;
	real_t error_radius;

	int _get_points_inside(int p_node, const Vector3 *p_points, int *p_indices, const Vector3 &p_center, const Vector3 &p_half_extents, int p_indices_count) const;

	template <class T>
	bool _test_convex(const Node *p_nodes, const Plane *p_planes, int p_current, const T &p_convex) const;

public:
	bool is_empty() const { return nodes.size() == 0; }
	Vector<Node> get_nodes() const;
	Vector<Plane> get_planes() const;
	AABB get_aabb() const;

	bool point_is_inside(const Vector3 &p_point) const;
	int get_points_inside(const Vector3 *p_points, int p_point_count) const;
	template <class T>
	bool convex_is_inside(const T &p_convex) const;

	operator Variant() const;

	void from_aabb(const AABB &p_aabb);

	BSP_Tree();
	BSP_Tree(const Variant &p_variant);
	BSP_Tree(const PoolVector<Face3> &p_faces, real_t p_error_radius = 0);
	BSP_Tree(const Vector<Node> &p_nodes, const Vector<Plane> &p_planes, const AABB &p_aabb, real_t p_error_radius = 0);
	~BSP_Tree();
};

template <class T>
bool BSP_Tree::_test_convex(const Node *p_nodes, const Plane *p_planes, int p_current, const T &p_convex) const {
	if (p_current == UNDER_LEAF) {
		return true;
	} else if (p_current == OVER_LEAF) {
		return false;
	}

	bool collided = false;
	const Node &n = p_nodes[p_current];

	const Plane &p = p_planes[n.plane];

	real_t min, max;
	p_convex.project_range(p.normal, min, max);

	bool go_under = min < p.d;
	bool go_over = max >= p.d;

	if (go_under && _test_convex(p_nodes, p_planes, n.under, p_convex)) {
		collided = true;
	}
	if (go_over && _test_convex(p_nodes, p_planes, n.over, p_convex)) {
		collided = true;
	}

	return collided;
}

template <class T>
bool BSP_Tree::convex_is_inside(const T &p_convex) const {
	int node_count = nodes.size();
	if (node_count == 0) {
		return false;
	}
	const Node *nodes = &this->nodes[0];
	const Plane *planes = &this->planes[0];

	return _test_convex(nodes, planes, node_count - 1, p_convex);
}

#ifdef PTRCALL_ENABLED

template <>
struct PtrToArg<BSP_Tree> {
	_FORCE_INLINE_ static BSP_Tree convert(const void *p_ptr) {
		BSP_Tree s(Variant(*reinterpret_cast<const Dictionary *>(p_ptr)));
		return s;
	}
	_FORCE_INLINE_ static void encode(BSP_Tree p_val, void *p_ptr) {
		Dictionary *d = reinterpret_cast<Dictionary *>(p_ptr);
		*d = Variant(p_val);
	}
};

template <>
struct PtrToArg<const BSP_Tree &> {
	_FORCE_INLINE_ static BSP_Tree convert(const void *p_ptr) {
		BSP_Tree s(Variant(*reinterpret_cast<const Dictionary *>(p_ptr)));
		return s;
	}
};

#endif

#endif

/**************************************************************************/
/*  staticbody2d_navigation_geometry_parser_2d.cpp                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "staticbody2d_navigation_geometry_parser_2d.h"

#include "core/math/convex_hull.h"
#include "scene/2d/mesh_instance_2d.h"
#include "scene/2d/physics_body_2d.h"
#include "scene/resources/shapes_2d/capsule_shape_2d.h"
#include "scene/resources/shapes_2d/circle_shape_2d.h"
#include "scene/resources/shapes_2d/concave_polygon_shape_2d.h"
#include "scene/resources/shapes_2d/convex_polygon_shape_2d.h"
#include "scene/resources/shapes_2d/rectangle_shape_2d.h"
#include "scene/resources/shapes_2d/shape_2d.h"

#include "scene/resources/navigation_2d/navigation_mesh_source_geometry_data_2d.h"
#include "scene/resources/navigation_2d/navigation_polygon.h"

bool StaticBody2DNavigationGeometryParser2D::parses_node(Node *p_node) {
	return (Object::cast_to<StaticBody2D>(p_node) != nullptr);
}

void StaticBody2DNavigationGeometryParser2D::parse_geometry(Node *p_node, Ref<NavigationPolygon> p_navigation_polygon, Ref<NavigationMeshSourceGeometryData2D> p_source_geometry) {
	NavigationPolygon::ParsedGeometryType parsed_geometry_type = p_navigation_polygon->get_parsed_geometry_type();
	uint32_t navigation_polygon_collision_mask = p_navigation_polygon->get_collision_mask();

	if (Object::cast_to<StaticBody2D>(p_node) && parsed_geometry_type != NavigationPolygon::PARSED_GEOMETRY_MESH_INSTANCES) {
		StaticBody2D *static_body = Object::cast_to<StaticBody2D>(p_node);
		if (static_body->get_collision_layer() & navigation_polygon_collision_mask) {
			List<uint32_t> shape_owners;
			static_body->get_shape_owners(&shape_owners);

			for (List<uint32_t>::Element *E = shape_owners.front(); E; E = E->next()) {
				uint32_t shape_owner = E->get();

				if (static_body->is_shape_owner_disabled(shape_owner)) {
					continue;
				}
				const int shape_count = static_body->shape_owner_get_shape_count(shape_owner);
				for (int shape_index = 0; shape_index < shape_count; shape_index++) {
					Ref<Shape2D> s = static_body->shape_owner_get_shape(shape_owner, shape_index);

					if (s.is_null()) {
						continue;
					}

					const Transform2D transform = static_body->get_transform() * static_body->shape_owner_get_transform(shape_owner);

					RectangleShape2D *rectangle_shape = Object::cast_to<RectangleShape2D>(*s);
					if (rectangle_shape) {
						PoolVector<Vector2> shape_outline;

						const Vector2 rectangle_size = rectangle_shape->get_extents();

						shape_outline.resize(5);
						PoolVector<Vector2>::Write shape_outline_write = shape_outline.write();
						shape_outline_write[0] = transform.xform(-rectangle_size);
						shape_outline_write[1] = transform.xform(Vector2(rectangle_size.x, -rectangle_size.y));
						shape_outline_write[2] = transform.xform(rectangle_size);
						shape_outline_write[3] = transform.xform(Vector2(-rectangle_size.x, rectangle_size.y));
						shape_outline_write[4] = transform.xform(-rectangle_size);

						p_source_geometry->add_obstruction_outline(shape_outline);
					}

					CapsuleShape2D *capsule_shape = Object::cast_to<CapsuleShape2D>(*s);
					if (capsule_shape) {
						PoolVector<Vector2> shape_outline;

						Vector<Vector2> points = capsule_shape->get_points();

						shape_outline.resize(points.size() + 1);
						PoolVector<Vector2>::Write shape_outline_write = shape_outline.write();

						for (int i = 0; i < points.size(); ++i) {
							shape_outline_write[i] = transform.xform(points[i]);
						}

						if (points.size() > 0) {
							shape_outline_write[points.size()] = transform.xform(points[0]);
						}

						p_source_geometry->add_obstruction_outline(shape_outline);
					}

					CircleShape2D *circle_shape = Object::cast_to<CircleShape2D>(*s);
					if (circle_shape) {
						PoolVector<Vector2> shape_outline;
						int circle_edge_count = 12;
						shape_outline.resize(circle_edge_count);
						PoolVector<Vector2>::Write shape_outline_write = shape_outline.write();

						const real_t turn_step = Math_TAU / real_t(circle_edge_count);
						for (int i = 0; i < circle_edge_count; i++) {
							shape_outline_write[i] = transform.xform(Vector2(Math::cos(i * turn_step), Math::sin(i * turn_step)) * circle_shape->get_radius());
						}

						p_source_geometry->add_obstruction_outline(shape_outline);
					}

					ConcavePolygonShape2D *concave_polygon_shape = Object::cast_to<ConcavePolygonShape2D>(*s);
					if (concave_polygon_shape) {
						PoolVector<Vector2> shape_outline = concave_polygon_shape->get_segments();
						PoolVector<Vector2>::Write shape_outline_write = shape_outline.write();

						for (int i = 0; i < shape_outline.size(); i++) {
							shape_outline_write[i] = transform.xform(shape_outline[i]);
						}

						p_source_geometry->add_obstruction_outline(shape_outline);
					}

					ConvexPolygonShape2D *convex_polygon_shape = Object::cast_to<ConvexPolygonShape2D>(*s);
					if (convex_polygon_shape) {
						Vector<Vector2> shape_outlinev = convex_polygon_shape->get_points();

						PoolVector<Vector2> shape_outline;
						shape_outline.resize(shape_outlinev.size());
						PoolVector<Vector2>::Write shape_outline_write = shape_outline.write();

						for (int i = 0; i < shape_outline.size(); i++) {
							shape_outline_write[i] = transform.xform(shape_outlinev[i]);
						}

						p_source_geometry->add_obstruction_outline(shape_outline);
					}
				}
			}
		}
	}
}

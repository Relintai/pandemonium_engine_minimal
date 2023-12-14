/**************************************************************************/
/*  gridmap_navigation_geometry_parser_3d.cpp                             */
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

#include "gridmap_navigation_geometry_parser_3d.h"

#include "modules/gridmap/grid_map.h"

#include "core/math/convex_hull.h"
#include "scene/3d/mesh_instance.h"
#include "scene/3d/physics_body.h"
#include "scene/resources/shapes/box_shape.h"
#include "scene/resources/shapes/capsule_shape.h"
#include "scene/resources/shapes/concave_polygon_shape.h"
#include "scene/resources/shapes/convex_polygon_shape.h"
#include "scene/resources/shapes/cylinder_shape.h"
#include "scene/resources/shapes/height_map_shape.h"
#include "scene/resources/navigation/navigation_mesh.h"
#include "scene/resources/navigation/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/shapes/plane_shape.h"
#include "scene/resources/mesh/primitive_meshes.h"
#include "scene/resources/shapes/shape.h"
#include "scene/resources/shapes/sphere_shape.h"

bool GridMap3DNavigationGeometryParser3D::parses_node(Node *p_node) {
	return (Object::cast_to<GridMap>(p_node) != nullptr);
}

void GridMap3DNavigationGeometryParser3D::parse_geometry(Node *p_node, Ref<NavigationMesh> p_navigationmesh, Ref<NavigationMeshSourceGeometryData3D> p_source_geometry) {
	GridMap *gridmap = Object::cast_to<GridMap>(p_node);
	NavigationMesh::ParsedGeometryType parsed_geometry_type = p_navigationmesh->get_parsed_geometry_type();
	uint32_t navigationmesh_collision_mask = p_navigationmesh->get_collision_mask();

	if (gridmap) {
		if (parsed_geometry_type != NavigationMesh::PARSED_GEOMETRY_STATIC_COLLIDERS) {
			Array meshes = gridmap->get_meshes();
			Transform xform = gridmap->get_global_transform();
			for (int i = 0; i < meshes.size(); i += 2) {
				Ref<Mesh> mesh = meshes[i + 1];
				if (mesh.is_valid()) {
					p_source_geometry->add_mesh(mesh, xform * meshes[i].operator Transform());
				}
			}
		}

		else if (parsed_geometry_type != NavigationMesh::PARSED_GEOMETRY_MESH_INSTANCES && (gridmap->get_collision_layer() & navigationmesh_collision_mask)) {
			Array shapes = gridmap->get_collision_shapes();
			for (int i = 0; i < shapes.size(); i += 2) {
				RID shape = shapes[i + 1];
				PhysicsServer::ShapeType type = PhysicsServer::get_singleton()->shape_get_type(shape);
				Variant data = PhysicsServer::get_singleton()->shape_get_data(shape);

				switch (type) {
					case PhysicsServer::SHAPE_SPHERE: {
						real_t radius = data;
						Array arr;
						arr.resize(RS::ARRAY_MAX);
						SphereMesh::create_mesh_array(arr, radius, radius * 2.0);
						p_source_geometry->add_mesh_array(arr, shapes[i]);
					} break;
					case PhysicsServer::SHAPE_BOX: {
						Vector3 extents = data;
						Array arr;
						arr.resize(RS::ARRAY_MAX);
						CubeMesh::create_mesh_array(arr, extents * 2.0);
						p_source_geometry->add_mesh_array(arr, shapes[i]);
					} break;
					case PhysicsServer::SHAPE_CAPSULE: {
						Dictionary dict = data;
						real_t radius = dict["radius"];
						real_t height = dict["height"];
						Array arr;
						arr.resize(RS::ARRAY_MAX);
						CapsuleMesh::create_mesh_array(arr, radius, height);
						p_source_geometry->add_mesh_array(arr, shapes[i]);
					} break;
					case PhysicsServer::SHAPE_CYLINDER: {
						Dictionary dict = data;
						real_t radius = dict["radius"];
						real_t height = dict["height"];
						Array arr;
						arr.resize(RS::ARRAY_MAX);
						CylinderMesh::create_mesh_array(arr, radius, radius, height);
						p_source_geometry->add_mesh_array(arr, shapes[i]);
					} break;
					case PhysicsServer::SHAPE_CONVEX_POLYGON: {
						PoolVector3Array vertices = data;
						Geometry::MeshData md;

						Error err = ConvexHullComputer::convex_hull(vertices, md);

						if (err == OK) {
							PoolVector3Array faces;

							for (int j = 0; j < md.faces.size(); ++j) {
								const Geometry::MeshData::Face &face = md.faces[j];

								for (int k = 2; k < face.indices.size(); ++k) {
									faces.push_back(md.vertices[face.indices[0]]);
									faces.push_back(md.vertices[face.indices[k - 1]]);
									faces.push_back(md.vertices[face.indices[k]]);
								}
							}

							p_source_geometry->add_faces(faces, shapes[i]);
						}
					} break;
					case PhysicsServer::SHAPE_CONCAVE_POLYGON: {
						Dictionary dict = data;
						PoolVector3Array faces = Variant(dict["faces"]);
						p_source_geometry->add_faces(faces, shapes[i]);
					} break;
					case PhysicsServer::SHAPE_HEIGHTMAP: {
						Dictionary dict = data;
						///< dict( int:"width", int:"depth",float:"cell_size", float_array:"heights"
						int heightmap_depth = dict["depth"];
						int heightmap_width = dict["width"];

						if (heightmap_depth >= 2 && heightmap_width >= 2) {
							const Vector<real_t> &map_data = dict["heights"];

							Vector2 heightmap_gridsize(heightmap_width - 1, heightmap_depth - 1);
							Vector2 start = heightmap_gridsize * -0.5;

							PoolVector<Vector3> vertex_array;
							vertex_array.resize((heightmap_depth - 1) * (heightmap_width - 1) * 6);
							int map_data_current_index = 0;

							for (int d = 0; d < heightmap_depth; d++) {
								for (int w = 0; w < heightmap_width; w++) {
									if (map_data_current_index + 1 + heightmap_depth < map_data.size()) {
										float top_left_height = map_data[map_data_current_index];
										float top_right_height = map_data[map_data_current_index + 1];
										float bottom_left_height = map_data[map_data_current_index + heightmap_depth];
										float bottom_right_height = map_data[map_data_current_index + 1 + heightmap_depth];

										Vector3 top_left = Vector3(start.x + w, top_left_height, start.y + d);
										Vector3 top_right = Vector3(start.x + w + 1.0, top_right_height, start.y + d);
										Vector3 bottom_left = Vector3(start.x + w, bottom_left_height, start.y + d + 1.0);
										Vector3 bottom_right = Vector3(start.x + w + 1.0, bottom_right_height, start.y + d + 1.0);

										vertex_array.push_back(top_right);
										vertex_array.push_back(bottom_left);
										vertex_array.push_back(top_left);
										vertex_array.push_back(top_right);
										vertex_array.push_back(bottom_right);
										vertex_array.push_back(bottom_left);
									}
									map_data_current_index += 1;
								}
							}
							if (vertex_array.size() > 0) {
								p_source_geometry->add_faces(vertex_array, shapes[i]);
							}
						}
					} break;
					default: {
						WARN_PRINT("Unsupported collision shape type.");
					} break;
				}
			}
		}
	}
}

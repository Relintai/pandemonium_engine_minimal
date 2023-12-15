
#include "navigation_mesh_generator_dummy.h"

#include "core/config/project_settings.h"

#include "scene/2d/navigation_geometry_parser_2d.h"
#include "scene/main/node.h"
#include "scene/resources/navigation/navigation_mesh.h"
#include "scene/resources/navigation_2d/navigation_mesh_source_geometry_data_2d.h"

void NavigationMeshGeneratorDummy::cleanup() {}
void NavigationMeshGeneratorDummy::process() {}

// 2D //////////////////////////////
void NavigationMeshGeneratorDummy::register_geometry_parser_2d(Ref<NavigationGeometryParser2D> p_geometry_parser) {}
void NavigationMeshGeneratorDummy::unregister_geometry_parser_2d(Ref<NavigationGeometryParser2D> p_geometry_parser) {}

Ref<NavigationMeshSourceGeometryData2D> NavigationMeshGeneratorDummy::parse_2d_source_geometry_data(Ref<NavigationPolygon> p_navigation_polygon, Node *p_root_node, Ref<FuncRef> p_callback) {
	return Ref<NavigationMeshSourceGeometryData2D>();
}
void NavigationMeshGeneratorDummy::bake_2d_from_source_geometry_data(Ref<NavigationPolygon> p_navigation_polygon, Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Ref<FuncRef> p_callback) {}

void NavigationMeshGeneratorDummy::parse_and_bake_2d(Ref<NavigationPolygon> p_navigation_polygon, Node *p_root_node, Ref<FuncRef> p_callback) {}

bool NavigationMeshGeneratorDummy::is_navigation_polygon_baking(Ref<NavigationPolygon> p_navigation_polygon) const {
	return false;
}



#include "dummy_navigation_server.h"

#include "scene/resources/navigation/navigation_mesh.h"

void DummyNavigationServer::region_set_navigation_mesh(RID p_region, Ref<NavigationMesh> p_navigation_mesh) {
}

NavigationUtilities::PathQueryResult DummyNavigationServer::_query_path(const NavigationUtilities::PathQueryParameters &p_parameters) const {
	return NavigationUtilities::PathQueryResult();
}

DummyNavigationServer::DummyNavigationServer() {
}

DummyNavigationServer::~DummyNavigationServer() {
}
/**************************************************************************/
/*  navigation_path_query_result_2d.cpp                                   */
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

#include "navigation_path_query_result_2d.h"

void NavigationPathQueryResult2D::set_path(const Vector<Vector2> &p_path) {
	path = p_path;
}

Vector<Vector2> NavigationPathQueryResult2D::get_path() const {
	return path;
}

void NavigationPathQueryResult2D::set_path_types(const Vector<int32_t> &p_path_types) {
	path_types = p_path_types;
}

Vector<int32_t> NavigationPathQueryResult2D::get_path_types() const {
	return path_types;
}

void NavigationPathQueryResult2D::set_path_rids(const Array &p_path_rids) {
	path_rids = p_path_rids;
}

Array NavigationPathQueryResult2D::get_path_rids() const {
	return path_rids;
}

void NavigationPathQueryResult2D::set_path_owner_ids(const Vector<ObjectID> &p_path_owner_ids) {
	path_owner_ids = p_path_owner_ids;
}

Vector<ObjectID> NavigationPathQueryResult2D::get_path_owner_ids() const {
	return path_owner_ids;
}

void NavigationPathQueryResult2D::set_path_owner_ids_bind(const Array p_path_owner_ids) {
	path_owner_ids.resize(p_path_owner_ids.size());

	for (int i = 0; i < path_owner_ids.size(); ++i) {
		path_owner_ids.write[i] = p_path_owner_ids[i];
	}
}
Array NavigationPathQueryResult2D::get_path_owner_ids_bind() const {
	Array ret;

	ret.resize(path_owner_ids.size());

	for (int i = 0; i < path_owner_ids.size(); ++i) {
		ret[i] = path_owner_ids[i];
	}

	return ret;
}

void NavigationPathQueryResult2D::set_from_query_result(const NavigationUtilities::PathQueryResult2D &p_result) {
	path = p_result.path;
	path_types = p_result.path_types;
	path_rids = p_result.path_rids;
	path_owner_ids = p_result.path_owner_ids;
}

void NavigationPathQueryResult2D::reset() {
	path.clear();
	path_types.clear();
	path_rids.clear();
	path_owner_ids.clear();
}

void NavigationPathQueryResult2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_path", "path"), &NavigationPathQueryResult2D::set_path);
	ClassDB::bind_method(D_METHOD("get_path"), &NavigationPathQueryResult2D::get_path);

	ClassDB::bind_method(D_METHOD("set_path_types", "path_types"), &NavigationPathQueryResult2D::set_path_types);
	ClassDB::bind_method(D_METHOD("get_path_types"), &NavigationPathQueryResult2D::get_path_types);

	ClassDB::bind_method(D_METHOD("set_path_rids", "path_rids"), &NavigationPathQueryResult2D::set_path_rids);
	ClassDB::bind_method(D_METHOD("get_path_rids"), &NavigationPathQueryResult2D::get_path_rids);

	ClassDB::bind_method(D_METHOD("set_path_owner_ids", "path_owner_ids"), &NavigationPathQueryResult2D::set_path_owner_ids_bind);
	ClassDB::bind_method(D_METHOD("get_path_owner_ids"), &NavigationPathQueryResult2D::get_path_owner_ids_bind);

	ClassDB::bind_method(D_METHOD("reset"), &NavigationPathQueryResult2D::reset);

	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, "path"), "set_path", "get_path");
	ADD_PROPERTY(PropertyInfo(Variant::POOL_INT_ARRAY, "path_types"), "set_path_types", "get_path_types");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "path_rids"), "set_path_rids", "get_path_rids");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "path_owner_ids"), "set_path_owner_ids", "get_path_owner_ids");

	BIND_ENUM_CONSTANT(PATH_SEGMENT_TYPE_REGION);
	BIND_ENUM_CONSTANT(PATH_SEGMENT_TYPE_LINK);
}

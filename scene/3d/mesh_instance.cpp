/*************************************************************************/
/*  mesh_instance.cpp                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "mesh_instance.h"

#include "core/config/project_settings.h"
#include "core/core_string_names.h"
#include "scene/resources/material/material.h"
#include "scene/resources/material/spatial_material.h"
#include "scene/resources/mesh/mesh.h"
#include "scene/main/scene_string_names.h"
#include "servers/rendering/rendering_server_globals.h"

bool MeshInstance::_set(const StringName &p_name, const Variant &p_value) {
	//this is not _too_ bad performance wise, really. it only arrives here if the property was not set anywhere else.
	//add to it that it's probably found on first call to _set anyway.

	if (!get_instance().is_valid()) {
		return false;
	}

	RBMap<StringName, BlendShapeTrack>::Element *E = blend_shape_tracks.find(p_name);
	if (E) {
		E->get().value = p_value;
		RenderingServer::get_singleton()->instance_set_blend_shape_weight(get_instance(), E->get().idx, E->get().value);
		return true;
	}

	if (p_name.operator String().begins_with("material/")) {
		int idx = p_name.operator String().get_slicec('/', 1).to_int();
		if (idx >= materials.size() || idx < 0) {
			return false;
		}

		set_surface_material(idx, p_value);
		return true;
	}

	return false;
}

bool MeshInstance::_get(const StringName &p_name, Variant &r_ret) const {
	if (!get_instance().is_valid()) {
		return false;
	}

	const RBMap<StringName, BlendShapeTrack>::Element *E = blend_shape_tracks.find(p_name);
	if (E) {
		r_ret = E->get().value;
		return true;
	}

	if (p_name.operator String().begins_with("material/")) {
		int idx = p_name.operator String().get_slicec('/', 1).to_int();
		if (idx >= materials.size() || idx < 0) {
			return false;
		}
		r_ret = materials[idx];
		return true;
	}
	return false;
}

void MeshInstance::_get_property_list(List<PropertyInfo> *p_list) const {
	List<String> ls;
	for (const RBMap<StringName, BlendShapeTrack>::Element *E = blend_shape_tracks.front(); E; E = E->next()) {
		ls.push_back(E->key());
	}

	ls.sort();

	for (List<String>::Element *E = ls.front(); E; E = E->next()) {
		p_list->push_back(PropertyInfo(Variant::REAL, E->get(), PROPERTY_HINT_RANGE, "-1,1,0.00001"));
	}

	if (mesh.is_valid()) {
		for (int i = 0; i < mesh->get_surface_count(); i++) {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "material/" + itos(i), PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial,SpatialMaterial"));
		}
	}
}

void MeshInstance::set_mesh(const Ref<Mesh> &p_mesh) {
	if (mesh == p_mesh) {
		return;
	}

	if (mesh.is_valid()) {
		mesh->disconnect(CoreStringNames::get_singleton()->changed, this, SceneStringNames::get_singleton()->_mesh_changed);
	}

	mesh = p_mesh;

	blend_shape_tracks.clear();
	if (mesh.is_valid()) {
		for (int i = 0; i < mesh->get_blend_shape_count(); i++) {
			BlendShapeTrack mt;
			mt.idx = i;
			mt.value = 0;
			blend_shape_tracks["blend_shapes/" + String(mesh->get_blend_shape_name(i))] = mt;
		}

		mesh->connect(CoreStringNames::get_singleton()->changed, this, SceneStringNames::get_singleton()->_mesh_changed);
		materials.resize(mesh->get_surface_count());

#ifdef MODULE_SKELETON_3D_ENABLED
		_initialize_skinning(false, true);
#endif
	} else {
		set_base(RID());
	}

	update_gizmos();

	_change_notify();
}
Ref<Mesh> MeshInstance::get_mesh() const {
	return mesh;
}

AABB MeshInstance::get_aabb() const {
	if (!mesh.is_null()) {
		return mesh->get_aabb();
	}

	return AABB();
}

PoolVector<Face3> MeshInstance::get_faces(uint32_t p_usage_flags) const {
	if (!(p_usage_flags & (FACES_SOLID | FACES_ENCLOSING))) {
		return PoolVector<Face3>();
	}

	if (mesh.is_null()) {
		return PoolVector<Face3>();
	}

	return mesh->get_faces();
}

Node *MeshInstance::create_trimesh_collision_node() {
	return NULL;
}

void MeshInstance::create_trimesh_collision() {
}

Node *MeshInstance::create_multiple_convex_collisions_node() {
	return NULL;
}

void MeshInstance::create_multiple_convex_collisions() {
}

Node *MeshInstance::create_convex_collision_node(bool p_clean, bool p_simplify) {
	return NULL;
}

void MeshInstance::create_convex_collision(bool p_clean, bool p_simplify) {
}

void MeshInstance::_notification(int p_what) {
}

int MeshInstance::get_surface_material_count() const {
	return materials.size();
}

void MeshInstance::set_surface_material(int p_surface, const Ref<Material> &p_material) {
	ERR_FAIL_INDEX(p_surface, materials.size());

	materials.write[p_surface] = p_material;

	if (materials[p_surface].is_valid()) {
		RS::get_singleton()->instance_set_surface_material(get_instance(), p_surface, materials[p_surface]->get_rid());
	} else {
		RS::get_singleton()->instance_set_surface_material(get_instance(), p_surface, RID());
	}
}

Ref<Material> MeshInstance::get_surface_material(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, materials.size(), Ref<Material>());

	return materials[p_surface];
}

Ref<Material> MeshInstance::get_active_material(int p_surface) const {
	Ref<Material> material_override = get_material_override();
	if (material_override.is_valid()) {
		return material_override;
	}

	Ref<Material> surface_material = get_surface_material(p_surface);
	if (surface_material.is_valid()) {
		return surface_material;
	}

	Ref<Mesh> mesh = get_mesh();
	if (mesh.is_valid()) {
		return mesh->surface_get_material(p_surface);
	}

	return Ref<Material>();
}

void MeshInstance::set_material_override(const Ref<Material> &p_material) {
	if (p_material == get_material_override()) {
		return;
	}

	GeometryInstance::set_material_override(p_material);
}

void MeshInstance::set_material_overlay(const Ref<Material> &p_material) {
	if (p_material == get_material_overlay()) {
		return;
	}

	GeometryInstance::set_material_overlay(p_material);
}

void MeshInstance::_mesh_changed() {
	ERR_FAIL_COND(mesh.is_null());
	materials.resize(mesh->get_surface_count());
}

void MeshInstance::create_debug_tangents() {
	Vector<Vector3> lines;
	Vector<Color> colors;

	Ref<Mesh> mesh = get_mesh();
	if (!mesh.is_valid()) {
		return;
	}

	for (int i = 0; i < mesh->get_surface_count(); i++) {
		Array arrays = mesh->surface_get_arrays(i);
		Vector<Vector3> verts = arrays[Mesh::ARRAY_VERTEX];
		Vector<Vector3> norms = arrays[Mesh::ARRAY_NORMAL];
		if (norms.size() == 0) {
			continue;
		}
		Vector<float> tangents = arrays[Mesh::ARRAY_TANGENT];
		if (tangents.size() == 0) {
			continue;
		}

		for (int j = 0; j < verts.size(); j++) {
			Vector3 v = verts[j];
			Vector3 n = norms[j];
			Vector3 t = Vector3(tangents[j * 4 + 0], tangents[j * 4 + 1], tangents[j * 4 + 2]);
			Vector3 b = (n.cross(t)).normalized() * tangents[j * 4 + 3];

			lines.push_back(v); //normal
			colors.push_back(Color(0, 0, 1)); //color
			lines.push_back(v + n * 0.04); //normal
			colors.push_back(Color(0, 0, 1)); //color

			lines.push_back(v); //tangent
			colors.push_back(Color(1, 0, 0)); //color
			lines.push_back(v + t * 0.04); //tangent
			colors.push_back(Color(1, 0, 0)); //color

			lines.push_back(v); //binormal
			colors.push_back(Color(0, 1, 0)); //color
			lines.push_back(v + b * 0.04); //binormal
			colors.push_back(Color(0, 1, 0)); //color
		}
	}

	if (lines.size()) {
		Ref<SpatialMaterial> sm;
		sm.instance();

		sm->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
		sm->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
		sm->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

		Ref<ArrayMesh> am;
		am.instance();
		Array a;
		a.resize(Mesh::ARRAY_MAX);
		a[Mesh::ARRAY_VERTEX] = lines;
		a[Mesh::ARRAY_COLOR] = colors;

		am->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, a);
		am->surface_set_material(0, sm);

		MeshInstance *mi = memnew(MeshInstance);
		mi->set_mesh(am);
		mi->set_name("DebugTangents");
		add_child(mi);
	}
}

bool MeshInstance::merge_meshes(Vector<Variant> p_list, bool p_use_global_space, bool p_check_compatibility) {
	// bound function only support variants, so we need to convert to a list of MeshInstances
	Vector<MeshInstance *> mis;

	for (int n = 0; n < p_list.size(); n++) {
		MeshInstance *mi = Object::cast_to<MeshInstance>(p_list[n]);
		if (mi) {
			if (mi != this) {
				mis.push_back(mi);
			} else {
				ERR_PRINT("Destination MeshInstance cannot be a source.");
			}
		} else {
			ERR_PRINT("Only MeshInstances can be merged.");
		}
	}

	ERR_FAIL_COND_V(!mis.size(), "Array contains no MeshInstances");
	return _merge_meshes(mis, p_use_global_space, p_check_compatibility);
}

bool MeshInstance::is_mergeable_with(Node *p_other) const {
	const MeshInstance *mi = Object::cast_to<MeshInstance>(p_other);

	if (mi) {
		return _is_mergeable_with(*mi);
	}

	return false;
}

bool MeshInstance::_is_mergeable_with(const MeshInstance &p_other) const {
	if (!get_mesh().is_valid() || !p_other.get_mesh().is_valid()) {
		return false;
	}
	if (!get_allow_merging() || !p_other.get_allow_merging()) {
		return false;
	}

	// various settings that must match
	if (get_material_overlay() != p_other.get_material_overlay()) {
		return false;
	}
	if (get_material_override() != p_other.get_material_override()) {
		return false;
	}
	if (get_cast_shadows_setting() != p_other.get_cast_shadows_setting()) {
		return false;
	}
	if (is_visible() != p_other.is_visible()) {
		return false;
	}

	Ref<Mesh> rmesh_a = get_mesh();
	Ref<Mesh> rmesh_b = p_other.get_mesh();

	int num_surfaces = rmesh_a->get_surface_count();
	if (num_surfaces != rmesh_b->get_surface_count()) {
		return false;
	}

	for (int n = 0; n < num_surfaces; n++) {
		// materials must match
		if (get_active_material(n) != p_other.get_active_material(n)) {
			return false;
		}

		// formats must match
		uint32_t format_a = rmesh_a->surface_get_format(n);
		uint32_t format_b = rmesh_b->surface_get_format(n);

		if (format_a != format_b) {
			return false;
		}
	}

	// NOTE : These three commented out sections below are more conservative
	// checks for whether to allow mesh merging. I am not absolutely sure a priori
	// how conservative we need to be, so we can further enable this if testing
	// shows they are required.

	//	if (get_surface_material_count() != p_other.get_surface_material_count()) {
	//		return false;
	//	}

	//	for (int n = 0; n < get_surface_material_count(); n++) {
	//		if (get_surface_material(n) != p_other.get_surface_material(n)) {
	//			return false;
	//		}
	//	}

	// test only allow identical meshes
	//	if (get_mesh() != p_other.get_mesh()) {
	//		return false;
	//	}

	return true;
}

void MeshInstance::_merge_into_mesh_data(const MeshInstance &p_mi, const Transform &p_dest_tr_inv, int p_surface_id, LocalVector<Vector3> &r_verts, LocalVector<Vector3> &r_norms, LocalVector<real_t> &r_tangents, LocalVector<Color> &r_colors, LocalVector<Vector2> &r_uvs, LocalVector<Vector2> &r_uv2s, LocalVector<int> &r_inds) {
	_merge_log("\t\t\tmesh data from " + p_mi.get_name());

	// get the mesh verts in local space
	Ref<Mesh> rmesh = p_mi.get_mesh();

	if (rmesh->get_surface_count() <= p_surface_id) {
		return;
	}

	Array arrays = rmesh->surface_get_arrays(p_surface_id);

	LocalVector<Vector3> verts = PoolVector<Vector3>(arrays[RS::ARRAY_VERTEX]);
	if (!verts.size()) {
		// early out if there are no vertices, no point in doing anything else
		return;
	}

	LocalVector<Vector3> normals = PoolVector<Vector3>(arrays[RS::ARRAY_NORMAL]);
	LocalVector<real_t> tangents = PoolVector<real_t>(arrays[RS::ARRAY_TANGENT]);
	LocalVector<Color> colors = PoolVector<Color>(arrays[RS::ARRAY_COLOR]);
	LocalVector<Vector2> uvs = PoolVector<Vector2>(arrays[RS::ARRAY_TEX_UV]);
	LocalVector<Vector2> uv2s = PoolVector<Vector2>(arrays[RS::ARRAY_TEX_UV2]);
	LocalVector<int> indices = PoolVector<int>(arrays[RS::ARRAY_INDEX]);

	// The attributes present must match the first mesh for the attributes
	// to remain in sync. Here we reject meshes with different attributes.
	// We could alternatively invent missing attributes.
	// This should hopefully be already caught by the mesh_format, but is included just in case here.

	// Don't perform these checks on the first Mesh, the first Mesh is a master
	// and determines the attributes we want to be present.
	if (r_verts.size() != 0) {
		if ((bool)r_norms.size() != (bool)normals.size()) {
			ERR_FAIL_MSG("Attribute mismatch with first Mesh (Normals), ignoring surface.");
		}
		if ((bool)r_tangents.size() != (bool)tangents.size()) {
			ERR_FAIL_MSG("Attribute mismatch with first Mesh (Tangents), ignoring surface.");
		}
		if ((bool)r_colors.size() != (bool)colors.size()) {
			ERR_FAIL_MSG("Attribute mismatch with first Mesh (Colors), ignoring surface.");
		}
		if ((bool)r_uvs.size() != (bool)uvs.size()) {
			ERR_FAIL_MSG("Attribute mismatch with first Mesh (UVs), ignoring surface.");
		}
		if ((bool)r_uv2s.size() != (bool)uv2s.size()) {
			ERR_FAIL_MSG("Attribute mismatch with first Mesh (UV2s), ignoring surface.");
		}
	}

	// The checking for valid triangles should be on WORLD SPACE vertices,
	// NOT model space

	// special case, if no indices, create some
	int num_indices_before = indices.size();
	if (!_ensure_indices_valid(indices, verts)) {
		_merge_log("\tignoring INVALID TRIANGLES (duplicate indices or zero area triangle) detected in " + p_mi.get_name() + ", num inds before / after " + itos(num_indices_before) + " / " + itos(indices.size()));
	}

	// the first index of this mesh is offset from the verts we already have stored in the merged mesh
	int starting_index = r_verts.size();

	// transform verts to world space
	Transform tr = p_mi.get_global_transform();

	// But relative to the destination transform.
	// This can either be identity (when the destination is global space),
	// or the global transform of the owner MeshInstance (if using local space is selected).
	tr = p_dest_tr_inv * tr;

	// to transform normals
	Basis normal_basis = tr.basis.inverse();
	normal_basis.transpose();

	int num_verts = verts.size();

	// verts
	DEV_ASSERT(num_verts > 0);
	int first_vert = r_verts.size();
	r_verts.resize(first_vert + num_verts);
	Vector3 *dest_verts = &r_verts[first_vert];

	for (int n = 0; n < num_verts; n++) {
		Vector3 pt_world = tr.xform(verts[n]);
		*dest_verts++ = pt_world;
	}

	// normals
	if (normals.size()) {
		int first_norm = r_norms.size();
		r_norms.resize(first_norm + num_verts);
		Vector3 *dest_norms = &r_norms[first_norm];
		for (int n = 0; n < num_verts; n++) {
			Vector3 pt_norm = normal_basis.xform(normals[n]);
			pt_norm.normalize();
			*dest_norms++ = pt_norm;
		}
	}

	// tangents
	if (tangents.size()) {
		int first_tang = r_tangents.size();
		r_tangents.resize(first_tang + (num_verts * 4));
		real_t *dest_tangents = &r_tangents[first_tang];

		for (int n = 0; n < num_verts; n++) {
			int tstart = n * 4;
			Vector3 pt_tangent = Vector3(tangents[tstart], tangents[tstart + 1], tangents[tstart + 2]);
			real_t fourth = tangents[tstart + 3];

			pt_tangent = normal_basis.xform(pt_tangent);
			pt_tangent.normalize();
			*dest_tangents++ = pt_tangent.x;
			*dest_tangents++ = pt_tangent.y;
			*dest_tangents++ = pt_tangent.z;
			*dest_tangents++ = fourth;
		}
	}

	// colors
	if (colors.size()) {
		int first_col = r_colors.size();
		r_colors.resize(first_col + num_verts);
		Color *dest_colors = &r_colors[first_col];

		for (int n = 0; n < num_verts; n++) {
			*dest_colors++ = colors[n];
		}
	}

	// uvs
	if (uvs.size()) {
		int first_uv = r_uvs.size();
		r_uvs.resize(first_uv + num_verts);
		Vector2 *dest_uvs = &r_uvs[first_uv];

		for (int n = 0; n < num_verts; n++) {
			*dest_uvs++ = uvs[n];
		}
	}

	// uv2s
	if (uv2s.size()) {
		int first_uv2 = r_uv2s.size();
		r_uv2s.resize(first_uv2 + num_verts);
		Vector2 *dest_uv2s = &r_uv2s[first_uv2];

		for (int n = 0; n < num_verts; n++) {
			*dest_uv2s++ = uv2s[n];
		}
	}

	// indices
	if (indices.size()) {
		int first_ind = r_inds.size();
		r_inds.resize(first_ind + indices.size());
		int *dest_inds = &r_inds[first_ind];

		for (unsigned int n = 0; n < indices.size(); n++) {
			int ind = indices[n] + starting_index;
			*dest_inds++ = ind;
		}
	}
}

bool MeshInstance::_ensure_indices_valid(LocalVector<int> &r_indices, const PoolVector<Vector3> &p_verts) const {
	// no indices? create some
	if (!r_indices.size()) {
		_merge_log("\t\t\t\tindices are blank, creating...");

		// indices are blank!! let's create some, assuming the mesh is using triangles
		r_indices.resize(p_verts.size());

		// this is assuming each triangle vertex is unique
		for (unsigned int n = 0; n < r_indices.size(); n++) {
			r_indices[n] = n;
		}
	}

	if (!_check_for_valid_indices(r_indices, p_verts, nullptr)) {
		LocalVector<int> new_inds;
		_check_for_valid_indices(r_indices, p_verts, &new_inds);

		// copy the new indices
		r_indices = new_inds;

		return false;
	}

	return true;
}

// check for invalid tris, or make a list of the valid triangles, depending on whether r_inds is set
bool MeshInstance::_check_for_valid_indices(const LocalVector<int> &p_inds, const PoolVector<Vector3> &p_verts, LocalVector<int> *r_inds) const {
	int nTris = p_inds.size();
	nTris /= 3;
	int indCount = 0;

	for (int t = 0; t < nTris; t++) {
		int i0 = p_inds[indCount++];
		int i1 = p_inds[indCount++];
		int i2 = p_inds[indCount++];

		bool ok = true;

		// if the indices are the same, the triangle is invalid
		if (i0 == i1) {
			ok = false;
		}
		if (i1 == i2) {
			ok = false;
		}
		if (i0 == i2) {
			ok = false;
		}

		// check positions
		if (ok) {
			// vertex positions
			const Vector3 &p0 = p_verts[i0];
			const Vector3 &p1 = p_verts[i1];
			const Vector3 &p2 = p_verts[i2];

			// if the area is zero, the triangle is invalid (and will crash xatlas if we use it)
			if (_triangle_is_degenerate(p0, p1, p2, 0.00001)) {
				_merge_log("\t\tdetected zero area triangle, ignoring");
				ok = false;
			}
		}

		if (ok) {
			// if the triangle is ok, we will output it if we are outputting
			if (r_inds) {
				r_inds->push_back(i0);
				r_inds->push_back(i1);
				r_inds->push_back(i2);
			}
		} else {
			// if triangle not ok, return failed check if we are not outputting
			if (!r_inds) {
				return false;
			}
		}
	}

	return true;
}

bool MeshInstance::_triangle_is_degenerate(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c, real_t p_epsilon) const {
	// not interested in the actual area, but numerical stability
	Vector3 edge1 = p_b - p_a;
	Vector3 edge2 = p_c - p_a;

	// for numerical stability keep these values reasonably high
	edge1 *= 1024.0;
	edge2 *= 1024.0;

	Vector3 vec = edge1.cross(edge2);
	real_t sl = vec.length_squared();

	if (sl <= p_epsilon) {
		return true;
	}

	return false;
}

// If p_check_compatibility is set to false you MUST have performed a prior check using
// is_mergeable_with, otherwise you could get mismatching surface formats leading to graphical errors etc.
bool MeshInstance::_merge_meshes(Vector<MeshInstance *> p_list, bool p_use_global_space, bool p_check_compatibility) {
	if (p_list.size() < 1) {
		// should not happen but just in case
		return false;
	}

	// use the first mesh instance to get common data like number of surfaces
	const MeshInstance *first = p_list[0];

	// Mesh compatibility checking. This is relatively expensive, so if done already (e.g. in Room system)
	// this step can be avoided.
	LocalVector<bool> compat_list;
	if (p_check_compatibility) {
		compat_list.resize(p_list.size());

		for (int n = 0; n < p_list.size(); n++) {
			compat_list[n] = false;
		}

		compat_list[0] = true;

		for (uint32_t n = 1; n < compat_list.size(); n++) {
			compat_list[n] = first->_is_mergeable_with(*p_list[n]);

			if (compat_list[n] == false) {
				WARN_PRINT("MeshInstance " + p_list[n]->get_name() + " is incompatible for merging with " + first->get_name() + ", ignoring.");
			}
		}
	}

	Ref<ArrayMesh> am;
	am.instance();

	// If we want a local space result, we need the world space transform of this MeshInstance
	// available to back transform verts from world space.
	Transform dest_tr_inv;
	if (!p_use_global_space) {
		if (is_inside_tree()) {
			dest_tr_inv = get_global_transform();
			dest_tr_inv.affine_invert();
		} else {
			WARN_PRINT("MeshInstance must be inside tree to merge using local space, falling back to global space.");
		}
	}

	for (int s = 0; s < first->get_mesh()->get_surface_count(); s++) {
		LocalVector<Vector3> verts;
		LocalVector<Vector3> normals;
		LocalVector<real_t> tangents;
		LocalVector<Color> colors;
		LocalVector<Vector2> uvs;
		LocalVector<Vector2> uv2s;
		LocalVector<int> inds;

		for (int n = 0; n < p_list.size(); n++) {
			// Ignore if the mesh is incompatible
			if (p_check_compatibility && (!compat_list[n])) {
				continue;
			}

			_merge_into_mesh_data(*p_list[n], dest_tr_inv, s, verts, normals, tangents, colors, uvs, uv2s, inds);
		} // for n through source meshes

		if (!verts.size()) {
			WARN_PRINT_ONCE("No vertices for surface");
		}

		// sanity check on the indices
		for (unsigned int n = 0; n < inds.size(); n++) {
			int i = inds[n];
			if ((unsigned int)i >= verts.size()) {
				WARN_PRINT_ONCE("Mesh index out of range, invalid mesh, aborting");
				return false;
			}
		}

		Array arr;
		arr.resize(Mesh::ARRAY_MAX);
		arr[Mesh::ARRAY_VERTEX] = PoolVector<Vector3>(verts);
		if (normals.size()) {
			arr[Mesh::ARRAY_NORMAL] = PoolVector<Vector3>(normals);
		}
		if (tangents.size()) {
			arr[Mesh::ARRAY_TANGENT] = PoolVector<real_t>(tangents);
		}
		if (colors.size()) {
			arr[Mesh::ARRAY_COLOR] = PoolVector<Color>(colors);
		}
		if (uvs.size()) {
			arr[Mesh::ARRAY_TEX_UV] = PoolVector<Vector2>(uvs);
		}
		if (uv2s.size()) {
			arr[Mesh::ARRAY_TEX_UV2] = PoolVector<Vector2>(uv2s);
		}
		arr[Mesh::ARRAY_INDEX] = PoolVector<int>(inds);

		am->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr, Array(), Mesh::ARRAY_COMPRESS_DEFAULT);
	} // for s through surfaces

	// set all the surfaces on the mesh
	set_mesh(am);

	// set merged materials
	int num_surfaces = first->get_mesh()->get_surface_count();
	for (int n = 0; n < num_surfaces; n++) {
		set_surface_material(n, first->get_active_material(n));
	}

	// set some properties to match the merged meshes
	set_material_overlay(first->get_material_overlay());
	set_material_override(first->get_material_override());
	set_cast_shadows_setting(first->get_cast_shadows_setting());

	return true;
}

void MeshInstance::_merge_log(String p_string) const {
	print_verbose(p_string);
}

void MeshInstance::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &MeshInstance::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &MeshInstance::get_mesh);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "Mesh"), "set_mesh", "get_mesh");

	ClassDB::bind_method(D_METHOD("get_surface_material_count"), &MeshInstance::get_surface_material_count);
	ClassDB::bind_method(D_METHOD("set_surface_material", "surface", "material"), &MeshInstance::set_surface_material);
	ClassDB::bind_method(D_METHOD("get_surface_material", "surface"), &MeshInstance::get_surface_material);
	ClassDB::bind_method(D_METHOD("get_active_material", "surface"), &MeshInstance::get_active_material);

	ClassDB::bind_method(D_METHOD("create_trimesh_collision"), &MeshInstance::create_trimesh_collision);
	ClassDB::set_method_flags("MeshInstance", "create_trimesh_collision", METHOD_FLAGS_DEFAULT);
	ClassDB::bind_method(D_METHOD("create_multiple_convex_collisions"), &MeshInstance::create_multiple_convex_collisions);
	ClassDB::set_method_flags("MeshInstance", "create_multiple_convex_collisions", METHOD_FLAGS_DEFAULT);
	ClassDB::bind_method(D_METHOD("create_convex_collision", "clean", "simplify"), &MeshInstance::create_convex_collision, DEFVAL(true), DEFVAL(false));
	ClassDB::set_method_flags("MeshInstance", "create_convex_collision", METHOD_FLAGS_DEFAULT);
	ClassDB::bind_method(D_METHOD("_mesh_changed"), &MeshInstance::_mesh_changed);

	ClassDB::bind_method(D_METHOD("create_debug_tangents"), &MeshInstance::create_debug_tangents);
	ClassDB::set_method_flags("MeshInstance", "create_debug_tangents", METHOD_FLAGS_DEFAULT | METHOD_FLAG_EDITOR);

	ClassDB::bind_method(D_METHOD("is_mergeable_with", "other_mesh_instance"), &MeshInstance::is_mergeable_with);
	ClassDB::bind_method(D_METHOD("merge_meshes", "mesh_instances", "use_global_space", "check_compatibility"), &MeshInstance::merge_meshes, DEFVAL(Vector<Variant>()), DEFVAL(false), DEFVAL(true));
	ClassDB::set_method_flags("MeshInstance", "merge_meshes", METHOD_FLAGS_DEFAULT);
}

MeshInstance::MeshInstance() {
}

MeshInstance::~MeshInstance() {
}

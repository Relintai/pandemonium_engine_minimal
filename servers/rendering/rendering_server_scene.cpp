/*************************************************************************/
/*  rendering_server_scene.cpp                                              */
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

#include "rendering_server_scene.h"

#include "core/config/project_settings.h"
#include "core/math/transform_interpolator.h"
#include "core/os/os.h"
#include "rendering_server_globals.h"
#include "rendering_server_raster.h"

#include <new>

/* CAMERA API */

Transform RenderingServerScene::Camera::get_transform_interpolated() const {
	if (!interpolated) {
		return transform;
	}

	Transform final;
	TransformInterpolator::interpolate_transform_via_method(transform_prev, transform, final, Engine::get_singleton()->get_physics_interpolation_fraction(), interpolation_method);
	return final;
}

RID RenderingServerScene::camera_create() {
	Camera *camera = memnew(Camera);
	return camera_owner.make_rid(camera);
}

void RenderingServerScene::camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);
	camera->type = Camera::PERSPECTIVE;
	camera->fov = p_fovy_degrees;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RenderingServerScene::camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);
	camera->type = Camera::ORTHOGONAL;
	camera->size = p_size;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RenderingServerScene::camera_set_frustum(RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);
	camera->type = Camera::FRUSTUM;
	camera->size = p_size;
	camera->offset = p_offset;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RenderingServerScene::camera_reset_physics_interpolation(RID p_camera) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);

	if (_interpolation_data.interpolation_enabled && camera->interpolated) {
		_interpolation_data.camera_teleport_list.push_back(p_camera);
	}
}

void RenderingServerScene::camera_set_interpolated(RID p_camera, bool p_interpolated) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);
	camera->interpolated = p_interpolated;
}

void RenderingServerScene::camera_set_transform(RID p_camera, const Transform &p_transform) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);

	camera->transform = p_transform.orthonormalized();

	if (_interpolation_data.interpolation_enabled && camera->interpolated) {
		if (!camera->on_interpolate_transform_list) {
			_interpolation_data.camera_transform_update_list_curr->push_back(p_camera);
			camera->on_interpolate_transform_list = true;
		}

		// decide on the interpolation method .. slerp if possible
		camera->interpolation_method = TransformInterpolator::find_method(camera->transform_prev.basis, camera->transform.basis);
	}
}

void RenderingServerScene::camera_set_cull_mask(RID p_camera, uint32_t p_layers) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);

	camera->visible_layers = p_layers;
}

void RenderingServerScene::camera_set_environment(RID p_camera, RID p_env) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);
	camera->env = p_env;
}

void RenderingServerScene::camera_set_use_vertical_aspect(RID p_camera, bool p_enable) {
	Camera *camera = camera_owner.get(p_camera);
	ERR_FAIL_COND(!camera);
	camera->vaspect = p_enable;
}

/* SPATIAL PARTITIONING */

RenderingServerScene::SpatialPartitioningScene_BVH::SpatialPartitioningScene_BVH() {
	_bvh.params_set_thread_safe(GLOBAL_GET("rendering/threads/thread_safe_bvh"));
	_bvh.params_set_pairing_expansion(GLOBAL_GET("rendering/quality/spatial_partitioning/bvh_collision_margin"));

	_dummy_cull_object = memnew(Instance);
}

RenderingServerScene::SpatialPartitioningScene_BVH::~SpatialPartitioningScene_BVH() {
	if (_dummy_cull_object) {
		memdelete(_dummy_cull_object);
		_dummy_cull_object = nullptr;
	}
}

RenderingServerScene::SpatialPartitionID RenderingServerScene::SpatialPartitioningScene_BVH::create(Instance *p_userdata, const AABB &p_aabb, int p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask) {
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
	// we are relying on this instance to be valid in order to pass
	// the visible flag to the bvh.
	DEV_ASSERT(p_userdata);
#endif

	// cache the pairable mask and pairable type on the instance as it is needed for user callbacks from the BVH, and this is
	// too complex to calculate each callback...
	p_userdata->bvh_pairable_mask = p_pairable_mask;
	p_userdata->bvh_pairable_type = p_pairable_type;

	uint32_t tree_collision_mask = 0;
	uint32_t tree_id = find_tree_id_and_collision_mask(p_pairable, tree_collision_mask);

	return _bvh.create(p_userdata, p_userdata->visible, tree_id, tree_collision_mask, p_aabb, p_subindex) + 1;
}

void RenderingServerScene::SpatialPartitioningScene_BVH::erase(SpatialPartitionID p_handle) {
	_bvh.erase(p_handle - 1);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::move(SpatialPartitionID p_handle, const AABB &p_aabb) {
	_bvh.move(p_handle - 1, p_aabb);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::activate(SpatialPartitionID p_handle, const AABB &p_aabb) {
	// be very careful here, we are deferring the collision check, expecting a set_pairable to be called
	// immediately after.
	// see the notes in the BVH function.
	_bvh.activate(p_handle - 1, p_aabb, true);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::deactivate(SpatialPartitionID p_handle) {
	_bvh.deactivate(p_handle - 1);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::force_collision_check(SpatialPartitionID p_handle) {
	_bvh.force_collision_check(p_handle - 1);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::update() {
	_bvh.update();
}

void RenderingServerScene::SpatialPartitioningScene_BVH::update_collisions() {
	_bvh.update_collisions();
}

void RenderingServerScene::SpatialPartitioningScene_BVH::set_pairable(Instance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask) {
	SpatialPartitionID handle = p_instance->spatial_partition_id;

	p_instance->bvh_pairable_mask = p_pairable_mask;
	p_instance->bvh_pairable_type = p_pairable_type;

	uint32_t tree_collision_mask = 0;
	uint32_t tree_id = find_tree_id_and_collision_mask(p_pairable, tree_collision_mask);

	_bvh.set_tree(handle - 1, tree_id, tree_collision_mask);
}

int RenderingServerScene::SpatialPartitioningScene_BVH::cull_convex(const Vector<Plane> &p_convex, Instance **p_result_array, int p_result_max, uint32_t p_mask) {
	_dummy_cull_object->bvh_pairable_mask = p_mask;
	_dummy_cull_object->bvh_pairable_type = 0;
	return _bvh.cull_convex(p_convex, p_result_array, p_result_max, _dummy_cull_object);
}

int RenderingServerScene::SpatialPartitioningScene_BVH::cull_aabb(const AABB &p_aabb, Instance **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask) {
	_dummy_cull_object->bvh_pairable_mask = p_mask;
	_dummy_cull_object->bvh_pairable_type = 0;
	return _bvh.cull_aabb(p_aabb, p_result_array, p_result_max, _dummy_cull_object, 0xFFFFFFFF, p_subindex_array);
}

int RenderingServerScene::SpatialPartitioningScene_BVH::cull_segment(const Vector3 &p_from, const Vector3 &p_to, Instance **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask) {
	_dummy_cull_object->bvh_pairable_mask = p_mask;
	_dummy_cull_object->bvh_pairable_type = 0;
	return _bvh.cull_segment(p_from, p_to, p_result_array, p_result_max, _dummy_cull_object, 0xFFFFFFFF, p_subindex_array);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::set_pair_callback(PairCallback p_callback, void *p_userdata) {
	_bvh.set_pair_callback(p_callback, p_userdata);
}

void RenderingServerScene::SpatialPartitioningScene_BVH::set_unpair_callback(UnpairCallback p_callback, void *p_userdata) {
	_bvh.set_unpair_callback(p_callback, p_userdata);
}

///////////////////////

RenderingServerScene::SpatialPartitionID RenderingServerScene::SpatialPartitioningScene_Octree::create(Instance *p_userdata, const AABB &p_aabb, int p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask) {
	return _octree.create(p_userdata, p_aabb, p_subindex, p_pairable, p_pairable_type, p_pairable_mask);
}

void RenderingServerScene::SpatialPartitioningScene_Octree::erase(SpatialPartitionID p_handle) {
	_octree.erase(p_handle);
}

void RenderingServerScene::SpatialPartitioningScene_Octree::move(SpatialPartitionID p_handle, const AABB &p_aabb) {
	_octree.move(p_handle, p_aabb);
}

void RenderingServerScene::SpatialPartitioningScene_Octree::set_pairable(Instance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask) {
	SpatialPartitionID handle = p_instance->spatial_partition_id;
	_octree.set_pairable(handle, p_pairable, p_pairable_type, p_pairable_mask);
}

int RenderingServerScene::SpatialPartitioningScene_Octree::cull_convex(const Vector<Plane> &p_convex, Instance **p_result_array, int p_result_max, uint32_t p_mask) {
	return _octree.cull_convex(p_convex, p_result_array, p_result_max, p_mask);
}

int RenderingServerScene::SpatialPartitioningScene_Octree::cull_aabb(const AABB &p_aabb, Instance **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask) {
	return _octree.cull_aabb(p_aabb, p_result_array, p_result_max, p_subindex_array, p_mask);
}

int RenderingServerScene::SpatialPartitioningScene_Octree::cull_segment(const Vector3 &p_from, const Vector3 &p_to, Instance **p_result_array, int p_result_max, int *p_subindex_array, uint32_t p_mask) {
	return _octree.cull_segment(p_from, p_to, p_result_array, p_result_max, p_subindex_array, p_mask);
}

void RenderingServerScene::SpatialPartitioningScene_Octree::set_pair_callback(PairCallback p_callback, void *p_userdata) {
	_octree.set_pair_callback(p_callback, p_userdata);
}

void RenderingServerScene::SpatialPartitioningScene_Octree::set_unpair_callback(UnpairCallback p_callback, void *p_userdata) {
	_octree.set_unpair_callback(p_callback, p_userdata);
}

void RenderingServerScene::SpatialPartitioningScene_Octree::set_balance(float p_balance) {
	_octree.set_balance(p_balance);
}

/* SCENARIO API */

RenderingServerScene::Scenario::Scenario() {
	debug = RS::SCENARIO_DEBUG_DISABLED;

	bool use_bvh_or_octree = GLOBAL_GET("rendering/quality/spatial_partitioning/use_bvh");

	if (use_bvh_or_octree) {
		sps = memnew(SpatialPartitioningScene_BVH);
	} else {
		sps = memnew(SpatialPartitioningScene_Octree);
	}
}

void *RenderingServerScene::_instance_pair(void *p_self, SpatialPartitionID, Instance *p_A, int, SpatialPartitionID, Instance *p_B, int) {
	//RenderingServerScene *self = (RenderingServerScene*)p_self;
	Instance *A = p_A;
	Instance *B = p_B;

	//instance indices are designed so greater always contains lesser
	if (A->base_type > B->base_type) {
		SWAP(A, B); //lesser always first
	}

	return nullptr;
}

void RenderingServerScene::_instance_unpair(void *p_self, SpatialPartitionID, Instance *p_A, int, SpatialPartitionID, Instance *p_B, int, void *udata) {
	//RenderingServerScene *self = (RenderingServerScene*)p_self;
	Instance *A = p_A;
	Instance *B = p_B;

	//instance indices are designed so greater always contains lesser
	if (A->base_type > B->base_type) {
		SWAP(A, B); //lesser always first
	}
}

RID RenderingServerScene::scenario_create() {
	Scenario *scenario = memnew(Scenario);
	ERR_FAIL_COND_V(!scenario, RID());
	RID scenario_rid = scenario_owner.make_rid(scenario);
	scenario->self = scenario_rid;

	scenario->sps->set_balance(GLOBAL_GET("rendering/quality/spatial_partitioning/render_tree_balance"));
	scenario->sps->set_pair_callback(_instance_pair, this);
	scenario->sps->set_unpair_callback(_instance_unpair, this);

	return scenario_rid;
}

void RenderingServerScene::set_physics_interpolation_enabled(bool p_enabled) {
	_interpolation_data.interpolation_enabled = p_enabled;
}

void RenderingServerScene::tick() {
	if (_interpolation_data.interpolation_enabled) {
		update_interpolation_tick(true);
	}
}

void RenderingServerScene::pre_draw(bool p_will_draw) {
	// even when running and not drawing scenes, we still need to clear intermediate per frame
	// interpolation data .. hence the p_will_draw flag (so we can reduce the processing if the frame
	// will not be drawn)
	if (_interpolation_data.interpolation_enabled) {
		update_interpolation_frame(p_will_draw);
	}
}

void RenderingServerScene::scenario_set_debug(RID p_scenario, RS::ScenarioDebugMode p_debug_mode) {
	Scenario *scenario = scenario_owner.get(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->debug = p_debug_mode;
}

/* INSTANCING API */

void RenderingServerScene::_instance_queue_update(Instance *p_instance, bool p_update_aabb, bool p_update_materials) {
	if (p_update_aabb) {
		p_instance->update_aabb = true;
	}
	if (p_update_materials) {
		p_instance->update_materials = true;
	}

	if (p_instance->update_item.in_list()) {
		return;
	}

	_instance_update_list.add(&p_instance->update_item);
}

void RenderingServerScene::InterpolationData::notify_free_camera(RID p_rid, Camera &r_camera) {
	r_camera.on_interpolate_transform_list = false;

	if (!interpolation_enabled) {
		return;
	}

	// if the camera was on any of the lists, remove
	camera_transform_update_list_curr->erase_multiple_unordered(p_rid);
	camera_transform_update_list_prev->erase_multiple_unordered(p_rid);
	camera_teleport_list.erase_multiple_unordered(p_rid);
}

void RenderingServerScene::InterpolationData::notify_free_instance(RID p_rid, Instance &r_instance) {
	r_instance.on_interpolate_list = false;
	r_instance.on_interpolate_transform_list = false;

	if (!interpolation_enabled) {
		return;
	}

	// if the instance was on any of the lists, remove
	instance_interpolate_update_list.erase_multiple_unordered(p_rid);
	instance_transform_update_list_curr->erase_multiple_unordered(p_rid);
	instance_transform_update_list_prev->erase_multiple_unordered(p_rid);
	instance_teleport_list.erase_multiple_unordered(p_rid);
}

void RenderingServerScene::update_interpolation_tick(bool p_process) {
	// update interpolation in storage
	RSG::storage->update_interpolation_tick(p_process);

	// detect any that were on the previous transform list that are no longer active,
	// we should remove them from the interpolate list

	for (unsigned int n = 0; n < _interpolation_data.instance_transform_update_list_prev->size(); n++) {
		const RID &rid = (*_interpolation_data.instance_transform_update_list_prev)[n];
		Instance *instance = instance_owner.getornull(rid);

		bool active = true;

		// no longer active? (either the instance deleted or no longer being transformed)
		if (instance && !instance->on_interpolate_transform_list) {
			active = false;
			instance->on_interpolate_list = false;

			// make sure the most recent transform is set
			instance->transform = instance->transform_curr;

			// and that both prev and current are the same, just in case of any interpolations
			instance->transform_prev = instance->transform_curr;

			// make sure are updated one more time to ensure the AABBs are correct
			_instance_queue_update(instance, true);
		}

		if (!instance) {
			active = false;
		}

		if (!active) {
			_interpolation_data.instance_interpolate_update_list.erase(rid);
		}
	}

	// and now for any in the transform list (being actively interpolated), keep the previous transform
	// value up to date ready for the next tick
	if (p_process) {
		for (unsigned int n = 0; n < _interpolation_data.instance_transform_update_list_curr->size(); n++) {
			const RID &rid = (*_interpolation_data.instance_transform_update_list_curr)[n];
			Instance *instance = instance_owner.getornull(rid);
			if (instance) {
				instance->transform_prev = instance->transform_curr;
				instance->transform_checksum_prev = instance->transform_checksum_curr;
				instance->on_interpolate_transform_list = false;
			}
		}
	}

	// we maintain a mirror list for the transform updates, so we can detect when an instance
	// is no longer being transformed, and remove it from the interpolate list
	SWAP(_interpolation_data.instance_transform_update_list_curr, _interpolation_data.instance_transform_update_list_prev);

	// prepare for the next iteration
	_interpolation_data.instance_transform_update_list_curr->clear();

	// CAMERAS
	// detect any that were on the previous transform list that are no longer active,
	for (unsigned int n = 0; n < _interpolation_data.camera_transform_update_list_prev->size(); n++) {
		const RID &rid = (*_interpolation_data.camera_transform_update_list_prev)[n];
		Camera *camera = camera_owner.getornull(rid);

		// no longer active? (either the instance deleted or no longer being transformed)
		if (camera && !camera->on_interpolate_transform_list) {
			camera->transform = camera->transform_prev;
		}
	}

	// cameras , swap any current with previous
	for (unsigned int n = 0; n < _interpolation_data.camera_transform_update_list_curr->size(); n++) {
		const RID &rid = (*_interpolation_data.camera_transform_update_list_curr)[n];
		Camera *camera = camera_owner.getornull(rid);
		if (camera) {
			camera->transform_prev = camera->transform;
			camera->on_interpolate_transform_list = false;
		}
	}

	// we maintain a mirror list for the transform updates, so we can detect when an instance
	// is no longer being transformed, and remove it from the interpolate list
	SWAP(_interpolation_data.camera_transform_update_list_curr, _interpolation_data.camera_transform_update_list_prev);

	// prepare for the next iteration
	_interpolation_data.camera_transform_update_list_curr->clear();
}

void RenderingServerScene::update_interpolation_frame(bool p_process) {
	// update interpolation in storage
	RSG::storage->update_interpolation_frame(p_process);

	// teleported instances
	for (unsigned int n = 0; n < _interpolation_data.instance_teleport_list.size(); n++) {
		const RID &rid = _interpolation_data.instance_teleport_list[n];
		Instance *instance = instance_owner.getornull(rid);
		if (instance) {
			instance->transform_prev = instance->transform_curr;
			instance->transform_checksum_prev = instance->transform_checksum_curr;
		}
	}

	_interpolation_data.instance_teleport_list.clear();

	// camera teleports
	for (unsigned int n = 0; n < _interpolation_data.camera_teleport_list.size(); n++) {
		const RID &rid = _interpolation_data.camera_teleport_list[n];
		Camera *camera = camera_owner.getornull(rid);
		if (camera) {
			camera->transform_prev = camera->transform;
		}
	}

	_interpolation_data.camera_teleport_list.clear();

	if (p_process) {
		real_t f = Engine::get_singleton()->get_physics_interpolation_fraction();

		for (unsigned int i = 0; i < _interpolation_data.instance_interpolate_update_list.size(); i++) {
			const RID &rid = _interpolation_data.instance_interpolate_update_list[i];
			Instance *instance = instance_owner.getornull(rid);
			if (instance) {
				TransformInterpolator::interpolate_transform_via_method(instance->transform_prev, instance->transform_curr, instance->transform, f, instance->interpolation_method);

				// make sure AABBs are constantly up to date through the interpolation
				_instance_queue_update(instance, true);
			}
		} // for n
	}
}

// Rooms
void RenderingServerScene::callbacks_register(RenderingServerCallbacks *p_callbacks) {
	_rendering_server_callbacks = p_callbacks;
}


void RenderingServerScene::_update_instance(Instance *p_instance) {
	p_instance->version++;

	// when not using interpolation the transform is used straight
	const Transform *instance_xform = &p_instance->transform;

	// Can possibly use the most up to date current transform here when using physics interpolation ..
	// uncomment the next line for this..
	// if (p_instance->is_currently_interpolated()) {
	// instance_xform = &p_instance->transform_curr;
	// }
	// However it does seem that using the interpolated transform (transform) works for keeping AABBs
	// up to date to avoid culling errors.

	if (p_instance->aabb.has_no_surface()) {
		return;
	}

	p_instance->mirror = instance_xform->basis.determinant() < 0.0;

	AABB new_aabb;

	new_aabb = instance_xform->xform(p_instance->aabb);

	p_instance->transformed_aabb = new_aabb;

	if (!p_instance->scenario) {
		return;
	}

	if (p_instance->spatial_partition_id == 0) {
		uint32_t base_type = 1 << p_instance->base_type;
		uint32_t pairable_mask = 0;
		bool pairable = false;

		// not inside octree
		p_instance->spatial_partition_id = p_instance->scenario->sps->create(p_instance, new_aabb, 0, pairable, base_type, pairable_mask);

	} else {
		/*
		if (new_aabb==p_instance->data.transformed_aabb)
			return;
		*/

		p_instance->scenario->sps->move(p_instance->spatial_partition_id, new_aabb);
	}

}

void RenderingServerScene::_update_instance_aabb(Instance *p_instance) {
	AABB new_aabb;

	ERR_FAIL_COND(p_instance->base_type != RS::INSTANCE_NONE && !p_instance->base.is_valid());

	switch (p_instance->base_type) {
		case RenderingServer::INSTANCE_NONE: {
			// do nothing
		} break;
		case RenderingServer::INSTANCE_MESH: {
			if (p_instance->custom_aabb) {
				new_aabb = *p_instance->custom_aabb;
			} else {
				new_aabb = RSG::storage->mesh_get_aabb(p_instance->base);
			}

		} break;

		case RenderingServer::INSTANCE_MULTIMESH: {
			if (p_instance->custom_aabb) {
				new_aabb = *p_instance->custom_aabb;
			} else {
				new_aabb = RSG::storage->multimesh_get_aabb(p_instance->base);
			}

		} break;
		default: {
		}
	}

	// <Zylann> This is why I didn't re-use Instance::aabb to implement custom AABBs
	if (p_instance->extra_margin) {
		new_aabb.grow_by(p_instance->extra_margin);
	}

	p_instance->aabb = new_aabb;
}

void RenderingServerScene::_update_dirty_instance(Instance *p_instance) {
	if (p_instance->update_aabb) {
		_update_instance_aabb(p_instance);
	}

	if (p_instance->update_materials) {
		if (p_instance->base_type == RS::INSTANCE_MESH) {
			//remove materials no longer used and un-own them

			int new_mat_count = RSG::storage->mesh_get_surface_count(p_instance->base);
			for (int i = p_instance->materials.size() - 1; i >= new_mat_count; i--) {
				if (p_instance->materials[i].is_valid()) {
					RSG::storage->material_remove_instance_owner(p_instance->materials[i], p_instance);
				}
			}
			p_instance->materials.resize(new_mat_count);

			int new_blend_shape_count = RSG::storage->mesh_get_blend_shape_count(p_instance->base);
			if (new_blend_shape_count != p_instance->blend_values.size()) {
				p_instance->blend_values.resize(new_blend_shape_count);
				for (int i = 0; i < new_blend_shape_count; i++) {
					p_instance->blend_values.write().ptr()[i] = 0;
				}
			}
		}

		if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
			InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);

			bool is_animated = false;

			 if (p_instance->material_override.is_valid()) {
				is_animated = RSG::storage->material_is_animated(p_instance->material_override);
			} else {
				if (p_instance->base_type == RS::INSTANCE_MESH) {
					RID mesh = p_instance->base;

					if (mesh.is_valid()) {
						for (int i = 0; i < p_instance->materials.size(); i++) {
							RID mat = p_instance->materials[i].is_valid() ? p_instance->materials[i] : RSG::storage->mesh_surface_get_material(mesh, i);

							if (mat.is_valid()) {
								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}
							}
						}
					}

				} else if (p_instance->base_type == RS::INSTANCE_MULTIMESH) {
					RID mesh = RSG::storage->multimesh_get_mesh(p_instance->base);
					if (mesh.is_valid()) {
						int sc = RSG::storage->mesh_get_surface_count(mesh);
						for (int i = 0; i < sc; i++) {
							RID mat = RSG::storage->mesh_surface_get_material(mesh, i);

							if (mat.is_valid()) {
								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}
							}
						}
					}
				}
			}

			if (p_instance->material_overlay.is_valid()) {
				is_animated = is_animated || RSG::storage->material_is_animated(p_instance->material_overlay);
			}

			geom->material_is_animated = is_animated;
		}
	}

	_instance_update_list.remove(&p_instance->update_item);

	_update_instance(p_instance);

	p_instance->update_aabb = false;
	p_instance->update_materials = false;
}

void RenderingServerScene::render_camera(RID p_camera, RID p_scenario, Size2 p_viewport_size) {
// render to mono camera
#ifndef _3D_DISABLED

	Camera *camera = camera_owner.getornull(p_camera);
	ERR_FAIL_COND(!camera);

	/* STEP 1 - SETUP CAMERA */
	Projection camera_matrix;
	bool ortho = false;

	switch (camera->type) {
		case Camera::ORTHOGONAL: {
			camera_matrix.set_orthogonal(
					camera->size,
					p_viewport_size.width / (float)p_viewport_size.height,
					camera->znear,
					camera->zfar,
					camera->vaspect);
			ortho = true;
		} break;
		case Camera::PERSPECTIVE: {
			camera_matrix.set_perspective(
					camera->fov,
					p_viewport_size.width / (float)p_viewport_size.height,
					camera->znear,
					camera->zfar,
					camera->vaspect);
			ortho = false;

		} break;
		case Camera::FRUSTUM: {
			camera_matrix.set_frustum(
					camera->size,
					p_viewport_size.width / (float)p_viewport_size.height,
					camera->offset,
					camera->znear,
					camera->zfar,
					camera->vaspect);
			ortho = false;
		} break;
	}

	Transform camera_transform = _interpolation_data.interpolation_enabled ? camera->get_transform_interpolated() : camera->transform;

	_prepare_scene(camera_transform, camera_matrix, ortho, camera->env, camera->visible_layers, p_scenario, camera->previous_room_id_hint);
	_render_scene(camera_transform, camera_matrix, 0, ortho, camera->env, p_scenario);
#endif
}

void RenderingServerScene::_prepare_scene(const Transform p_cam_transform, const Projection &p_cam_projection, bool p_cam_orthogonal, RID p_force_environment, uint32_t p_visible_layers, RID p_scenario, int32_t &r_previous_room_id_hint) {
	// Note, in stereo rendering:
	// - p_cam_transform will be a transform in the middle of our two eyes
	// - p_cam_projection is a wider frustrum that encompasses both eyes

	render_pass++;
	uint32_t camera_layer_mask = p_visible_layers;

	RSG::scene_render->set_scene_pass(render_pass);

	//rasterizer->set_camera(camera->transform, camera_matrix,ortho);

	Vector<Plane> planes = p_cam_projection.get_projection_planes(p_cam_transform);

	Plane near_plane(p_cam_transform.origin, -p_cam_transform.basis.get_axis(2).normalized());
	float z_far = p_cam_projection.get_z_far();

	/* STEP 2 - CULL */
	instance_cull_count = 0;

	//light_samplers_culled=0;

	/*
	print_line("OT: "+rtos( (OS::get_singleton()->get_ticks_usec()-t)/1000.0));
	print_line("OTO: "+itos(p_scenario->octree.get_octant_count()));
	print_line("OTE: "+itos(p_scenario->octree.get_elem_count()));
	print_line("OTP: "+itos(p_scenario->octree.get_pair_count()));
	*/

	/* STEP 3 - PROCESS PORTALS, VALIDATE ROOMS */
	//removed, will replace with culling

	/* STEP 4 - REMOVE FURTHER CULLED OBJECTS, ADD LIGHTS */

	for (int i = 0; i < instance_cull_count; i++) {
		Instance *ins = instance_cull_result[i];

		bool keep = false;

		if ((camera_layer_mask & ins->layer_mask) == 0) {
			//failure
		} else if (((1 << ins->base_type) & RS::INSTANCE_GEOMETRY_MASK) && ins->visible) {
			keep = true;

			InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(ins->base_data);

			if (ins->redraw_if_visible) {
				RenderingServerRaster::redraw_request(false);
			}

			if (geom->lighting_dirty) {
				//only called when lights AABB enter/exit this geometry
				ins->light_instances.resize(geom->lighting.size());
				geom->lighting_dirty = false;
			}

		}

		if (!keep) {
			// remove, no reason to keep
			instance_cull_count--;
			SWAP(instance_cull_result[i], instance_cull_result[instance_cull_count]);
			i--;
			ins->last_render_pass = 0; // make invalid
		} else {
			ins->last_render_pass = render_pass;
		}
	}

	/* STEP 5 - PROCESS LIGHTS */

	// Calculate instance->depth from the camera, after shadow calculation has stopped overwriting instance->depth
	for (int i = 0; i < instance_cull_count; i++) {
		Instance *ins = instance_cull_result[i];

		if (((1 << ins->base_type) & RS::INSTANCE_GEOMETRY_MASK) && ins->visible) {
			Vector3 center = ins->transform.origin;
			if (ins->use_aabb_center) {
				center = ins->transformed_aabb.position + (ins->transformed_aabb.size * 0.5);
			}
			if (p_cam_orthogonal) {
				ins->depth = near_plane.distance_to(center) - ins->sorting_offset;
			} else {
				ins->depth = p_cam_transform.origin.distance_to(center) - ins->sorting_offset;
			}
			ins->depth_layer = CLAMP(int(ins->depth * 16 / z_far), 0, 15);
		}
	}
}

void RenderingServerScene::_render_scene(const Transform p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_orthogonal, RID p_force_environment, RID p_scenario) {
	/* PROCESS GEOMETRY AND DRAW SCENE */

	RSG::scene_render->render_scene(p_cam_transform, p_cam_projection, p_eye, p_cam_orthogonal, (RasterizerScene::InstanceBase **)instance_cull_result, instance_cull_count);
}

void RenderingServerScene::render_empty_scene(RID p_scenario) {
#ifndef _3D_DISABLED
	RSG::scene_render->render_scene(Transform(), Projection(), 0, true, nullptr, 0);
#endif
}

void RenderingServerScene::update_dirty_instances() {
	RSG::storage->update_dirty_resources();

	// this is just to get access to scenario so we can update the spatial partitioning scheme
	Scenario *scenario = nullptr;
	if (_instance_update_list.first()) {
		scenario = _instance_update_list.first()->self()->scenario;
	}

	while (_instance_update_list.first()) {
		_update_dirty_instance(_instance_update_list.first()->self());
	}

	if (scenario) {
		scenario->sps->update();
	}
}

bool RenderingServerScene::free(RID p_rid) {
	if (camera_owner.owns(p_rid)) {
		Camera *camera = camera_owner.get(p_rid);

		_interpolation_data.notify_free_camera(p_rid, *camera);

		camera_owner.free(p_rid);
		memdelete(camera);
	} else if (scenario_owner.owns(p_rid)) {
		Scenario *scenario = scenario_owner.get(p_rid);


		scenario_owner.free(p_rid);
		memdelete(scenario);

	} else if (instance_owner.owns(p_rid)) {
		// delete the instance

		update_dirty_instances();

		Instance *instance = instance_owner.get(p_rid);

		_interpolation_data.notify_free_instance(p_rid, *instance);

		update_dirty_instances(); //in case something changed this

		instance_owner.free(p_rid);
		memdelete(instance);

	} else {
		return false;
	}

	return true;
}

RenderingServerScene *RenderingServerScene::singleton = nullptr;

RenderingServerScene::RenderingServerScene() {
	render_pass = 1;
	singleton = this;
	_use_bvh = GLOBAL_DEF("rendering/quality/spatial_partitioning/use_bvh", true);
	GLOBAL_DEF("rendering/quality/spatial_partitioning/bvh_collision_margin", 0.1);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/quality/spatial_partitioning/bvh_collision_margin", PropertyInfo(Variant::REAL, "rendering/quality/spatial_partitioning/bvh_collision_margin", PROPERTY_HINT_RANGE, "0.0,2.0,0.01"));

	_rendering_server_callbacks = nullptr;
}

RenderingServerScene::~RenderingServerScene() {
}

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

	if (B->base_type == RS::INSTANCE_LIGHT && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightData *light = static_cast<InstanceLightData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		InstanceLightData::PairInfo pinfo;
		pinfo.geometry = A;
		pinfo.L = geom->lighting.push_back(B);

		List<InstanceLightData::PairInfo>::Element *E = light->geometries.push_back(pinfo);

		if (geom->can_cast_shadows) {
			light->shadow_dirty = true;
		}
		geom->lighting_dirty = true;

		return E; //this element should make freeing faster
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

	if (B->base_type == RS::INSTANCE_LIGHT && ((1 << A->base_type) & RS::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightData *light = static_cast<InstanceLightData *>(B->base_data);
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(A->base_data);

		List<InstanceLightData::PairInfo>::Element *E = reinterpret_cast<List<InstanceLightData::PairInfo>::Element *>(udata);

		geom->lighting.erase(E->get().L);
		light->geometries.erase(E);

		if (geom->can_cast_shadows) {
			light->shadow_dirty = true;
		}
		geom->lighting_dirty = true;

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

	scenario->shadow_atlas = RSG::scene_render->shadow_atlas_create();
	RSG::scene_render->shadow_atlas_set_size(scenario->shadow_atlas, 1024); //make enough shadows for close distance, don't bother with rest
	RSG::scene_render->shadow_atlas_set_quadrant_subdivision(scenario->shadow_atlas, 0, 4);
	RSG::scene_render->shadow_atlas_set_quadrant_subdivision(scenario->shadow_atlas, 1, 4);
	RSG::scene_render->shadow_atlas_set_quadrant_subdivision(scenario->shadow_atlas, 2, 4);
	RSG::scene_render->shadow_atlas_set_quadrant_subdivision(scenario->shadow_atlas, 3, 8);

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

void RenderingServerScene::scenario_set_environment(RID p_scenario, RID p_environment) {
	Scenario *scenario = scenario_owner.get(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->environment = p_environment;
}

void RenderingServerScene::scenario_set_fallback_environment(RID p_scenario, RID p_environment) {
	Scenario *scenario = scenario_owner.get(p_scenario);
	ERR_FAIL_COND(!scenario);
	scenario->fallback_environment = p_environment;
}

void RenderingServerScene::scenario_set_reflection_atlas_size(RID p_scenario, int p_size, int p_subdiv) {
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

RID RenderingServerScene::instance_create() {
	Instance *instance = memnew(Instance);
	ERR_FAIL_COND_V(!instance, RID());

	RID instance_rid = instance_owner.make_rid(instance);
	instance->self = instance_rid;

	return instance_rid;
}

void RenderingServerScene::instance_set_base(RID p_instance, RID p_base) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	Scenario *scenario = instance->scenario;

	if (instance->base_type != RS::INSTANCE_NONE) {
		//free anything related to that base

		RSG::storage->instance_remove_dependency(instance->base, instance);

		if (scenario && instance->spatial_partition_id) {
			scenario->sps->erase(instance->spatial_partition_id);
			instance->spatial_partition_id = 0;
		}

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);

				if (instance->scenario && light->D) {
					instance->scenario->directional_lights.erase(light->D);
					light->D = nullptr;
				}
				RSG::scene_render->free(light->instance);
			} break;
			default: {
			}
		}

		if (instance->base_data) {
			memdelete(instance->base_data);
			instance->base_data = nullptr;
		}

		instance->blend_values = PoolRealArray();

		for (int i = 0; i < instance->materials.size(); i++) {
			if (instance->materials[i].is_valid()) {
				RSG::storage->material_remove_instance_owner(instance->materials[i], instance);
			}
		}
		instance->materials.clear();
	}

	instance->base_type = RS::INSTANCE_NONE;
	instance->base = RID();

	if (p_base.is_valid()) {
		instance->base_type = RSG::storage->get_base_type(p_base);
		ERR_FAIL_COND(instance->base_type == RS::INSTANCE_NONE);

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = memnew(InstanceLightData);

				if (scenario && RSG::storage->light_get_type(p_base) == RS::LIGHT_DIRECTIONAL) {
					light->D = scenario->directional_lights.push_back(instance);
				}

				light->instance = RSG::scene_render->light_instance_create(p_base);

				instance->base_data = light;
			} break;
			case RS::INSTANCE_MESH:
			case RS::INSTANCE_MULTIMESH: {
				InstanceGeometryData *geom = memnew(InstanceGeometryData);
				instance->base_data = geom;
				if (instance->base_type == RS::INSTANCE_MESH) {
					instance->blend_values.resize(RSG::storage->mesh_get_blend_shape_count(p_base));
				}
			} break;

			default: {
			}
		}

		RSG::storage->instance_add_dependency(p_base, instance);

		instance->base = p_base;

		if (scenario) {
			_instance_queue_update(instance, true, true);
		}
	}
}
void RenderingServerScene::instance_set_scenario(RID p_instance, RID p_scenario) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->scenario) {
		instance->scenario->instances.remove(&instance->scenario_item);

		if (instance->spatial_partition_id) {
			instance->scenario->sps->erase(instance->spatial_partition_id);
			instance->spatial_partition_id = 0;
		}

		// remove any interpolation data associated with the instance in this scenario
		_interpolation_data.notify_free_instance(p_instance, *instance);

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);

				if (light->D) {
					instance->scenario->directional_lights.erase(light->D);
					light->D = nullptr;
				}
			} break;
			default: {
			}
		}

		instance->scenario = nullptr;
	}

	if (p_scenario.is_valid()) {
		Scenario *scenario = scenario_owner.get(p_scenario);
		ERR_FAIL_COND(!scenario);

		instance->scenario = scenario;

		scenario->instances.add(&instance->scenario_item);

		switch (instance->base_type) {
			case RS::INSTANCE_LIGHT: {
				InstanceLightData *light = static_cast<InstanceLightData *>(instance->base_data);

				if (RSG::storage->light_get_type(instance->base) == RS::LIGHT_DIRECTIONAL) {
					light->D = scenario->directional_lights.push_back(instance);
				}
			} break;
			default: {
			}
		}

		_instance_queue_update(instance, true, true);
	}
}
void RenderingServerScene::instance_set_layer_mask(RID p_instance, uint32_t p_mask) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->layer_mask == p_mask) {
		return;
	}

	instance->layer_mask = p_mask;

	// update lights to show / hide shadows according to the new mask
	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);

		if (geom->can_cast_shadows) {
			for (List<Instance *>::Element *E = geom->lighting.front(); E; E = E->next()) {
				InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
				light->shadow_dirty = true;
			}
		}
	}
}

void RenderingServerScene::instance_set_pivot_data(RID p_instance, float p_sorting_offset, bool p_use_aabb_center) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	instance->sorting_offset = p_sorting_offset;
	instance->use_aabb_center = p_use_aabb_center;
}

void RenderingServerScene::instance_reset_physics_interpolation(RID p_instance) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (_interpolation_data.interpolation_enabled && instance->interpolated) {
		_interpolation_data.instance_teleport_list.push_back(p_instance);
	}
}

void RenderingServerScene::instance_set_interpolated(RID p_instance, bool p_interpolated) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);
	instance->interpolated = p_interpolated;
}

void RenderingServerScene::instance_set_transform(RID p_instance, const Transform &p_transform) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (!(_interpolation_data.interpolation_enabled && instance->interpolated) || !instance->scenario) {
		if (instance->transform == p_transform) {
			return; //must be checked to avoid worst evil
		}

#ifdef DEBUG_ENABLED

		for (int i = 0; i < 4; i++) {
			const Vector3 &v = i < 3 ? p_transform.basis.rows[i] : p_transform.origin;
			ERR_FAIL_COND(Math::is_inf(v.x));
			ERR_FAIL_COND(Math::is_nan(v.x));
			ERR_FAIL_COND(Math::is_inf(v.y));
			ERR_FAIL_COND(Math::is_nan(v.y));
			ERR_FAIL_COND(Math::is_inf(v.z));
			ERR_FAIL_COND(Math::is_nan(v.z));
		}

#endif
		instance->transform = p_transform;
		_instance_queue_update(instance, true);
		return;
	}

	float new_checksum = TransformInterpolator::checksum_transform(p_transform);
	bool checksums_match = (instance->transform_checksum_curr == new_checksum) && (instance->transform_checksum_prev == new_checksum);

	// we can't entirely reject no changes because we need the interpolation
	// system to keep on stewing

	// Optimized check. First checks the checksums. If they pass it does the slow check at the end.
	// Alternatively we can do this non-optimized and ignore the checksum...
	// if no change
	if (checksums_match && (instance->transform_curr == p_transform) && (instance->transform_prev == p_transform)) {
		return;
	}

#ifdef DEBUG_ENABLED

	for (int i = 0; i < 4; i++) {
		const Vector3 &v = i < 3 ? p_transform.basis.rows[i] : p_transform.origin;
		ERR_FAIL_COND(Math::is_inf(v.x));
		ERR_FAIL_COND(Math::is_nan(v.x));
		ERR_FAIL_COND(Math::is_inf(v.y));
		ERR_FAIL_COND(Math::is_nan(v.y));
		ERR_FAIL_COND(Math::is_inf(v.z));
		ERR_FAIL_COND(Math::is_nan(v.z));
	}

#endif

	instance->transform_curr = p_transform;

	// keep checksums up to date
	instance->transform_checksum_curr = new_checksum;

	if (!instance->on_interpolate_transform_list) {
		_interpolation_data.instance_transform_update_list_curr->push_back(p_instance);
		instance->on_interpolate_transform_list = true;
	} else {
		DEV_ASSERT(_interpolation_data.instance_transform_update_list_curr->size());
	}

	// If the instance is invisible, then we are simply updating the data flow, there is no need to calculate the interpolated
	// transform or anything else.
	// Ideally we would not even call the RenderingServer::set_transform() when invisible but that would entail having logic
	// to keep track of the previous transform on the SceneTree side. The "early out" below is less efficient but a lot cleaner codewise.
	if (!instance->visible) {
		return;
	}

	// decide on the interpolation method .. slerp if possible
	instance->interpolation_method = TransformInterpolator::find_method(instance->transform_prev.basis, instance->transform_curr.basis);

	if (!instance->on_interpolate_list) {
		_interpolation_data.instance_interpolate_update_list.push_back(p_instance);
		instance->on_interpolate_list = true;
	} else {
		DEV_ASSERT(_interpolation_data.instance_interpolate_update_list.size());
	}

	_instance_queue_update(instance, true);
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

void RenderingServerScene::instance_attach_object_instance_id(RID p_instance, ObjectID p_id) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	instance->object_id = p_id;
}
void RenderingServerScene::instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->update_item.in_list()) {
		_update_dirty_instance(instance);
	}

	ERR_FAIL_INDEX(p_shape, instance->blend_values.size());
	instance->blend_values.write().ptr()[p_shape] = p_weight;
	RSG::storage->mesh_set_blend_shape_values(instance->base, instance->blend_values);
}

void RenderingServerScene::instance_set_surface_material(RID p_instance, int p_surface, RID p_material) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->base_type == RS::INSTANCE_MESH) {
		//may not have been updated yet
		instance->materials.resize(RSG::storage->mesh_get_surface_count(instance->base));
	}

	ERR_FAIL_INDEX(p_surface, instance->materials.size());

	if (instance->materials[p_surface].is_valid()) {
		RSG::storage->material_remove_instance_owner(instance->materials[p_surface], instance);
	}
	instance->materials.write[p_surface] = p_material;
	instance->base_changed(false, true);

	if (instance->materials[p_surface].is_valid()) {
		RSG::storage->material_add_instance_owner(instance->materials[p_surface], instance);
	}
}

void RenderingServerScene::instance_set_visible(RID p_instance, bool p_visible) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->visible == p_visible) {
		return;
	}

	instance->visible = p_visible;

	// Special case for physics interpolation, we want to ensure the interpolated data is up to date
	if (_interpolation_data.interpolation_enabled && p_visible && instance->interpolated && instance->scenario && !instance->on_interpolate_list) {
		// Do all the extra work we normally do on instance_set_transform(), because this is optimized out for hidden instances.
		// This prevents a glitch of stale interpolation transform data when unhiding before the next physics tick.
		instance->interpolation_method = TransformInterpolator::find_method(instance->transform_prev.basis, instance->transform_curr.basis);
		_interpolation_data.instance_interpolate_update_list.push_back(p_instance);
		instance->on_interpolate_list = true;
		_instance_queue_update(instance, true);

		// We must also place on the transform update list for a tick, so the system
		// can auto-detect if the instance is no longer moving, and remove from the interpolate lists again.
		// If this step is ignored, an unmoving instance could remain on the interpolate lists indefinitely
		// (or rather until the object is deleted) and cause unnecessary updates and drawcalls.
		if (!instance->on_interpolate_transform_list) {
			_interpolation_data.instance_transform_update_list_curr->push_back(p_instance);
			instance->on_interpolate_transform_list = true;
		}
	}

	// give the opportunity for the spatial partitioning scene to use a special implementation of visibility
	// for efficiency (supported in BVH but not octree)

	// slightly bug prone optimization here - we want to avoid doing a collision check twice
	// once when activating, and once when calling set_pairable. We do this by deferring the collision check.
	// However, in some cases (notably meshes), set_pairable never gets called. So we want to catch this case
	// and force a collision check (see later in this function).
	// This is only done in two stages to maintain compatibility with the octree.
	if (instance->spatial_partition_id && instance->scenario) {
		if (p_visible) {
			instance->scenario->sps->activate(instance->spatial_partition_id, instance->transformed_aabb);
		} else {
			instance->scenario->sps->deactivate(instance->spatial_partition_id);
		}
	}

	// when showing or hiding geometry, lights must be kept up to date to show / hide shadows
	if ((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(instance->base_data);

		if (geom->can_cast_shadows) {
			for (List<Instance *>::Element *E = geom->lighting.front(); E; E = E->next()) {
				InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
				light->shadow_dirty = true;
			}
		}
	}

	switch (instance->base_type) {
		case RS::INSTANCE_LIGHT: {
			if (RSG::storage->light_get_type(instance->base) != RS::LIGHT_DIRECTIONAL && instance->spatial_partition_id && instance->scenario) {
				instance->scenario->sps->set_pairable(instance, p_visible, 1 << RS::INSTANCE_LIGHT, p_visible ? RS::INSTANCE_GEOMETRY_MASK : 0);
			}

		} break;
		default: {
			// if we haven't called set_pairable, we STILL need to do a collision check
			// for activated items because we deferred it earlier in the call to activate.
			if (instance->spatial_partition_id && instance->scenario && p_visible) {
				instance->scenario->sps->force_collision_check(instance->spatial_partition_id);
			}
		}
	}
}
inline bool is_geometry_instance(RenderingServer::InstanceType p_type) {
	return p_type == RS::INSTANCE_MESH || p_type == RS::INSTANCE_MULTIMESH;
}

void RenderingServerScene::instance_set_custom_aabb(RID p_instance, AABB p_aabb) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);
	ERR_FAIL_COND(!is_geometry_instance(instance->base_type));

	if (p_aabb != AABB()) {
		// Set custom AABB
		if (instance->custom_aabb == nullptr) {
			instance->custom_aabb = memnew(AABB);
		}
		*instance->custom_aabb = p_aabb;

	} else {
		// Clear custom AABB
		if (instance->custom_aabb != nullptr) {
			memdelete(instance->custom_aabb);
			instance->custom_aabb = nullptr;
		}
	}

	if (instance->scenario) {
		_instance_queue_update(instance, true, false);
	}
}

void RenderingServerScene::instance_attach_skeleton(RID p_instance, RID p_skeleton) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->skeleton == p_skeleton) {
		return;
	}

	if (instance->skeleton.is_valid()) {
		RSG::storage->instance_remove_skeleton(instance->skeleton, instance);
	}

	instance->skeleton = p_skeleton;

	if (instance->skeleton.is_valid()) {
		RSG::storage->instance_add_skeleton(instance->skeleton, instance);
	}

	_instance_queue_update(instance, true);
}

void RenderingServerScene::instance_set_exterior(RID p_instance, bool p_enabled) {
}

void RenderingServerScene::instance_set_extra_visibility_margin(RID p_instance, real_t p_margin) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	instance->extra_margin = p_margin;
	_instance_queue_update(instance, true, false);
}

// Rooms
void RenderingServerScene::callbacks_register(RenderingServerCallbacks *p_callbacks) {
	_rendering_server_callbacks = p_callbacks;
}

Vector<ObjectID> RenderingServerScene::instances_cull_aabb(const AABB &p_aabb, RID p_scenario) const {
	Vector<ObjectID> instances;
	Scenario *scenario = scenario_owner.get(p_scenario);
	ERR_FAIL_COND_V(!scenario, instances);

	const_cast<RenderingServerScene *>(this)->update_dirty_instances(); // check dirty instances before culling

	int culled = 0;
	Instance *cull[1024];
	culled = scenario->sps->cull_aabb(p_aabb, cull, 1024);

	for (int i = 0; i < culled; i++) {
		Instance *instance = cull[i];
		ERR_CONTINUE(!instance);
		if (instance->object_id == 0) {
			continue;
		}

		instances.push_back(instance->object_id);
	}

	return instances;
}
Vector<ObjectID> RenderingServerScene::instances_cull_ray(const Vector3 &p_from, const Vector3 &p_to, RID p_scenario) const {
	Vector<ObjectID> instances;
	Scenario *scenario = scenario_owner.get(p_scenario);
	ERR_FAIL_COND_V(!scenario, instances);
	const_cast<RenderingServerScene *>(this)->update_dirty_instances(); // check dirty instances before culling

	int culled = 0;
	Instance *cull[1024];
	culled = scenario->sps->cull_segment(p_from, p_from + p_to * 10000, cull, 1024);

	for (int i = 0; i < culled; i++) {
		Instance *instance = cull[i];
		ERR_CONTINUE(!instance);
		if (instance->object_id == 0) {
			continue;
		}

		instances.push_back(instance->object_id);
	}

	return instances;
}
Vector<ObjectID> RenderingServerScene::instances_cull_convex(const Vector<Plane> &p_convex, RID p_scenario) const {
	Vector<ObjectID> instances;
	Scenario *scenario = scenario_owner.get(p_scenario);
	ERR_FAIL_COND_V(!scenario, instances);
	const_cast<RenderingServerScene *>(this)->update_dirty_instances(); // check dirty instances before culling

	int culled = 0;
	Instance *cull[1024];

	culled = scenario->sps->cull_convex(p_convex, cull, 1024);

	for (int i = 0; i < culled; i++) {
		Instance *instance = cull[i];
		ERR_CONTINUE(!instance);
		if (instance->object_id == 0) {
			continue;
		}

		instances.push_back(instance->object_id);
	}

	return instances;
}

void RenderingServerScene::instance_geometry_set_flag(RID p_instance, RS::InstanceFlags p_flags, bool p_enabled) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	switch (p_flags) {
		case RS::INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE: {
			instance->redraw_if_visible = p_enabled;

		} break;
		default: {
		}
	}
}
void RenderingServerScene::instance_geometry_set_cast_shadows_setting(RID p_instance, RS::ShadowCastingSetting p_shadow_casting_setting) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	instance->cast_shadows = p_shadow_casting_setting;
	instance->base_changed(false, true); // to actually compute if shadows are visible or not
}
void RenderingServerScene::instance_geometry_set_material_override(RID p_instance, RID p_material) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->material_override.is_valid()) {
		RSG::storage->material_remove_instance_owner(instance->material_override, instance);
	}
	instance->material_override = p_material;
	instance->base_changed(false, true);

	if (instance->material_override.is_valid()) {
		RSG::storage->material_add_instance_owner(instance->material_override, instance);
	}
}
void RenderingServerScene::instance_geometry_set_material_overlay(RID p_instance, RID p_material) {
	Instance *instance = instance_owner.get(p_instance);
	ERR_FAIL_COND(!instance);

	if (instance->material_overlay.is_valid()) {
		RSG::storage->material_remove_instance_owner(instance->material_overlay, instance);
	}
	instance->material_overlay = p_material;
	instance->base_changed(false, true);

	if (instance->material_overlay.is_valid()) {
		RSG::storage->material_add_instance_owner(instance->material_overlay, instance);
	}
}

void RenderingServerScene::instance_geometry_set_draw_range(RID p_instance, float p_min, float p_max, float p_min_margin, float p_max_margin) {
}
void RenderingServerScene::instance_geometry_set_as_instance_lod(RID p_instance, RID p_as_lod_of_instance) {
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

	if (p_instance->base_type == RS::INSTANCE_LIGHT) {
		InstanceLightData *light = static_cast<InstanceLightData *>(p_instance->base_data);

		RSG::scene_render->light_instance_set_transform(light->instance, *instance_xform);
		light->shadow_dirty = true;
	}

	if (p_instance->aabb.has_no_surface()) {
		return;
	}

	if ((1 << p_instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) {
		InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(p_instance->base_data);
		//make sure lights are updated if it casts shadow

		if (geom->can_cast_shadows) {
			for (List<Instance *>::Element *E = geom->lighting.front(); E; E = E->next()) {
				InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
				light->shadow_dirty = true;
			}
		}
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

		if (p_instance->base_type == RS::INSTANCE_LIGHT) {
			pairable_mask = p_instance->visible ? RS::INSTANCE_GEOMETRY_MASK : 0;
			pairable = true;
		}

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
		case RenderingServer::INSTANCE_LIGHT: {
			new_aabb = RSG::storage->light_get_aabb(p_instance->base);

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

			bool can_cast_shadows = true;
			bool is_animated = false;

			if (p_instance->cast_shadows == RS::SHADOW_CASTING_SETTING_OFF) {
				can_cast_shadows = false;
			} else if (p_instance->material_override.is_valid()) {
				can_cast_shadows = RSG::storage->material_casts_shadows(p_instance->material_override);
				is_animated = RSG::storage->material_is_animated(p_instance->material_override);
			} else {
				if (p_instance->base_type == RS::INSTANCE_MESH) {
					RID mesh = p_instance->base;

					if (mesh.is_valid()) {
						bool cast_shadows = false;

						for (int i = 0; i < p_instance->materials.size(); i++) {
							RID mat = p_instance->materials[i].is_valid() ? p_instance->materials[i] : RSG::storage->mesh_surface_get_material(mesh, i);

							if (!mat.is_valid()) {
								cast_shadows = true;
							} else {
								if (RSG::storage->material_casts_shadows(mat)) {
									cast_shadows = true;
								}

								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}
							}
						}

						if (!cast_shadows) {
							can_cast_shadows = false;
						}
					}

				} else if (p_instance->base_type == RS::INSTANCE_MULTIMESH) {
					RID mesh = RSG::storage->multimesh_get_mesh(p_instance->base);
					if (mesh.is_valid()) {
						bool cast_shadows = false;

						int sc = RSG::storage->mesh_get_surface_count(mesh);
						for (int i = 0; i < sc; i++) {
							RID mat = RSG::storage->mesh_surface_get_material(mesh, i);

							if (!mat.is_valid()) {
								cast_shadows = true;

							} else {
								if (RSG::storage->material_casts_shadows(mat)) {
									cast_shadows = true;
								}
								if (RSG::storage->material_is_animated(mat)) {
									is_animated = true;
								}
							}
						}

						if (!cast_shadows) {
							can_cast_shadows = false;
						}
					}
				}
			}

			if (p_instance->material_overlay.is_valid()) {
				can_cast_shadows = can_cast_shadows || RSG::storage->material_casts_shadows(p_instance->material_overlay);
				is_animated = is_animated || RSG::storage->material_is_animated(p_instance->material_overlay);
			}

			if (can_cast_shadows != geom->can_cast_shadows) {
				//ability to cast shadows change, let lights now
				for (List<Instance *>::Element *E = geom->lighting.front(); E; E = E->next()) {
					InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);
					light->shadow_dirty = true;
				}

				geom->can_cast_shadows = can_cast_shadows;
			}

			geom->material_is_animated = is_animated;
		}
	}

	_instance_update_list.remove(&p_instance->update_item);

	_update_instance(p_instance);

	p_instance->update_aabb = false;
	p_instance->update_materials = false;
}

bool RenderingServerScene::_light_instance_update_shadow(Instance *p_instance, const Transform p_cam_transform, const Projection &p_cam_projection, bool p_cam_orthogonal, RID p_shadow_atlas, Scenario *p_scenario, uint32_t p_visible_layers) {
	InstanceLightData *light = static_cast<InstanceLightData *>(p_instance->base_data);

	Transform light_transform = p_instance->transform;
	light_transform.orthonormalize(); //scale does not count on lights

	bool animated_material_found = false;

	switch (RSG::storage->light_get_type(p_instance->base)) {
		case RS::LIGHT_DIRECTIONAL: {
			float max_distance = p_cam_projection.get_z_far();
			float shadow_max = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_SHADOW_MAX_DISTANCE);
			if (shadow_max > 0 && !p_cam_orthogonal) { //its impractical (and leads to unwanted behaviors) to set max distance in orthogonal camera
				max_distance = MIN(shadow_max, max_distance);
			}
			max_distance = MAX(max_distance, p_cam_projection.get_z_near() + 0.001);
			float min_distance = MIN(p_cam_projection.get_z_near(), max_distance);

			RS::LightDirectionalShadowDepthRangeMode depth_range_mode = RSG::storage->light_directional_get_shadow_depth_range_mode(p_instance->base);

			if (depth_range_mode == RS::LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_OPTIMIZED) {
				//optimize min/max
				Vector<Plane> planes = p_cam_projection.get_projection_planes(p_cam_transform);
				int cull_count = p_scenario->sps->cull_convex(planes, instance_shadow_cull_result, MAX_INSTANCE_CULL, RS::INSTANCE_GEOMETRY_MASK);
				Plane base(p_cam_transform.origin, -p_cam_transform.basis.get_axis(2));
				//check distance max and min

				bool found_items = false;
				float z_max = -1e20;
				float z_min = 1e20;

				for (int i = 0; i < cull_count; i++) {
					Instance *instance = instance_shadow_cull_result[i];
					if (!instance->visible || !((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) || !static_cast<InstanceGeometryData *>(instance->base_data)->can_cast_shadows || !(p_visible_layers & instance->layer_mask)) {
						continue;
					}

					if (static_cast<InstanceGeometryData *>(instance->base_data)->material_is_animated) {
						animated_material_found = true;
					}

					float max, min;
					instance->transformed_aabb.project_range_in_plane(base, min, max);

					if (max > z_max) {
						z_max = max;
					}

					if (min < z_min) {
						z_min = min;
					}

					found_items = true;
				}

				if (found_items) {
					min_distance = MAX(min_distance, z_min);
					max_distance = MIN(max_distance, z_max);
				}
			}

			float range = max_distance - min_distance;

			int splits = 0;
			switch (RSG::storage->light_directional_get_shadow_mode(p_instance->base)) {
				case RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL:
					splits = 1;
					break;
				case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS:
					splits = 2;
					break;
				case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_3_SPLITS:
					splits = 3;
					break;
				case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS:
					splits = 4;
					break;
			}

			float distances[5];

			distances[0] = min_distance;
			for (int i = 0; i < splits; i++) {
				distances[i + 1] = min_distance + RSG::storage->light_get_param(p_instance->base, RS::LightParam(RS::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET + i)) * range;
			};

			distances[splits] = max_distance;

			float texture_size = RSG::scene_render->get_directional_light_shadow_size(light->instance);

			bool overlap = RSG::storage->light_directional_get_blend_splits(p_instance->base);

			float first_radius = 0.0;

			for (int i = 0; i < splits; i++) {
				// setup a camera matrix for that range!
				Projection camera_matrix;

				float aspect = p_cam_projection.get_aspect();

				if (p_cam_orthogonal) {
					Vector2 vp_he = p_cam_projection.get_viewport_half_extents();

					camera_matrix.set_orthogonal(vp_he.y * 2.0, aspect, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], false);
				} else {
					float fov = p_cam_projection.get_fov();
					camera_matrix.set_perspective(fov, aspect, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], false);
				}

				//obtain the frustum endpoints

				Vector3 endpoints[8]; // frustum plane endpoints
				bool res = camera_matrix.get_endpoints(p_cam_transform, endpoints);
				ERR_CONTINUE(!res);

				// obtain the light frustm ranges (given endpoints)

				Transform transform = light_transform; //discard scale and stabilize light

				Vector3 x_vec = transform.basis.get_axis(Vector3::AXIS_X).normalized();
				Vector3 y_vec = transform.basis.get_axis(Vector3::AXIS_Y).normalized();
				Vector3 z_vec = transform.basis.get_axis(Vector3::AXIS_Z).normalized();
				//z_vec points agsint the camera, like in default opengl

				float x_min = 0.f, x_max = 0.f;
				float y_min = 0.f, y_max = 0.f;
				float z_min = 0.f, z_max = 0.f;

				// FIXME: z_max_cam is defined, computed, but not used below when setting up
				// ortho_camera. Commented out for now to fix warnings but should be investigated.
				float x_min_cam = 0.f, x_max_cam = 0.f;
				float y_min_cam = 0.f, y_max_cam = 0.f;
				float z_min_cam = 0.f;
				//float z_max_cam = 0.f;

				float bias_scale = 1.0;

				//used for culling

				for (int j = 0; j < 8; j++) {
					float d_x = x_vec.dot(endpoints[j]);
					float d_y = y_vec.dot(endpoints[j]);
					float d_z = z_vec.dot(endpoints[j]);

					if (j == 0 || d_x < x_min) {
						x_min = d_x;
					}
					if (j == 0 || d_x > x_max) {
						x_max = d_x;
					}

					if (j == 0 || d_y < y_min) {
						y_min = d_y;
					}
					if (j == 0 || d_y > y_max) {
						y_max = d_y;
					}

					if (j == 0 || d_z < z_min) {
						z_min = d_z;
					}
					if (j == 0 || d_z > z_max) {
						z_max = d_z;
					}
				}

				{
					//camera viewport stuff

					Vector3 center;

					for (int j = 0; j < 8; j++) {
						center += endpoints[j];
					}
					center /= 8.0;

					//center=x_vec*(x_max-x_min)*0.5 + y_vec*(y_max-y_min)*0.5 + z_vec*(z_max-z_min)*0.5;

					float radius = 0;

					for (int j = 0; j < 8; j++) {
						float d = center.distance_to(endpoints[j]);
						if (d > radius) {
							radius = d;
						}
					}

					radius *= texture_size / (texture_size - 2.0); //add a texel by each side

					if (i == 0) {
						first_radius = radius;
					} else {
						bias_scale = radius / first_radius;
					}

					x_max_cam = x_vec.dot(center) + radius;
					x_min_cam = x_vec.dot(center) - radius;
					y_max_cam = y_vec.dot(center) + radius;
					y_min_cam = y_vec.dot(center) - radius;
					//z_max_cam = z_vec.dot(center) + radius;
					z_min_cam = z_vec.dot(center) - radius;

					if (depth_range_mode == RS::LIGHT_DIRECTIONAL_SHADOW_DEPTH_RANGE_STABLE) {
						//this trick here is what stabilizes the shadow (make potential jaggies to not move)
						//at the cost of some wasted resolution. Still the quality increase is very well worth it

						float unit = radius * 2.0 / texture_size;

						x_max_cam = Math::stepify(x_max_cam, unit);
						x_min_cam = Math::stepify(x_min_cam, unit);
						y_max_cam = Math::stepify(y_max_cam, unit);
						y_min_cam = Math::stepify(y_min_cam, unit);
					}
				}

				//now that we now all ranges, we can proceed to make the light frustum planes, for culling octree

				Vector<Plane> light_frustum_planes;
				light_frustum_planes.resize(6);

				//right/left
				light_frustum_planes.write[0] = Plane(x_vec, x_max);
				light_frustum_planes.write[1] = Plane(-x_vec, -x_min);
				//top/bottom
				light_frustum_planes.write[2] = Plane(y_vec, y_max);
				light_frustum_planes.write[3] = Plane(-y_vec, -y_min);
				//near/far
				light_frustum_planes.write[4] = Plane(z_vec, z_max + 1e6);
				light_frustum_planes.write[5] = Plane(-z_vec, -z_min); // z_min is ok, since casters further than far-light plane are not needed

				int cull_count = p_scenario->sps->cull_convex(light_frustum_planes, instance_shadow_cull_result, MAX_INSTANCE_CULL, RS::INSTANCE_GEOMETRY_MASK);

				// a pre pass will need to be needed to determine the actual z-near to be used

				Plane near_plane(light_transform.origin, -light_transform.basis.get_axis(2));

				for (int j = 0; j < cull_count; j++) {
					float min, max;
					Instance *instance = instance_shadow_cull_result[j];
					if (!instance->visible || !((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) || !static_cast<InstanceGeometryData *>(instance->base_data)->can_cast_shadows || !(p_visible_layers & instance->layer_mask)) {
						cull_count--;
						SWAP(instance_shadow_cull_result[j], instance_shadow_cull_result[cull_count]);
						j--;
						continue;
					}

					instance->transformed_aabb.project_range_in_plane(Plane(z_vec, 0), min, max);
					instance->depth = near_plane.distance_to(instance->transform.origin);
					instance->depth_layer = 0;
					if (max > z_max) {
						z_max = max;
					}
				}

				{
					Projection ortho_camera;
					real_t half_x = (x_max_cam - x_min_cam) * 0.5;
					real_t half_y = (y_max_cam - y_min_cam) * 0.5;

					ortho_camera.set_orthogonal(-half_x, half_x, -half_y, half_y, 0, (z_max - z_min_cam));

					Transform ortho_transform;
					ortho_transform.basis = transform.basis;
					ortho_transform.origin = x_vec * (x_min_cam + half_x) + y_vec * (y_min_cam + half_y) + z_vec * z_max;

					RSG::scene_render->light_instance_set_shadow_transform(light->instance, ortho_camera, ortho_transform, 0, distances[i + 1], i, bias_scale);
				}

				RSG::scene_render->render_shadow(light->instance, p_shadow_atlas, i, (RasterizerScene::InstanceBase **)instance_shadow_cull_result, cull_count);
			}

		} break;
		case RS::LIGHT_OMNI: {
			RS::LightOmniShadowMode shadow_mode = RSG::storage->light_omni_get_shadow_mode(p_instance->base);

			if (shadow_mode == RS::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID || !RSG::scene_render->light_instances_can_render_shadow_cube()) {
				for (int i = 0; i < 2; i++) {
					//using this one ensures that raster deferred will have it

					float radius = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_RANGE);

					float z = i == 0 ? -1 : 1;
					Vector<Plane> planes;
					planes.resize(6);
					planes.write[0] = light_transform.xform(Plane(Vector3(0, 0, z), radius));
					planes.write[1] = light_transform.xform(Plane(Vector3(1, 0, z).normalized(), radius));
					planes.write[2] = light_transform.xform(Plane(Vector3(-1, 0, z).normalized(), radius));
					planes.write[3] = light_transform.xform(Plane(Vector3(0, 1, z).normalized(), radius));
					planes.write[4] = light_transform.xform(Plane(Vector3(0, -1, z).normalized(), radius));
					planes.write[5] = light_transform.xform(Plane(Vector3(0, 0, -z), 0));

					int cull_count = p_scenario->sps->cull_convex(planes, instance_shadow_cull_result, MAX_INSTANCE_CULL, RS::INSTANCE_GEOMETRY_MASK);
					Plane near_plane(light_transform.origin, light_transform.basis.get_axis(2) * z);

					for (int j = 0; j < cull_count; j++) {
						Instance *instance = instance_shadow_cull_result[j];
						if (!instance->visible || !((1 << instance->base_type) & RS::INSTANCE_GEOMETRY_MASK) || !static_cast<InstanceGeometryData *>(instance->base_data)->can_cast_shadows || !(p_visible_layers & instance->layer_mask)) {
							cull_count--;
							SWAP(instance_shadow_cull_result[j], instance_shadow_cull_result[cull_count]);
							j--;
						} else {
							if (static_cast<InstanceGeometryData *>(instance->base_data)->material_is_animated) {
								animated_material_found = true;
							}

							instance->depth = near_plane.distance_to(instance->transform.origin);
							instance->depth_layer = 0;
						}
					}

					RSG::scene_render->light_instance_set_shadow_transform(light->instance, Projection(), light_transform, radius, 0, i);
					RSG::scene_render->render_shadow(light->instance, p_shadow_atlas, i, (RasterizerScene::InstanceBase **)instance_shadow_cull_result, cull_count);
				}
			} else { //shadow cube

				float radius = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_RANGE);
				Projection cm;
				cm.set_perspective(90, 1, 0.01, radius);

				for (int i = 0; i < 6; i++) {
					//using this one ensures that raster deferred will have it

					static const Vector3 view_normals[6] = {
						Vector3(-1, 0, 0),
						Vector3(+1, 0, 0),
						Vector3(0, -1, 0),
						Vector3(0, +1, 0),
						Vector3(0, 0, -1),
						Vector3(0, 0, +1)
					};
					static const Vector3 view_up[6] = {
						Vector3(0, -1, 0),
						Vector3(0, -1, 0),
						Vector3(0, 0, -1),
						Vector3(0, 0, +1),
						Vector3(0, -1, 0),
						Vector3(0, -1, 0)
					};

					Transform xform = light_transform * Transform().looking_at(view_normals[i], view_up[i]);

					Vector<Plane> planes = cm.get_projection_planes(xform);

					RSG::scene_render->light_instance_set_shadow_transform(light->instance, cm, xform, radius, 0, i);
					RSG::scene_render->render_shadow(light->instance, p_shadow_atlas, i, (RasterizerScene::InstanceBase **)instance_shadow_cull_result, 0);
				}

				//restore the regular DP matrix
				RSG::scene_render->light_instance_set_shadow_transform(light->instance, Projection(), light_transform, radius, 0, 0);
			}

		} break;
		case RS::LIGHT_SPOT: {
			float radius = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_RANGE);
			float angle = RSG::storage->light_get_param(p_instance->base, RS::LIGHT_PARAM_SPOT_ANGLE);

			Projection cm;
			cm.set_perspective(angle * 2.0, 1.0, 0.01, radius);

			Vector<Plane> planes = cm.get_projection_planes(light_transform);

			RSG::scene_render->light_instance_set_shadow_transform(light->instance, cm, light_transform, radius, 0, 0);
			RSG::scene_render->render_shadow(light->instance, p_shadow_atlas, 0, (RasterizerScene::InstanceBase **)instance_shadow_cull_result, 0);

		} break;
	}

	return animated_material_found;
}

void RenderingServerScene::render_camera(RID p_camera, RID p_scenario, Size2 p_viewport_size, RID p_shadow_atlas) {
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

	_prepare_scene(camera_transform, camera_matrix, ortho, camera->env, camera->visible_layers, p_scenario, p_shadow_atlas, RID(), camera->previous_room_id_hint);
	_render_scene(camera_transform, camera_matrix, 0, ortho, camera->env, p_scenario, p_shadow_atlas);
#endif
}

void RenderingServerScene::_prepare_scene(const Transform p_cam_transform, const Projection &p_cam_projection, bool p_cam_orthogonal, RID p_force_environment, uint32_t p_visible_layers, RID p_scenario, RID p_shadow_atlas, RID p_reflection_probe, int32_t &r_previous_room_id_hint) {
	// Note, in stereo rendering:
	// - p_cam_transform will be a transform in the middle of our two eyes
	// - p_cam_projection is a wider frustrum that encompasses both eyes

	Scenario *scenario = scenario_owner.getornull(p_scenario);

	render_pass++;
	uint32_t camera_layer_mask = p_visible_layers;

	RSG::scene_render->set_scene_pass(render_pass);

	//rasterizer->set_camera(camera->transform, camera_matrix,ortho);

	Vector<Plane> planes = p_cam_projection.get_projection_planes(p_cam_transform);

	Plane near_plane(p_cam_transform.origin, -p_cam_transform.basis.get_axis(2).normalized());
	float z_far = p_cam_projection.get_z_far();

	/* STEP 2 - CULL */
	instance_cull_count = 0;
	light_cull_count = 0;

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
		} else if (ins->base_type == RS::INSTANCE_LIGHT && ins->visible) {
			if (light_cull_count < MAX_LIGHTS_CULLED) {
				InstanceLightData *light = static_cast<InstanceLightData *>(ins->base_data);

				if (!light->geometries.empty()) {
					//do not add this light if no geometry is affected by it..
					light_cull_result[light_cull_count] = ins;
					light_instance_cull_result[light_cull_count] = light->instance;
					if (p_shadow_atlas.is_valid() && RSG::storage->light_has_shadow(ins->base)) {
						RSG::scene_render->light_instance_mark_visible(light->instance); //mark it visible for shadow allocation later
					}

					light_cull_count++;
				}
			}
		} else if (((1 << ins->base_type) & RS::INSTANCE_GEOMETRY_MASK) && ins->visible && ins->cast_shadows != RS::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			keep = true;

			InstanceGeometryData *geom = static_cast<InstanceGeometryData *>(ins->base_data);

			if (ins->redraw_if_visible) {
				RenderingServerRaster::redraw_request(false);
			}

			if (geom->lighting_dirty) {
				int l = 0;
				//only called when lights AABB enter/exit this geometry
				ins->light_instances.resize(geom->lighting.size());

				for (List<Instance *>::Element *E = geom->lighting.front(); E; E = E->next()) {
					InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);

					ins->light_instances.write[l++] = light->instance;
				}

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

	RID *directional_light_ptr = &light_instance_cull_result[light_cull_count];
	directional_light_count = 0;

	// directional lights
	{
		Instance **lights_with_shadow = (Instance **)alloca(sizeof(Instance *) * scenario->directional_lights.size());
		int directional_shadow_count = 0;

		for (List<Instance *>::Element *E = scenario->directional_lights.front(); E; E = E->next()) {
			if (light_cull_count + directional_light_count >= MAX_LIGHTS_CULLED) {
				break;
			}

			if (!E->get()->visible) {
				continue;
			}

			InstanceLightData *light = static_cast<InstanceLightData *>(E->get()->base_data);

			//check shadow..

			if (light) {
				if (p_shadow_atlas.is_valid() && RSG::storage->light_has_shadow(E->get()->base)) {
					lights_with_shadow[directional_shadow_count++] = E->get();
				}
				//add to list
				directional_light_ptr[directional_light_count++] = light->instance;
			}
		}

		RSG::scene_render->set_directional_shadow_count(directional_shadow_count);

		for (int i = 0; i < directional_shadow_count; i++) {
			_light_instance_update_shadow(lights_with_shadow[i], p_cam_transform, p_cam_projection, p_cam_orthogonal, p_shadow_atlas, scenario, p_visible_layers);
		}
	}

	{ //setup shadow maps

		//SortArray<Instance*,_InstanceLightsort> sorter;
		//sorter.sort(light_cull_result,light_cull_count);
		for (int i = 0; i < light_cull_count; i++) {
			Instance *ins = light_cull_result[i];

			if (!p_shadow_atlas.is_valid() || !RSG::storage->light_has_shadow(ins->base)) {
				continue;
			}

			InstanceLightData *light = static_cast<InstanceLightData *>(ins->base_data);

			float coverage = 0.f;

			{ //compute coverage

				Transform cam_xf = p_cam_transform;
				float zn = p_cam_projection.get_z_near();
				Plane p(cam_xf.origin + cam_xf.basis.get_axis(2) * -zn, -cam_xf.basis.get_axis(2)); //camera near plane

				// near plane half width and height
				Vector2 vp_half_extents = p_cam_projection.get_viewport_half_extents();

				switch (RSG::storage->light_get_type(ins->base)) {
					case RS::LIGHT_OMNI: {
						float radius = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_RANGE);

						//get two points parallel to near plane
						Vector3 points[2] = {
							ins->transform.origin,
							ins->transform.origin + cam_xf.basis.get_axis(0) * radius
						};

						if (!p_cam_orthogonal) {
							//if using perspetive, map them to near plane
							for (int j = 0; j < 2; j++) {
								if (p.distance_to(points[j]) < 0) {
									points[j].z = -zn; //small hack to keep size constant when hitting the screen
								}

								p.intersects_segment(cam_xf.origin, points[j], &points[j]); //map to plane
							}
						}

						float screen_diameter = points[0].distance_to(points[1]) * 2;
						coverage = screen_diameter / (vp_half_extents.x + vp_half_extents.y);
					} break;
					case RS::LIGHT_SPOT: {
						float radius = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_RANGE);
						float angle = RSG::storage->light_get_param(ins->base, RS::LIGHT_PARAM_SPOT_ANGLE);

						float w = radius * Math::sin(Math::deg2rad(angle));
						float d = radius * Math::cos(Math::deg2rad(angle));

						Vector3 base = ins->transform.origin - ins->transform.basis.get_axis(2).normalized() * d;

						Vector3 points[2] = {
							base,
							base + cam_xf.basis.get_axis(0) * w
						};

						if (!p_cam_orthogonal) {
							//if using perspetive, map them to near plane
							for (int j = 0; j < 2; j++) {
								if (p.distance_to(points[j]) < 0) {
									points[j].z = -zn; //small hack to keep size constant when hitting the screen
								}

								p.intersects_segment(cam_xf.origin, points[j], &points[j]); //map to plane
							}
						}

						float screen_diameter = points[0].distance_to(points[1]) * 2;
						coverage = screen_diameter / (vp_half_extents.x + vp_half_extents.y);

					} break;
					default: {
						ERR_PRINT("Invalid Light Type");
					}
				}
			}

			if (light->shadow_dirty) {
				light->last_version++;
				light->shadow_dirty = false;
			}

			bool redraw = RSG::scene_render->shadow_atlas_update_light(p_shadow_atlas, light->instance, coverage, light->last_version);

			if (redraw) {
				//must redraw!
				light->shadow_dirty = _light_instance_update_shadow(ins, p_cam_transform, p_cam_projection, p_cam_orthogonal, p_shadow_atlas, scenario, p_visible_layers);
			}
		}
	}

	// Calculate instance->depth from the camera, after shadow calculation has stopped overwriting instance->depth
	for (int i = 0; i < instance_cull_count; i++) {
		Instance *ins = instance_cull_result[i];

		if (((1 << ins->base_type) & RS::INSTANCE_GEOMETRY_MASK) && ins->visible && ins->cast_shadows != RS::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
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

void RenderingServerScene::_render_scene(const Transform p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_orthogonal, RID p_force_environment, RID p_scenario, RID p_shadow_atlas) {
	Scenario *scenario = scenario_owner.getornull(p_scenario);

	/* ENVIRONMENT */

	RID environment;
	if (p_force_environment.is_valid()) { //camera has more environment priority
		environment = p_force_environment;
	} else if (scenario->environment.is_valid()) {
		environment = scenario->environment;
	} else {
		environment = scenario->fallback_environment;
	}

	/* PROCESS GEOMETRY AND DRAW SCENE */

	RSG::scene_render->render_scene(p_cam_transform, p_cam_projection, p_eye, p_cam_orthogonal, (RasterizerScene::InstanceBase **)instance_cull_result, instance_cull_count, light_instance_cull_result, light_cull_count + directional_light_count, p_shadow_atlas);
}

void RenderingServerScene::render_empty_scene(RID p_scenario, RID p_shadow_atlas) {
#ifndef _3D_DISABLED

	Scenario *scenario = scenario_owner.getornull(p_scenario);

	RID environment;
	if (scenario->environment.is_valid()) {
		environment = scenario->environment;
	} else {
		environment = scenario->fallback_environment;
	}
	RSG::scene_render->render_scene(Transform(), Projection(), 0, true, nullptr, 0, nullptr, 0, p_shadow_atlas);
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

		while (scenario->instances.first()) {
			instance_set_scenario(scenario->instances.first()->self()->self, RID());
		}
		RSG::scene_render->free(scenario->shadow_atlas);
		scenario_owner.free(p_rid);
		memdelete(scenario);

	} else if (instance_owner.owns(p_rid)) {
		// delete the instance

		update_dirty_instances();

		Instance *instance = instance_owner.get(p_rid);

		_interpolation_data.notify_free_instance(p_rid, *instance);

		instance_set_scenario(p_rid, RID());
		instance_set_base(p_rid, RID());
		instance_geometry_set_material_override(p_rid, RID());
		instance_geometry_set_material_overlay(p_rid, RID());
		instance_attach_skeleton(p_rid, RID());

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

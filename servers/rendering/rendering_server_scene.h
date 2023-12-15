#ifndef RENDERINGSERVERSCENE_H
#define RENDERINGSERVERSCENE_H
/*************************************************************************/
/*  rendering_server_scene.h                                                */
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

#include "servers/rendering/rasterizer.h"

#include "core/containers/self_list.h"
#include "core/math/bvh.h"
#include "core/math/geometry.h"
#include "core/math/octree.h"
#include "core/os/safe_refcount.h"
#include "core/os/semaphore.h"
#include "core/os/thread.h"

class RenderingServerScene {
public:
	enum {

		MAX_INSTANCE_CULL = 65536,
		MAX_LIGHTS_CULLED = 4096,
		MAX_REFLECTION_PROBES_CULLED = 4096,
	};

	uint64_t render_pass;
	static RenderingServerScene *singleton;

	/* EVENT QUEUING */

	void tick();
	void pre_draw(bool p_will_draw);

	/* CAMERA API */

	struct Scenario;

	struct Camera : public RID_Data {
		enum Type {
			PERSPECTIVE,
			ORTHOGONAL,
			FRUSTUM
		};
		Type type;
		float fov;
		float znear, zfar;
		float size;
		Vector2 offset;
		uint32_t visible_layers;
		RID env;

		// transform_prev is only used when using fixed timestep interpolation
		Transform transform;
		Transform transform_prev;

		bool interpolated : 1;
		bool on_interpolate_transform_list : 1;

		bool vaspect : 1;
		TransformInterpolator::Method interpolation_method : 3;

		int32_t previous_room_id_hint;

		Transform get_transform_interpolated() const;

		Camera() {
			visible_layers = 0xFFFFFFFF;
			fov = 70;
			type = PERSPECTIVE;
			znear = 0.05;
			zfar = 100;
			size = 1.0;
			offset = Vector2();
			vaspect = false;
			previous_room_id_hint = -1;
			interpolated = true;
			on_interpolate_transform_list = false;
			interpolation_method = TransformInterpolator::INTERP_LERP;
		}
	};

	mutable RID_Owner<Camera> camera_owner;

	virtual RID camera_create();
	virtual void camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	virtual void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	virtual void camera_set_frustum(RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	virtual void camera_set_transform(RID p_camera, const Transform &p_transform);
	virtual void camera_set_interpolated(RID p_camera, bool p_interpolated);
	virtual void camera_reset_physics_interpolation(RID p_camera);
	virtual void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	virtual void camera_set_environment(RID p_camera, RID p_env);
	virtual void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);

	/* SCENARIO API */

	struct Instance;

	// common interface for all spatial partitioning schemes
	// this is a bit excessive boilerplatewise but can be removed if we decide to stick with one method

	// note this is actually the BVH id +1, so that visual server can test against zero
	// for validity to maintain compatibility with octree (where 0 indicates invalid)
	typedef uint32_t SpatialPartitionID;

	class SpatialPartitioningScene {
	public:
		virtual SpatialPartitionID create(Instance *p_userdata, const AABB &p_aabb, int p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t pairable_mask) = 0;
		virtual void erase(SpatialPartitionID p_handle) = 0;
		virtual void move(SpatialPartitionID p_handle, const AABB &p_aabb) = 0;
		virtual void activate(SpatialPartitionID p_handle, const AABB &p_aabb) {}
		virtual void deactivate(SpatialPartitionID p_handle) {}
		virtual void force_collision_check(SpatialPartitionID p_handle) {}
		virtual void update() {}
		virtual void update_collisions() {}
		virtual void set_pairable(Instance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask) = 0;
		virtual int cull_convex(const Vector<Plane> &p_convex, Instance **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF) = 0;
		virtual int cull_aabb(const AABB &p_aabb, Instance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF) = 0;
		virtual int cull_segment(const Vector3 &p_from, const Vector3 &p_to, Instance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF) = 0;

		typedef void *(*PairCallback)(void *, uint32_t, Instance *, int, uint32_t, Instance *, int);
		typedef void (*UnpairCallback)(void *, uint32_t, Instance *, int, uint32_t, Instance *, int, void *);

		virtual void set_pair_callback(PairCallback p_callback, void *p_userdata) = 0;
		virtual void set_unpair_callback(UnpairCallback p_callback, void *p_userdata) = 0;

		// bvh specific
		virtual void params_set_node_expansion(real_t p_value) {}
		virtual void params_set_pairing_expansion(real_t p_value) {}

		// octree specific
		virtual void set_balance(float p_balance) {}

		virtual ~SpatialPartitioningScene() {}
	};

	class SpatialPartitioningScene_Octree : public SpatialPartitioningScene {
		Octree_CL<Instance, true> _octree;

	public:
		SpatialPartitionID create(Instance *p_userdata, const AABB &p_aabb, int p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t pairable_mask);
		void erase(SpatialPartitionID p_handle);
		void move(SpatialPartitionID p_handle, const AABB &p_aabb);
		void set_pairable(Instance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask);
		int cull_convex(const Vector<Plane> &p_convex, Instance **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF);
		int cull_aabb(const AABB &p_aabb, Instance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
		int cull_segment(const Vector3 &p_from, const Vector3 &p_to, Instance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
		void set_pair_callback(PairCallback p_callback, void *p_userdata);
		void set_unpair_callback(UnpairCallback p_callback, void *p_userdata);
		void set_balance(float p_balance);
	};

	class SpatialPartitioningScene_BVH : public SpatialPartitioningScene {
		template <class T>
		class UserPairTestFunction {
		public:
			static bool user_pair_check(const T *p_a, const T *p_b) {
				// return false if no collision, decided by masks etc
				return true;
			}
		};

		template <class T>
		class UserCullTestFunction {
			// write this logic once for use in all routines
			// double check this as a possible source of bugs in future.
			static bool _cull_pairing_mask_test_hit(uint32_t p_maskA, uint32_t p_typeA, uint32_t p_maskB, uint32_t p_typeB) {
				// double check this as a possible source of bugs in future.
				bool A_match_B = p_maskA & p_typeB;

				if (!A_match_B) {
					bool B_match_A = p_maskB & p_typeA;
					if (!B_match_A) {
						return false;
					}
				}

				return true;
			}

		public:
			static bool user_cull_check(const T *p_a, const T *p_b) {
				DEV_ASSERT(p_a);
				DEV_ASSERT(p_b);

				uint32_t a_mask = p_a->bvh_pairable_mask;
				uint32_t a_type = p_a->bvh_pairable_type;
				uint32_t b_mask = p_b->bvh_pairable_mask;
				uint32_t b_type = p_b->bvh_pairable_type;

				if (!_cull_pairing_mask_test_hit(a_mask, a_type, b_mask, b_type)) {
					return false;
				}

				return true;
			}
		};

	private:
		// Note that SpatialPartitionIDs are +1 based when stored in visual server, to enable 0 to indicate invalid ID.
		BVH_Manager<Instance, 2, true, 256, UserPairTestFunction<Instance>, UserCullTestFunction<Instance>> _bvh;
		Instance *_dummy_cull_object;

		uint32_t find_tree_id_and_collision_mask(bool p_pairable, uint32_t &r_tree_collision_mask) const {
			// "pairable" (lights etc) can pair with geometry (non pairable) or other pairables.
			// Geometry never pairs with other geometry, so we can eliminate geometry - geometry collision checks.

			// Additionally, when lights are made invisible their p_pairable_mask is set to zero to stop their collisions.
			// We could potentially choose `tree_collision_mask` based on whether p_pairable_mask is zero,
			// in order to catch invisible lights, but in practice these instances will already have been deactivated within
			// the BVH so this step is unnecessary. So we can keep the simpler logic of geometry collides with pairable,
			// pairable collides with everything.
			r_tree_collision_mask = !p_pairable ? 2 : 3;

			// Returns tree_id.
			return p_pairable ? 1 : 0;
		}

	public:
		SpatialPartitioningScene_BVH();
		~SpatialPartitioningScene_BVH();
		SpatialPartitionID create(Instance *p_userdata, const AABB &p_aabb, int p_subindex, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask);
		void erase(SpatialPartitionID p_handle);
		void move(SpatialPartitionID p_handle, const AABB &p_aabb);
		void activate(SpatialPartitionID p_handle, const AABB &p_aabb);
		void deactivate(SpatialPartitionID p_handle);
		void force_collision_check(SpatialPartitionID p_handle);
		void update();
		void update_collisions();
		void set_pairable(Instance *p_instance, bool p_pairable, uint32_t p_pairable_type, uint32_t p_pairable_mask);
		int cull_convex(const Vector<Plane> &p_convex, Instance **p_result_array, int p_result_max, uint32_t p_mask = 0xFFFFFFFF);
		int cull_aabb(const AABB &p_aabb, Instance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
		int cull_segment(const Vector3 &p_from, const Vector3 &p_to, Instance **p_result_array, int p_result_max, int *p_subindex_array = nullptr, uint32_t p_mask = 0xFFFFFFFF);
		void set_pair_callback(PairCallback p_callback, void *p_userdata);
		void set_unpair_callback(UnpairCallback p_callback, void *p_userdata);

		void params_set_node_expansion(real_t p_value) { _bvh.params_set_node_expansion(p_value); }
		void params_set_pairing_expansion(real_t p_value) { _bvh.params_set_pairing_expansion(p_value); }
	};

	struct Scenario : RID_Data {
		RS::ScenarioDebugMode debug;
		RID self;

		SpatialPartitioningScene *sps;

		SelfList<Instance>::List instances;

		Scenario();
		~Scenario() { memdelete(sps); }
	};

	mutable RID_Owner<Scenario> scenario_owner;

	static void *_instance_pair(void *p_self, SpatialPartitionID, Instance *p_A, int, SpatialPartitionID, Instance *p_B, int);
	static void _instance_unpair(void *p_self, SpatialPartitionID, Instance *p_A, int, SpatialPartitionID, Instance *p_B, int, void *);

	virtual RID scenario_create();

	virtual void scenario_set_debug(RID p_scenario, RS::ScenarioDebugMode p_debug_mode);

	/* INSTANCING API */

	struct InstanceBaseData {
		virtual ~InstanceBaseData() {}
	};

	struct Instance : RasterizerScene::InstanceBase {
		RID self;
		//scenario stuff
		SpatialPartitionID spatial_partition_id;

		Scenario *scenario;
		SelfList<Instance> scenario_item;

		//aabb stuff
		bool update_aabb;
		bool update_materials;

		SelfList<Instance> update_item;

		AABB aabb;
		AABB transformed_aabb;
		AABB *custom_aabb; // <Zylann> would using aabb directly with a bool be better?
		float sorting_offset;
		bool use_aabb_center;
		float extra_margin;
		uint32_t object_id;

		float lod_begin;
		float lod_end;
		float lod_begin_hysteresis;
		float lod_end_hysteresis;
		RID lod_instance;

		// These are used for the user cull testing function
		// in the BVH, this is precached rather than recalculated each time.
		uint32_t bvh_pairable_mask;
		uint32_t bvh_pairable_type;

		uint64_t last_render_pass;
		uint64_t last_frame_pass;

		uint64_t version; // changes to this, and changes to base increase version

		InstanceBaseData *base_data;

		virtual void base_removed() {
		}

		virtual void base_changed(bool p_aabb, bool p_materials) {
			singleton->_instance_queue_update(this, p_aabb, p_materials);
		}

		Instance() :
				scenario_item(this),
				update_item(this) {
			spatial_partition_id = 0;
			scenario = nullptr;

			update_aabb = false;
			update_materials = false;

			extra_margin = 0;

			object_id = 0;
			visible = true;

			lod_begin = 0;
			lod_end = 0;
			lod_begin_hysteresis = 0;
			lod_end_hysteresis = 0;

			bvh_pairable_mask = 0;
			bvh_pairable_type = 0;

			last_render_pass = 0;
			last_frame_pass = 0;
			version = 1;
			base_data = nullptr;

			custom_aabb = nullptr;
			sorting_offset = 0.0f;
			use_aabb_center = true;
		}

		~Instance() {
			if (base_data) {
				memdelete(base_data);
			}
			if (custom_aabb) {
				memdelete(custom_aabb);
			}
		}
	};

	SelfList<Instance>::List _instance_update_list;

	// fixed timestep interpolation
	virtual void set_physics_interpolation_enabled(bool p_enabled);

	struct InterpolationData {
		void notify_free_camera(RID p_rid, Camera &r_camera);
		void notify_free_instance(RID p_rid, Instance &r_instance);
		LocalVector<RID> instance_interpolate_update_list;
		LocalVector<RID> instance_transform_update_lists[2];
		LocalVector<RID> *instance_transform_update_list_curr = &instance_transform_update_lists[0];
		LocalVector<RID> *instance_transform_update_list_prev = &instance_transform_update_lists[1];
		LocalVector<RID> instance_teleport_list;

		LocalVector<RID> camera_transform_update_lists[2];
		LocalVector<RID> *camera_transform_update_list_curr = &camera_transform_update_lists[0];
		LocalVector<RID> *camera_transform_update_list_prev = &camera_transform_update_lists[1];
		LocalVector<RID> camera_teleport_list;

		bool interpolation_enabled = false;
	} _interpolation_data;

	void _instance_queue_update(Instance *p_instance, bool p_update_aabb, bool p_update_materials = false);

	struct InstanceGeometryData : public InstanceBaseData {
		List<Instance *> lighting;
		bool lighting_dirty;
		bool material_is_animated;

		InstanceGeometryData() {
			lighting_dirty = true;
			material_is_animated = true;
		}
	};

	int instance_cull_count;
	Instance *instance_cull_result[MAX_INSTANCE_CULL];

	RID_Owner<Instance> instance_owner;

public:
	virtual void callbacks_register(RenderingServerCallbacks *p_callbacks);
	RenderingServerCallbacks *get_callbacks() const {
		return _rendering_server_callbacks;
	}

	_FORCE_INLINE_ void _update_instance(Instance *p_instance);
	_FORCE_INLINE_ void _update_instance_aabb(Instance *p_instance);
	_FORCE_INLINE_ void _update_dirty_instance(Instance *p_instance);

	void _prepare_scene(const Transform p_cam_transform, const Projection &p_cam_projection, bool p_cam_orthogonal, RID p_force_environment, uint32_t p_visible_layers, RID p_scenario, int32_t &r_previous_room_id_hint);
	void _render_scene(const Transform p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_orthogonal, RID p_force_environment, RID p_scenario);
	void render_empty_scene(RID p_scenario);

	void render_camera(RID p_camera, RID p_scenario, Size2 p_viewport_size);
	void update_dirty_instances();

	// interpolation
	void update_interpolation_tick(bool p_process = true);
	void update_interpolation_frame(bool p_process = true);

	bool free(RID p_rid);

private:
	bool _use_bvh;
	RenderingServerCallbacks *_rendering_server_callbacks;

public:
	RenderingServerScene();
	virtual ~RenderingServerScene();
};

#endif // RENDERINGSERVERSCENE_H

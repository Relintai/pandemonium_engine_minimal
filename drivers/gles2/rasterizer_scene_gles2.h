#ifndef RASTERIZERSCENEGLES2_H
#define RASTERIZERSCENEGLES2_H
/*************************************************************************/
/*  rasterizer_scene_gles2.h                                             */
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

/* Must come before shaders or the Windows build fails... */
#include "rasterizer_storage_gles2.h"

#include "shaders/cube_to_dp.glsl.gen.h"
#include "shaders/effect_blur.glsl.gen.h"
#include "shaders/scene.glsl.gen.h"
#include "shaders/tonemap.glsl.gen.h"
/*


#include "drivers/gles3/shaders/exposure.glsl.gen.h"
#include "drivers/gles3/shaders/resolve.glsl.gen.h"
#include "drivers/gles3/shaders/scene.glsl.gen.h"
#include "drivers/gles3/shaders/screen_space_reflection.glsl.gen.h"
#include "drivers/gles3/shaders/ssao.glsl.gen.h"
#include "drivers/gles3/shaders/ssao_blur.glsl.gen.h"
#include "drivers/gles3/shaders/ssao_minify.glsl.gen.h"
#include "drivers/gles3/shaders/subsurf_scattering.glsl.gen.h"

*/

class RasterizerSceneGLES2 : public RasterizerScene {
public:
	enum ShadowFilterMode {
		SHADOW_FILTER_NEAREST,
		SHADOW_FILTER_PCF5,
		SHADOW_FILTER_PCF13,
	};

	enum {
		INSTANCE_ATTRIB_BASE = 8,
		INSTANCE_BONE_BASE = 13,
	};

	ShadowFilterMode shadow_filter_mode;

	RID default_material;
	RID default_material_twosided;
	RID default_shader;
	RID default_shader_twosided;

	RID default_worldcoord_material;
	RID default_worldcoord_material_twosided;
	RID default_worldcoord_shader;
	RID default_worldcoord_shader_twosided;

	RID default_overdraw_material;
	RID default_overdraw_shader;

	uint64_t render_pass;
	uint64_t scene_pass;
	uint32_t current_material_index;
	uint32_t current_geometry_index;
	uint32_t current_light_index;
	uint32_t current_refprobe_index;
	uint32_t current_shader_index;

private:
	uint32_t _light_counter;
	static const GLenum gl_primitive[];
	static const GLenum _cube_side_enum[6];

public:
	RasterizerStorageGLES2 *storage;
	struct State {
		bool texscreen_copied;
		int current_blend_mode;
		float current_line_width;
		int current_depth_draw;
		bool current_depth_test;
		GLuint current_main_tex;

		SceneShaderGLES2 scene_shader;
		CubeToDpShaderGLES2 cube_to_dp_shader;
		TonemapShaderGLES2 tonemap_shader;
		EffectBlurShaderGLES2 effect_blur_shader;

		GLuint sky_verts;

		GLuint immediate_buffer;
		Color default_ambient;
		Color default_bg;

		// ResolveShaderGLES3 resolve_shader;
		// ScreenSpaceReflectionShaderGLES3 ssr_shader;
		// EffectBlurShaderGLES3 effect_blur_shader;
		// SubsurfScatteringShaderGLES3 sss_shader;
		// SsaoMinifyShaderGLES3 ssao_minify_shader;
		// SsaoShaderGLES3 ssao_shader;
		// SsaoBlurShaderGLES3 ssao_blur_shader;
		// ExposureShaderGLES3 exposure_shader;

		/*
		struct SceneDataUBO {
			//this is a std140 compatible struct. Please read the OpenGL 3.3 Specificaiton spec before doing any changes
			float projection_matrix[16];
			float inv_projection_matrix[16];
			float camera_inverse_matrix[16];
			float camera_matrix[16];
			float ambient_light_color[4];
			float bg_color[4];
			float fog_color_enabled[4];
			float fog_sun_color_amount[4];

			float ambient_energy;
			float bg_energy;
			float z_offset;
			float z_slope_scale;
			float shadow_dual_paraboloid_render_zfar;
			float shadow_dual_paraboloid_render_side;
			float viewport_size[2];
			float screen_pixel_size[2];
			float shadow_atlas_pixel_size[2];
			float shadow_directional_pixel_size[2];

			float time;
			float z_far;
			float reflection_multiplier;
			float subsurface_scatter_width;
			float ambient_occlusion_affect_light;

			uint32_t fog_depth_enabled;
			float fog_depth_begin;
			float fog_depth_curve;
			uint32_t fog_transmit_enabled;
			float fog_transmit_curve;
			uint32_t fog_height_enabled;
			float fog_height_min;
			float fog_height_max;
			float fog_height_curve;
			// make sure this struct is padded to be a multiple of 16 bytes for webgl

		} ubo_data;

		GLuint scene_ubo;

		struct Environment3DRadianceUBO {

			float transform[16];
			float ambient_contribution;
			uint8_t padding[12];

		} env_radiance_data;

		GLuint env_radiance_ubo;

		GLuint sky_array;

		GLuint directional_ubo;

		GLuint spot_array_ubo;
		GLuint omni_array_ubo;
		GLuint reflection_array_ubo;

		GLuint immediate_buffer;
		GLuint immediate_array;

		uint32_t ubo_light_size;
		uint8_t *spot_array_tmp;
		uint8_t *omni_array_tmp;
		uint8_t *reflection_array_tmp;

		int max_ubo_lights;
		int max_forward_lights_per_object;
		int max_ubo_reflections;
		int max_skeleton_bones;

		bool used_contact_shadows;

		int spot_light_count;
		int omni_light_count;
		int directional_light_count;
		int reflection_probe_count;

		bool used_sss;
		bool using_contact_shadows;

		RS::ViewportDebugDraw debug_draw;
		*/

		bool cull_front;
		bool cull_disabled;

		bool used_screen_texture;
		bool shadow_is_dual_parabolloid;
		float dual_parbolloid_direction;
		float dual_parbolloid_zfar;

		bool render_no_shadows;

		Vector2 viewport_size;

		Vector2 screen_pixel_size;
	} state;

	/* SHADOW ATLAS API */

	uint64_t shadow_atlas_realloc_tolerance_msec;

	struct ShadowAtlas : public RID_Data {
		enum {
			QUADRANT_SHIFT = 27,
			SHADOW_INDEX_MASK = (1 << QUADRANT_SHIFT) - 1,
			SHADOW_INVALID = 0xFFFFFFFF,
		};

		struct Quadrant {
			uint32_t subdivision;

			struct Shadow {
				RID owner;
				uint64_t version;
				uint64_t alloc_tick;

				Shadow() {
					version = 0;
					alloc_tick = 0;
				}
			};

			Vector<Shadow> shadows;

			Quadrant() {
				subdivision = 0;
			}
		} quadrants[4];

		int size_order[4];
		uint32_t smallest_subdiv;

		int size;

		GLuint fbo;
		GLuint depth;
		GLuint color;

		RBMap<RID, uint32_t> shadow_owners;
	};

	struct ShadowCubeMap {
		GLuint fbo[6];
		GLuint cubemap;
		uint32_t size;
	};

	Vector<ShadowCubeMap> shadow_cubemaps;

	RID_Owner<ShadowAtlas> shadow_atlas_owner;

	int directional_shadow_size;

	void directional_shadow_create();

	RID shadow_atlas_create();
	void shadow_atlas_set_size(RID p_atlas, int p_size);
	void shadow_atlas_set_quadrant_subdivision(RID p_atlas, int p_quadrant, int p_subdivision);
	bool _shadow_atlas_find_shadow(ShadowAtlas *shadow_atlas, int *p_in_quadrants, int p_quadrant_count, int p_current_subdiv, uint64_t p_tick, int &r_quadrant, int &r_shadow);
	bool shadow_atlas_update_light(RID p_atlas, RID p_light_intance, float p_coverage, uint64_t p_light_version);

	struct DirectionalShadow {
		GLuint fbo = 0;
		GLuint depth = 0;
		GLuint color = 0;

		int light_count = 0;
		int size = 0;
		int current_light = 0;
	} directional_shadow;

	virtual int get_directional_light_shadow_size(RID p_light_intance);
	virtual void set_directional_shadow_count(int p_count);

	/* REFLECTION PROBE ATLAS API */

	virtual RID reflection_atlas_create();
	virtual void reflection_atlas_set_size(RID p_ref_atlas, int p_size);
	virtual void reflection_atlas_set_subdivision(RID p_ref_atlas, int p_subdiv);

	/* REFLECTION CUBEMAPS */

	/* REFLECTION PROBE INSTANCE */

	struct ReflectionProbeInstance : public RID_Data {
		RasterizerStorageGLES2::ReflectionProbe *probe_ptr;
		RID probe;
		RID self;
		RID atlas;

		int reflection_atlas_index;

		int render_step;
		int reflection_index;

		GLuint fbo[6];
		GLuint color[6];
		GLuint depth;
		GLuint cubemap;

		int current_resolution;
		mutable bool dirty;

		uint64_t last_pass;
		uint32_t index;

		Transform transform;
	};

	mutable RID_Owner<ReflectionProbeInstance> reflection_probe_instance_owner;

	ReflectionProbeInstance **reflection_probe_instances;
	int reflection_probe_count;

	virtual RID reflection_probe_instance_create(RID p_probe);
	virtual void reflection_probe_instance_set_transform(RID p_instance, const Transform &p_transform);
	virtual void reflection_probe_release_atlas_index(RID p_instance);
	virtual bool reflection_probe_instance_needs_redraw(RID p_instance);
	virtual bool reflection_probe_instance_has_reflection(RID p_instance);
	virtual bool reflection_probe_instance_begin_render(RID p_instance, RID p_reflection_atlas);
	virtual bool reflection_probe_instance_postprocess_step(RID p_instance);

	/* LIGHT INSTANCE */

	struct LightInstance : public RID_Data {
		struct ShadowTransform {
			Projection camera;
			Transform transform;
			float farplane;
			float split;
			float bias_scale;
		};

		ShadowTransform shadow_transform[4];

		RID self;
		RID light;

		RasterizerStorageGLES2::Light *light_ptr;
		Transform transform;

		Vector3 light_vector;
		Vector3 spot_vector;
		float linear_att;

		// TODO passes and all that stuff ?
		uint64_t last_scene_pass;
		uint64_t last_scene_shadow_pass;

		uint16_t light_index;
		uint16_t light_directional_index;

		Rect2 directional_rect;

		// an ever increasing counter for each light added,
		// used for sorting lights for a consistent render
		uint32_t light_counter;

		RBSet<RID> shadow_atlases; // atlases where this light is registered
	};

	mutable RID_Owner<LightInstance> light_instance_owner;

	virtual RID light_instance_create(RID p_light);
	virtual void light_instance_set_transform(RID p_light_instance, const Transform &p_transform);
	virtual void light_instance_set_shadow_transform(RID p_light_instance, const Projection &p_projection, const Transform &p_transform, float p_far, float p_split, int p_pass, float p_bias_scale = 1.0);
	virtual void light_instance_mark_visible(RID p_light_instance);
	virtual bool light_instances_can_render_shadow_cube() const { return storage->config.support_shadow_cubemaps; }

	LightInstance **render_light_instances;
	int render_directional_lights;
	int render_light_instance_count;

	/* RENDER LIST */

	enum LightMode {
		LIGHTMODE_NORMAL,
		LIGHTMODE_UNSHADED,
	};

	struct RenderList {
		enum {
			MAX_LIGHTS = 255,
			MAX_REFLECTION_PROBES = 255,
			DEFAULT_MAX_ELEMENTS = 65536
		};

		int max_elements;

		struct Element {
			RasterizerScene::InstanceBase *instance;

			RasterizerStorageGLES2::Geometry *geometry;
			RasterizerStorageGLES2::Material *material;
			RasterizerStorageGLES2::GeometryOwner *owner;

			bool use_accum; //is this an add pass for multipass
			bool *use_accum_ptr;
			bool front_facing;

			union {
				//TODO: should be endian swapped on big endian
				struct {
					int32_t depth_layer : 16;
					int32_t priority : 16;
				};

				uint32_t depth_key;
			};

			union {
				struct {
					//from least significant to most significant in sort, TODO: should be endian swapped on big endian

					uint64_t geometry_index : 14;
					uint64_t instancing : 1;
					uint64_t skeleton : 1;
					uint64_t shader_index : 10;
					uint64_t material_index : 10;
					uint64_t light_index : 8;
					uint64_t light_type2 : 1; // if 1==0 : nolight/directional, else omni/spot
					uint64_t refprobe_1_index : 8;
					uint64_t refprobe_0_index : 8;
					uint64_t light_type1 : 1; //no light, directional is 0, omni spot is 1
					uint64_t light_mode : 2; // LightMode enum
				};

				uint64_t sort_key;
			};
		};

		Element *base_elements;
		Element **elements;

		int element_count;
		int alpha_element_count;

		void clear() {
			element_count = 0;
			alpha_element_count = 0;
		}

		// sorts

		struct SortByKey {
			_FORCE_INLINE_ bool operator()(const Element *A, const Element *B) const {
				if (A->depth_key == B->depth_key) {
					return A->sort_key < B->sort_key;
				} else {
					return A->depth_key < B->depth_key;
				}
			}
		};

		void sort_by_key(bool p_alpha) {
			SortArray<Element *, SortByKey> sorter;

			if (p_alpha) {
				sorter.sort(&elements[max_elements - alpha_element_count], alpha_element_count);
			} else {
				sorter.sort(elements, element_count);
			}
		}

		struct SortByDepth {
			_FORCE_INLINE_ bool operator()(const Element *A, const Element *B) const {
				return A->instance->depth < B->instance->depth;
			}
		};

		void sort_by_depth(bool p_alpha) { //used for shadows

			SortArray<Element *, SortByDepth> sorter;
			if (p_alpha) {
				sorter.sort(&elements[max_elements - alpha_element_count], alpha_element_count);
			} else {
				sorter.sort(elements, element_count);
			}
		}

		struct SortByReverseDepthAndPriority {
			_FORCE_INLINE_ bool operator()(const Element *A, const Element *B) const {
				if (A->priority == B->priority) {
					return A->instance->depth > B->instance->depth;
				} else {
					return A->priority < B->priority;
				}
			}
		};

		void sort_by_reverse_depth_and_priority(bool p_alpha) { //used for alpha

			SortArray<Element *, SortByReverseDepthAndPriority> sorter;
			if (p_alpha) {
				sorter.sort(&elements[max_elements - alpha_element_count], alpha_element_count);
			} else {
				sorter.sort(elements, element_count);
			}
		}

		// element adding and stuff

		_FORCE_INLINE_ Element *add_element() {
			if (element_count + alpha_element_count >= max_elements) {
				return nullptr;
			}

			elements[element_count] = &base_elements[element_count];
			return elements[element_count++];
		}

		_FORCE_INLINE_ Element *add_alpha_element() {
			if (element_count + alpha_element_count >= max_elements) {
				return nullptr;
			}

			int idx = max_elements - alpha_element_count - 1;
			elements[idx] = &base_elements[idx];
			alpha_element_count++;
			return elements[idx];
		}

		void init() {
			element_count = 0;
			alpha_element_count = 0;

			elements = memnew_arr(Element *, max_elements);
			base_elements = memnew_arr(Element, max_elements);

			for (int i = 0; i < max_elements; i++) {
				elements[i] = &base_elements[i];
			}
		}

		RenderList() {
			max_elements = DEFAULT_MAX_ELEMENTS;
		}

		~RenderList() {
			memdelete_arr(elements);
			memdelete_arr(base_elements);
		}
	};

	RenderList render_list;

	void _add_geometry(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, int p_material, bool p_depth_pass, bool p_shadow_pass);
	void _add_geometry_with_material(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, RasterizerStorageGLES2::Material *p_material, bool p_depth_pass, bool p_shadow_pass);

	void _copy_texture_to_buffer(GLuint p_texture, GLuint p_buffer);
	void _fill_render_list(InstanceBase **p_cull_result, int p_cull_count, bool p_depth_pass, bool p_shadow_pass);
	void _render_render_list(RenderList::Element **p_elements, int p_element_count,
			const Transform &p_view_transform,
			const Projection &p_projection,
			const int p_eye,
			RID p_shadow_atlas,
			float p_shadow_bias,
			float p_shadow_normal_bias,
			bool p_reverse_cull,
			bool p_alpha_pass,
			bool p_shadow);

	_FORCE_INLINE_ void _set_cull(bool p_front, bool p_disabled, bool p_reverse_cull);
	_FORCE_INLINE_ bool _setup_material(RasterizerStorageGLES2::Material *p_material, bool p_alpha_pass, Size2i p_skeleton_tex_size = Size2i(0, 0));
	_FORCE_INLINE_ void _setup_geometry(RenderList::Element *p_element, RasterizerStorageGLES2::Skeleton *p_skeleton);
	_FORCE_INLINE_ void _setup_light_type(LightInstance *p_light, ShadowAtlas *shadow_atlas);
	_FORCE_INLINE_ void _setup_light(LightInstance *p_light, ShadowAtlas *shadow_atlas, const Transform &p_view_transform, bool accum_pass);
	_FORCE_INLINE_ void _setup_refprobes(ReflectionProbeInstance *p_refprobe1, ReflectionProbeInstance *p_refprobe2, const Transform &p_view_transform);
	_FORCE_INLINE_ void _render_geometry(RenderList::Element *p_element);

	void _post_process(const Projection &p_cam_projection);

	virtual void render_scene(const Transform &p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count, RID *p_light_cull_result, int p_light_cull_count, RID *p_reflection_probe_cull_result, int p_reflection_probe_cull_count, RID p_environment, RID p_shadow_atlas, RID p_reflection_atlas, RID p_reflection_probe, int p_reflection_probe_pass);
	virtual void render_shadow(RID p_light, RID p_shadow_atlas, int p_pass, InstanceBase **p_cull_result, int p_cull_count);
	virtual bool free(RID p_rid);

	virtual void set_scene_pass(uint64_t p_pass);
	virtual void set_debug_draw_mode(RS::ViewportDebugDraw p_debug_draw);

	void iteration();
	void initialize();
	void finalize();
	RasterizerSceneGLES2();
	~RasterizerSceneGLES2();
};

#endif // RASTERIZERSCENEGLES2_H

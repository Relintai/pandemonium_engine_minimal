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
	enum {
		INSTANCE_ATTRIB_BASE = 8,
		INSTANCE_BONE_BASE = 13,
	};

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

	/* RENDER LIST */

	enum LightMode {
		LIGHTMODE_NORMAL,
		LIGHTMODE_UNSHADED,
	};

	struct RenderList {
		enum {
			MAX_LIGHTS = 255,
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

	void _add_geometry(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, int p_material, bool p_depth_pass);
	void _add_geometry_with_material(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, RasterizerStorageGLES2::Material *p_material, bool p_depth_pass);

	void _copy_texture_to_buffer(GLuint p_texture, GLuint p_buffer);
	void _fill_render_list(InstanceBase **p_cull_result, int p_cull_count, bool p_depth_pass);
	void _render_render_list(RenderList::Element **p_elements, int p_element_count,
			const Transform &p_view_transform,
			const Projection &p_projection,
			const int p_eye,
			bool p_reverse_cull,
			bool p_alpha_pass);

	_FORCE_INLINE_ void _set_cull(bool p_front, bool p_disabled, bool p_reverse_cull);
	_FORCE_INLINE_ bool _setup_material(RasterizerStorageGLES2::Material *p_material, bool p_alpha_pass, Size2i p_skeleton_tex_size = Size2i(0, 0));
	_FORCE_INLINE_ void _setup_geometry(RenderList::Element *p_element);
	_FORCE_INLINE_ void _render_geometry(RenderList::Element *p_element);

	void _post_process(const Projection &p_cam_projection);

	virtual void render_scene(const Transform &p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count);
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

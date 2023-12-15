#ifndef RASTERIZER_DUMMY_H
#define RASTERIZER_DUMMY_H
/*************************************************************************/
/*  rasterizer_dummy.h                                                   */
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

#include "core/math/projection.h"
#include "core/containers/self_list.h"
#include "scene/resources/mesh/mesh.h"
#include "servers/rendering/rasterizer.h"
#include "servers/rendering_server.h"

class RasterizerSceneDummy : public RasterizerScene {
public:
	/* SHADOW ATLAS API */

	RID shadow_atlas_create() { return RID(); }
	void shadow_atlas_set_size(RID p_atlas, int p_size) {}
	void shadow_atlas_set_quadrant_subdivision(RID p_atlas, int p_quadrant, int p_subdivision) {}
	bool shadow_atlas_update_light(RID p_atlas, RID p_light_intance, float p_coverage, uint64_t p_light_version) { return false; }

	int get_directional_light_shadow_size(RID p_light_intance) { return 0; }
	void set_directional_shadow_count(int p_count) {}

	void render_scene(const Transform &p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count, RID *p_light_cull_result, int p_light_cull_count, RID p_environment, RID p_shadow_atlas) {}
	void render_shadow(RID p_light, RID p_shadow_atlas, int p_pass, InstanceBase **p_cull_result, int p_cull_count) {}

	void set_scene_pass(uint64_t p_pass) {}
	void set_debug_draw_mode(RS::ViewportDebugDraw p_debug_draw) {}

	bool free(RID p_rid) { return true; }

	RasterizerSceneDummy() {}
	~RasterizerSceneDummy() {}
};

class RasterizerStorageDummy : public RasterizerStorage {
public:
	/* TEXTURE API */
	struct DummyTexture : public RID_Data {
		int width;
		int height;
		uint32_t flags;
		Image::Format format;
		Ref<Image> image;
		String path;
	};

	struct DummySurface {
		uint32_t format;
		RS::PrimitiveType primitive;
		PoolVector<uint8_t> array;
		int vertex_count;
		PoolVector<uint8_t> index_array;
		int index_count;
		AABB aabb;
		Vector<PoolVector<uint8_t>> blend_shapes;
		Vector<AABB> bone_aabbs;
	};

	struct DummyMesh : public RID_Data {
		Vector<DummySurface> surfaces;
		int blend_shape_count;
		RS::BlendShapeMode blend_shape_mode;
		PoolRealArray blend_shape_values;
	};

	mutable RID_Owner<DummyTexture> texture_owner;
	mutable RID_Owner<DummyMesh> mesh_owner;

	RID texture_create() {
		DummyTexture *texture = memnew(DummyTexture);
		ERR_FAIL_COND_V(!texture, RID());
		return texture_owner.make_rid(texture);
	}

	void texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, RenderingServer::TextureType p_type = RS::TEXTURE_TYPE_2D, uint32_t p_flags = RS::TEXTURE_FLAGS_DEFAULT) {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND(!t);
		t->width = p_width;
		t->height = p_height;
		t->flags = p_flags;
		t->format = p_format;
		t->image = Ref<Image>(memnew(Image));
		t->image->create(p_width, p_height, false, p_format);
	}
	void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_level) {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND(!t);
		t->width = p_image->get_width();
		t->height = p_image->get_height();
		t->format = p_image->get_format();
		t->image->create(t->width, t->height, false, t->format, p_image->get_data());
	}

	void texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_level) {
		DummyTexture *t = texture_owner.get(p_texture);

		ERR_FAIL_COND(!t);
		ERR_FAIL_COND_MSG(p_image.is_null(), "It's not a reference to a valid Image object.");
		ERR_FAIL_COND(t->format != p_image->get_format());
		ERR_FAIL_COND(src_w <= 0 || src_h <= 0);
		ERR_FAIL_COND(src_x < 0 || src_y < 0 || src_x + src_w > p_image->get_width() || src_y + src_h > p_image->get_height());
		ERR_FAIL_COND(dst_x < 0 || dst_y < 0 || dst_x + src_w > t->width || dst_y + src_h > t->height);

		t->image->blit_rect(p_image, Rect2(src_x, src_y, src_w, src_h), Vector2(dst_x, dst_y));
	}

	Ref<Image> texture_get_data(RID p_texture, int p_level) const {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, Ref<Image>());
		return t->image;
	}
	void texture_set_flags(RID p_texture, uint32_t p_flags) {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND(!t);
		t->flags = p_flags;
	}
	uint32_t texture_get_flags(RID p_texture) const {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, 0);
		return t->flags;
	}
	Image::Format texture_get_format(RID p_texture) const {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, Image::FORMAT_RGB8);
		return t->format;
	}

	RenderingServer::TextureType texture_get_type(RID p_texture) const { return RS::TEXTURE_TYPE_2D; }
	uint32_t texture_get_texid(RID p_texture) const { return 0; }
	uint32_t texture_get_width(RID p_texture) const { return 0; }
	uint32_t texture_get_height(RID p_texture) const { return 0; }
	uint32_t texture_get_depth(RID p_texture) const { return 0; }
	void texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth_3d) {}
	void texture_bind(RID p_texture, uint32_t p_texture_no) {}

	void texture_set_path(RID p_texture, const String &p_path) {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND(!t);
		t->path = p_path;
	}
	String texture_get_path(RID p_texture) const {
		DummyTexture *t = texture_owner.getornull(p_texture);
		ERR_FAIL_COND_V(!t, String());
		return t->path;
	}

	void texture_set_shrink_all_x2_on_set_data(bool p_enable) {}

	void texture_debug_usage(List<RS::TextureInfo> *r_info) {}

	RID texture_create_radiance_cubemap(RID p_source, int p_resolution = -1) const { return RID(); }

	void texture_set_detect_3d_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) {}
	void texture_set_detect_srgb_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) {}
	void texture_set_detect_normal_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) {}

	void textures_keep_original(bool p_enable) {}

	void texture_set_proxy(RID p_proxy, RID p_base) {}
	virtual Size2 texture_size_with_proxy(RID p_texture) const { return Size2(); }
	void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {}

	/* SHADER API */

	RID shader_create() { return RID(); }

	void shader_set_code(RID p_shader, const String &p_code) {}
	String shader_get_code(RID p_shader) const { return ""; }
	void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const {}

	void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) {}
	RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const { return RID(); }

	void shader_add_custom_define(RID p_shader, const String &p_define) {}
	void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const {}
	void shader_remove_custom_define(RID p_shader, const String &p_define) {}

	void set_shader_async_hidden_forbidden(bool p_forbidden) {}
	bool is_shader_async_hidden_forbidden() { return false; }

	/* COMMON MATERIAL API */

	RID material_create() { return RID(); }

	void material_set_render_priority(RID p_material, int priority) {}
	void material_set_shader(RID p_shader_material, RID p_shader) {}
	RID material_get_shader(RID p_shader_material) const { return RID(); }

	void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) {}
	Variant material_get_param(RID p_material, const StringName &p_param) const { return Variant(); }
	Variant material_get_param_default(RID p_material, const StringName &p_param) const { return Variant(); }

	void material_set_line_width(RID p_material, float p_width) {}

	void material_set_next_pass(RID p_material, RID p_next_material) {}

	bool material_is_animated(RID p_material) { return false; }
	bool material_casts_shadows(RID p_material) { return false; }

	void material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {}
	void material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {}

	/* MESH API */

	RID mesh_create() {
		DummyMesh *mesh = memnew(DummyMesh);
		ERR_FAIL_COND_V(!mesh, RID());
		mesh->blend_shape_count = 0;
		mesh->blend_shape_mode = RS::BLEND_SHAPE_MODE_NORMALIZED;
		return mesh_owner.make_rid(mesh);
	}

	void mesh_add_surface(RID p_mesh, uint32_t p_format, RS::PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes = Vector<PoolVector<uint8_t>>(), const Vector<AABB> &p_bone_aabbs = Vector<AABB>()) {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);

		m->surfaces.push_back(DummySurface());
		DummySurface *s = &m->surfaces.write[m->surfaces.size() - 1];
		s->format = p_format;
		s->primitive = p_primitive;
		s->array = p_array;
		s->vertex_count = p_vertex_count;
		s->index_array = p_index_array;
		s->index_count = p_index_count;
		s->aabb = p_aabb;
		s->blend_shapes = p_blend_shapes;
		s->bone_aabbs = p_bone_aabbs;
	}

	void mesh_set_blend_shape_count(RID p_mesh, int p_amount) {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_count = p_amount;
	}
	int mesh_get_blend_shape_count(RID p_mesh) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);
		return m->blend_shape_count;
	}

	void mesh_set_blend_shape_mode(RID p_mesh, RS::BlendShapeMode p_mode) {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_mode = p_mode;
	}
	RS::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, RS::BLEND_SHAPE_MODE_NORMALIZED);
		return m->blend_shape_mode;
	}

	void mesh_set_blend_shape_values(RID p_mesh, PoolVector<float> p_values) {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		m->blend_shape_values = p_values;
	}
	PoolVector<float> mesh_get_blend_shape_values(RID p_mesh) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, PoolRealArray());
		return m->blend_shape_values;
	}

	void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data) {}

	void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) {}
	RID mesh_surface_get_material(RID p_mesh, int p_surface) const { return RID(); }

	int mesh_surface_get_array_len(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);

		return m->surfaces[p_surface].vertex_count;
	}
	int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);

		return m->surfaces[p_surface].index_count;
	}

	PoolVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, PoolVector<uint8_t>());

		return m->surfaces[p_surface].array;
	}
	PoolVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, PoolVector<uint8_t>());

		return m->surfaces[p_surface].index_array;
	}

	uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);

		return m->surfaces[p_surface].format;
	}
	RS::PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, RS::PRIMITIVE_POINTS);

		return m->surfaces[p_surface].primitive;
	}

	AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, AABB());

		return m->surfaces[p_surface].aabb;
	}
	Vector<PoolVector<uint8_t>> mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, Vector<PoolVector<uint8_t>>());

		return m->surfaces[p_surface].blend_shapes;
	}

	void mesh_remove_surface(RID p_mesh, int p_index) {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND(!m);
		ERR_FAIL_COND(p_index >= m->surfaces.size());

		m->surfaces.remove(p_index);
	}
	int mesh_get_surface_count(RID p_mesh) const {
		DummyMesh *m = mesh_owner.getornull(p_mesh);
		ERR_FAIL_COND_V(!m, 0);
		return m->surfaces.size();
	}

	void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) {}
	AABB mesh_get_custom_aabb(RID p_mesh) const { return AABB(); }

	AABB mesh_get_aabb(RID p_mesh) const { return AABB(); }
	void mesh_clear(RID p_mesh) {}

	/* MULTIMESH API */

	virtual RID _multimesh_create() { return RID(); }

	void _multimesh_allocate(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, RS::MultimeshColorFormat p_color_format, RS::MultimeshCustomDataFormat p_data = RS::MULTIMESH_CUSTOM_DATA_NONE) {}
	int _multimesh_get_instance_count(RID p_multimesh) const { return 0; }
	void _multimesh_set_mesh(RID p_multimesh, RID p_mesh) {}
	void _multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) {}
	void _multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) {}
	void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) {}
	void _multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) {}
	RID _multimesh_get_mesh(RID p_multimesh) const { return RID(); }
	Transform _multimesh_instance_get_transform(RID p_multimesh, int p_index) const { return Transform(); }
	Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const { return Transform2D(); }
	Color _multimesh_instance_get_color(RID p_multimesh, int p_index) const { return Color(); }
	Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const { return Color(); }
	void _multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array) {}
	void _multimesh_set_visible_instances(RID p_multimesh, int p_visible) {}
	int _multimesh_get_visible_instances(RID p_multimesh) const { return 0; }
	AABB _multimesh_get_aabb(RID p_multimesh) const { return AABB(); }

	MMInterpolator *_multimesh_get_interpolator(RID p_multimesh) const { return nullptr; }
	void multimesh_attach_canvas_item(RID p_multimesh, RID p_canvas_item, bool p_attach) {}

	/* RENDER TARGET */

	RID render_target_create() { return RID(); }
	void render_target_set_position(RID p_render_target, int p_x, int p_y) {}
	void render_target_set_size(RID p_render_target, int p_width, int p_height) {}
	RID render_target_get_texture(RID p_render_target) const { return RID(); }
	uint32_t render_target_get_depth_texture_id(RID p_render_target) const { return 0; }
	void render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id, unsigned int p_depth_id) {}
	void render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) {}
	bool render_target_was_used(RID p_render_target) { return false; }
	void render_target_clear_used(RID p_render_target) {}
	void render_target_set_msaa(RID p_render_target, RS::ViewportMSAA p_msaa) {}
	void render_target_set_use_fxaa(RID p_render_target, bool p_fxaa) {}
	void render_target_set_use_debanding(RID p_render_target, bool p_debanding) {}
	void render_target_set_sharpen_intensity(RID p_render_target, float p_intensity) {}

	/* CANVAS SHADOW */

	RID canvas_light_shadow_buffer_create(int p_width) { return RID(); }

	/* LIGHT SHADOW MAPPING */

	RID canvas_light_occluder_create() { return RID(); }
	void canvas_light_occluder_set_polylines(RID p_occluder, const PoolVector<Vector2> &p_lines) {}

	RS::InstanceType get_base_type(RID p_rid) const {
		if (mesh_owner.owns(p_rid)) {
			return RS::INSTANCE_MESH;
		}

		return RS::INSTANCE_NONE;
	}

	bool free(RID p_rid) {
		if (texture_owner.owns(p_rid)) {
			// delete the texture
			DummyTexture *texture = texture_owner.get(p_rid);
			texture_owner.free(p_rid);
			memdelete(texture);
		} else if (mesh_owner.owns(p_rid)) {
			// delete the mesh
			DummyMesh *mesh = mesh_owner.getornull(p_rid);
			mesh_owner.free(p_rid);
			memdelete(mesh);
		} else {
			return false;
		}

		return true;
	}

	bool has_os_feature(const String &p_feature) const { return false; }

	void update_dirty_resources() {}

	void set_debug_generate_wireframes(bool p_generate) {}

	void render_info_begin_capture() {}
	void render_info_end_capture() {}
	int get_captured_render_info(RS::RenderInfo p_info) { return 0; }

	uint64_t get_render_info(RS::RenderInfo p_info) { return 0; }
	String get_video_adapter_name() const { return String(); }
	String get_video_adapter_vendor() const { return String(); }

	static RasterizerStorage *base_singleton;

	RasterizerStorageDummy(){};
	~RasterizerStorageDummy() {}
};

class RasterizerCanvasDummy : public RasterizerCanvas {
public:
	RID light_internal_create() { return RID(); }
	void light_internal_update(RID p_rid, Light *p_light) {}
	void light_internal_free(RID p_rid) {}

	void canvas_begin(){};
	void canvas_end(){};

	void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, Light *p_light, const Transform2D &p_transform){};
	void canvas_debug_viewport_shadows(Light *p_lights_with_shadow){};

	void canvas_light_shadow_buffer_update(RID p_buffer, const Transform2D &p_light_xform, int p_light_mask, float p_near, float p_far, LightOccluderInstance *p_occluders, Projection *p_xform_cache) {}

	void reset_canvas() {}

	void draw_window_margins(int *p_margins, RID *p_margin_textures) {}

	RasterizerCanvasDummy() {}
	~RasterizerCanvasDummy() {}
};

class RasterizerDummy : public Rasterizer {
protected:
	RasterizerCanvasDummy canvas;
	RasterizerStorageDummy storage;
	RasterizerSceneDummy scene;

public:
	RasterizerStorage *get_storage() { return &storage; }
	RasterizerCanvas *get_canvas() { return &canvas; }
	RasterizerScene *get_scene() { return &scene; }

	void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) {}
	void set_shader_time_scale(float p_scale) {}

	void initialize() {}
	void begin_frame(double frame_step) {}
	void set_current_render_target(RID p_render_target) {}
	void restore_render_target(bool p_3d_was_drawn) {}
	void clear_render_target(const Color &p_color) {}
	void blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0) {}
	void output_lens_distorted_to_screen(RID p_render_target, const Rect2 &p_screen_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample) {}
	void end_frame(bool p_swap_buffers) {}
	void finalize() {}

	static Error is_viable() {
		return OK;
	}

	static Rasterizer *_create_current() {
		return memnew(RasterizerDummy);
	}

	static void make_current() {
		_create_func = _create_current;
	}

	virtual bool is_low_end() const { return true; }

	RasterizerDummy() {}
	~RasterizerDummy() {}
};

#endif // RASTERIZER_DUMMY_H

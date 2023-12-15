#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H
/*************************************************************************/
/*  rendering_server.h                                                      */
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

#include "core/containers/rid.h"
#include "core/io/image.h"
#include "core/math/bsp_tree.h"
#include "core/math/geometry.h"
#include "core/math/transform_2d.h"
#include "core/object/object.h"
#include "core/variant/variant.h"

class RenderingServerCallbacks;

class RenderingServer : public Object {
	GDCLASS(RenderingServer, Object);

	static RenderingServer *singleton;

	int mm_policy;
	bool render_loop_enabled = true;
#ifdef DEBUG_ENABLED
	bool force_shader_fallbacks = false;
#endif

	void _canvas_item_add_style_box(RID p_item, const Rect2 &p_rect, const Rect2 &p_source, RID p_texture, const Vector<float> &p_margins, const Color &p_modulate = Color(1, 1, 1));
	Array _get_array_from_surface(uint32_t p_format, PoolVector<uint8_t> p_vertex_data, int p_vertex_len, PoolVector<uint8_t> p_index_data, int p_index_len) const;

protected:
	RID _make_test_cube();
	void _free_internal_rids();
	RID test_texture;
	RID white_texture;
	RID test_material;

	Error _surface_set_data(Array p_arrays, uint32_t p_format, uint32_t *p_offsets, uint32_t *p_stride, PoolVector<uint8_t> &r_vertex_array, int p_vertex_array_len, PoolVector<uint8_t> &r_index_array, int p_index_array_len, AABB &r_aabb, Vector<AABB> &r_bone_aabb);

	static RenderingServer *(*create_func)();
	static void _bind_methods();

public:
	static RenderingServer *get_singleton();
	static RenderingServer *create();
	static Vector2 norm_to_oct(const Vector3 v);
	static Vector2 tangent_to_oct(const Vector3 v, const float sign, const bool high_precision);
	static Vector3 oct_to_norm(const Vector2 v);
	static Vector3 oct_to_tangent(const Vector2 v, float *out_sign);

	enum {

		NO_INDEX_ARRAY = -1,
		ARRAY_WEIGHTS_SIZE = 4,
		CANVAS_ITEM_Z_MIN = -4096,
		CANVAS_ITEM_Z_MAX = 4096,
		MAX_GLOW_LEVELS = 7,

		MAX_CURSORS = 8,
	};

	/* TEXTURE API */

	enum TextureFlags : unsigned int { // unsigned to stop sanitizer complaining about bit operations on ints
		TEXTURE_FLAG_MIPMAPS = 1, /// Enable automatic mipmap generation - when available
		TEXTURE_FLAG_REPEAT = 2, /// Repeat texture (Tiling), otherwise Clamping
		TEXTURE_FLAG_FILTER = 4, /// Create texture with linear (or available) filter
		TEXTURE_FLAG_ANISOTROPIC_FILTER = 8,
		TEXTURE_FLAG_CONVERT_TO_LINEAR = 16,
		TEXTURE_FLAG_MIRRORED_REPEAT = 32, /// Repeat texture, with alternate sections mirrored
		TEXTURE_FLAG_USED_FOR_STREAMING = 2048,
		TEXTURE_FLAGS_DEFAULT = TEXTURE_FLAG_REPEAT | TEXTURE_FLAG_MIPMAPS | TEXTURE_FLAG_FILTER
	};

	enum TextureType {
		TEXTURE_TYPE_2D,
		TEXTURE_TYPE_EXTERNAL,
		TEXTURE_TYPE_CUBEMAP,
		TEXTURE_TYPE_2D_ARRAY,
		TEXTURE_TYPE_3D,
	};

	enum CubeMapSide {

		CUBEMAP_LEFT,
		CUBEMAP_RIGHT,
		CUBEMAP_BOTTOM,
		CUBEMAP_TOP,
		CUBEMAP_FRONT,
		CUBEMAP_BACK
	};

	virtual RID texture_create() = 0;
	RID texture_create_from_image(const Ref<Image> &p_image, uint32_t p_flags = TEXTURE_FLAGS_DEFAULT); // helper
	virtual void texture_allocate(RID p_texture,
			int p_width,
			int p_height,
			int p_depth_3d,
			Image::Format p_format,
			TextureType p_type,
			uint32_t p_flags = TEXTURE_FLAGS_DEFAULT) = 0;

	virtual void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_layer = 0) = 0;
	virtual void texture_set_data_partial(RID p_texture,
			const Ref<Image> &p_image,
			int src_x, int src_y,
			int src_w, int src_h,
			int dst_x, int dst_y,
			int p_dst_mip,
			int p_layer = 0) = 0;

	virtual Ref<Image> texture_get_data(RID p_texture, int p_layer = 0) const = 0;
	virtual void texture_set_flags(RID p_texture, uint32_t p_flags) = 0;
	virtual uint32_t texture_get_flags(RID p_texture) const = 0;
	virtual Image::Format texture_get_format(RID p_texture) const = 0;
	virtual TextureType texture_get_type(RID p_texture) const = 0;
	virtual uint32_t texture_get_texid(RID p_texture) const = 0;
	virtual uint32_t texture_get_width(RID p_texture) const = 0;
	virtual uint32_t texture_get_height(RID p_texture) const = 0;
	virtual uint32_t texture_get_depth(RID p_texture) const = 0;
	virtual void texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth_3d) = 0;
	virtual void texture_bind(RID p_texture, uint32_t p_texture_no) = 0;

	virtual void texture_set_path(RID p_texture, const String &p_path) = 0;
	virtual String texture_get_path(RID p_texture) const = 0;

	virtual void texture_set_shrink_all_x2_on_set_data(bool p_enable) = 0;

	typedef void (*TextureDetectCallback)(void *);

	virtual void texture_set_detect_3d_callback(RID p_texture, TextureDetectCallback p_callback, void *p_userdata) = 0;
	virtual void texture_set_detect_srgb_callback(RID p_texture, TextureDetectCallback p_callback, void *p_userdata) = 0;
	virtual void texture_set_detect_normal_callback(RID p_texture, TextureDetectCallback p_callback, void *p_userdata) = 0;

	struct TextureInfo {
		RID texture;
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		Image::Format format;
		int bytes;
		String path;
	};

	virtual void texture_debug_usage(List<TextureInfo> *r_info) = 0;
	Array _texture_debug_usage_bind();

	virtual void textures_keep_original(bool p_enable) = 0;

	virtual void texture_set_proxy(RID p_proxy, RID p_base) = 0;
	virtual void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) = 0;

	/* SHADER API */

	enum ShaderMode {

		SHADER_SPATIAL,
		SHADER_CANVAS_ITEM,
		SHADER_PARTICLES,
		SHADER_MAX
	};

	virtual RID shader_create() = 0;

	virtual void shader_set_code(RID p_shader, const String &p_code) = 0;
	virtual String shader_get_code(RID p_shader) const = 0;
	virtual void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const = 0;
	Array _shader_get_param_list_bind(RID p_shader) const;

	virtual void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) = 0;
	virtual RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const = 0;

	virtual void shader_add_custom_define(RID p_shader, const String &p_define) = 0;
	virtual void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const = 0;
	virtual void shader_remove_custom_define(RID p_shader, const String &p_define) = 0;

	virtual void set_shader_async_hidden_forbidden(bool p_forbidden) = 0;

	/* COMMON MATERIAL API */

	enum {
		MATERIAL_RENDER_PRIORITY_MIN = -128,
		MATERIAL_RENDER_PRIORITY_MAX = 127,

	};
	virtual RID material_create() = 0;

	virtual void material_set_shader(RID p_shader_material, RID p_shader) = 0;
	virtual RID material_get_shader(RID p_shader_material) const = 0;

	virtual void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) = 0;
	virtual Variant material_get_param(RID p_material, const StringName &p_param) const = 0;
	virtual Variant material_get_param_default(RID p_material, const StringName &p_param) const = 0;

	virtual void material_set_render_priority(RID p_material, int priority) = 0;

	virtual void material_set_line_width(RID p_material, float p_width) = 0;
	virtual void material_set_next_pass(RID p_material, RID p_next_material) = 0;

	/* MESH API */

	enum ArrayType {

		ARRAY_VERTEX = 0,
		ARRAY_NORMAL = 1,
		ARRAY_TANGENT = 2,
		ARRAY_COLOR = 3,
		ARRAY_TEX_UV = 4,
		ARRAY_TEX_UV2 = 5,
		ARRAY_BONES = 6,
		ARRAY_WEIGHTS = 7,
		ARRAY_INDEX = 8,
		ARRAY_MAX = 9
	};

	enum ArrayFormat {
		/* ARRAY FORMAT FLAGS */
		ARRAY_FORMAT_VERTEX = 1 << ARRAY_VERTEX, // mandatory
		ARRAY_FORMAT_NORMAL = 1 << ARRAY_NORMAL,
		ARRAY_FORMAT_TANGENT = 1 << ARRAY_TANGENT,
		ARRAY_FORMAT_COLOR = 1 << ARRAY_COLOR,
		ARRAY_FORMAT_TEX_UV = 1 << ARRAY_TEX_UV,
		ARRAY_FORMAT_TEX_UV2 = 1 << ARRAY_TEX_UV2,
		ARRAY_FORMAT_BONES = 1 << ARRAY_BONES,
		ARRAY_FORMAT_WEIGHTS = 1 << ARRAY_WEIGHTS,
		ARRAY_FORMAT_INDEX = 1 << ARRAY_INDEX,

		ARRAY_COMPRESS_BASE = (ARRAY_INDEX + 1),
		ARRAY_COMPRESS_VERTEX = 1 << (ARRAY_VERTEX + ARRAY_COMPRESS_BASE), // mandatory
		ARRAY_COMPRESS_NORMAL = 1 << (ARRAY_NORMAL + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_TANGENT = 1 << (ARRAY_TANGENT + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_COLOR = 1 << (ARRAY_COLOR + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_TEX_UV = 1 << (ARRAY_TEX_UV + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_TEX_UV2 = 1 << (ARRAY_TEX_UV2 + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_BONES = 1 << (ARRAY_BONES + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_WEIGHTS = 1 << (ARRAY_WEIGHTS + ARRAY_COMPRESS_BASE),
		ARRAY_COMPRESS_INDEX = 1 << (ARRAY_INDEX + ARRAY_COMPRESS_BASE),

		ARRAY_FLAG_USE_2D_VERTICES = ARRAY_COMPRESS_INDEX << 1,
		ARRAY_FLAG_USE_16_BIT_BONES = ARRAY_COMPRESS_INDEX << 2,
		ARRAY_FLAG_USE_DYNAMIC_UPDATE = ARRAY_COMPRESS_INDEX << 3,
		ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION = ARRAY_COMPRESS_INDEX << 4,

		ARRAY_COMPRESS_DEFAULT = ARRAY_COMPRESS_NORMAL | ARRAY_COMPRESS_TANGENT | ARRAY_COMPRESS_COLOR | ARRAY_COMPRESS_TEX_UV | ARRAY_COMPRESS_TEX_UV2 | ARRAY_COMPRESS_WEIGHTS | ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION

	};

	enum PrimitiveType {
		PRIMITIVE_POINTS = 0,
		PRIMITIVE_LINES = 1,
		PRIMITIVE_LINE_STRIP = 2,
		PRIMITIVE_LINE_LOOP = 3,
		PRIMITIVE_TRIANGLES = 4,
		PRIMITIVE_TRIANGLE_STRIP = 5,
		PRIMITIVE_TRIANGLE_FAN = 6,
		PRIMITIVE_MAX = 7,
	};

	virtual RID mesh_create() = 0;

	virtual uint32_t mesh_surface_get_format_offset(uint32_t p_format, int p_vertex_len, int p_index_len, int p_array_index) const;
	virtual uint32_t mesh_surface_get_format_stride(uint32_t p_format, int p_vertex_len, int p_index_len, int p_array_index) const;
	/// Returns stride
	virtual void mesh_surface_make_offsets_from_format(uint32_t p_format, int p_vertex_len, int p_index_len, uint32_t *r_offsets, uint32_t *r_strides) const;
	virtual void mesh_add_surface_from_arrays(RID p_mesh, PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes = Array(), uint32_t p_compress_format = ARRAY_COMPRESS_DEFAULT);
	virtual void mesh_add_surface(RID p_mesh, uint32_t p_format, PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes = Vector<PoolVector<uint8_t>>(), const Vector<AABB> &p_bone_aabbs = Vector<AABB>()) = 0;

	virtual void mesh_set_blend_shape_count(RID p_mesh, int p_amount) = 0;
	virtual int mesh_get_blend_shape_count(RID p_mesh) const = 0;

	enum BlendShapeMode {
		BLEND_SHAPE_MODE_NORMALIZED,
		BLEND_SHAPE_MODE_RELATIVE,
	};

	virtual void mesh_set_blend_shape_mode(RID p_mesh, BlendShapeMode p_mode) = 0;
	virtual BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const = 0;

	virtual void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data) = 0;

	virtual void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) = 0;
	virtual RID mesh_surface_get_material(RID p_mesh, int p_surface) const = 0;

	virtual int mesh_surface_get_array_len(RID p_mesh, int p_surface) const = 0;
	virtual int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const = 0;

	virtual PoolVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const = 0;
	virtual PoolVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const = 0;

	virtual Array mesh_surface_get_arrays(RID p_mesh, int p_surface) const;
	virtual Array mesh_surface_get_blend_shape_arrays(RID p_mesh, int p_surface) const;

	virtual uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const = 0;
	virtual PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const = 0;

	virtual AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const = 0;
	virtual Vector<PoolVector<uint8_t>> mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const = 0;

	virtual void mesh_remove_surface(RID p_mesh, int p_index) = 0;
	virtual int mesh_get_surface_count(RID p_mesh) const = 0;

	virtual void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) = 0;
	virtual AABB mesh_get_custom_aabb(RID p_mesh) const = 0;

	virtual void mesh_clear(RID p_mesh) = 0;

	/* MULTIMESH API */

	virtual RID multimesh_create() = 0;

	enum MultimeshTransformFormat {
		MULTIMESH_TRANSFORM_2D,
		MULTIMESH_TRANSFORM_3D,
	};

	enum MultimeshColorFormat {
		MULTIMESH_COLOR_NONE,
		MULTIMESH_COLOR_8BIT,
		MULTIMESH_COLOR_FLOAT,
		MULTIMESH_COLOR_MAX,
	};

	enum MultimeshCustomDataFormat {
		MULTIMESH_CUSTOM_DATA_NONE,
		MULTIMESH_CUSTOM_DATA_8BIT,
		MULTIMESH_CUSTOM_DATA_FLOAT,
		MULTIMESH_CUSTOM_DATA_MAX,
	};

	enum MultimeshPhysicsInterpolationQuality {
		MULTIMESH_INTERP_QUALITY_FAST,
		MULTIMESH_INTERP_QUALITY_HIGH,
	};

	virtual void multimesh_allocate(RID p_multimesh, int p_instances, MultimeshTransformFormat p_transform_format, MultimeshColorFormat p_color_format, MultimeshCustomDataFormat p_data_format = MULTIMESH_CUSTOM_DATA_NONE) = 0;
	virtual int multimesh_get_instance_count(RID p_multimesh) const = 0;

	virtual void multimesh_set_mesh(RID p_multimesh, RID p_mesh) = 0;
	virtual void multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) = 0;
	virtual void multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) = 0;
	virtual void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) = 0;
	virtual void multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) = 0;

	virtual RID multimesh_get_mesh(RID p_multimesh) const = 0;
	virtual AABB multimesh_get_aabb(RID p_multimesh) const = 0;

	virtual Transform multimesh_instance_get_transform(RID p_multimesh, int p_index) const = 0;
	virtual Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const = 0;
	virtual Color multimesh_instance_get_color(RID p_multimesh, int p_index) const = 0;
	virtual Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const = 0;

	virtual void multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array) = 0;

	// Interpolation
	virtual void multimesh_set_as_bulk_array_interpolated(RID p_multimesh, const PoolVector<float> &p_array, const PoolVector<float> &p_array_prev) = 0;
	virtual void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated) = 0;
	virtual void multimesh_set_physics_interpolation_quality(RID p_multimesh, MultimeshPhysicsInterpolationQuality p_quality) = 0;
	virtual void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index) = 0;

	virtual void multimesh_set_visible_instances(RID p_multimesh, int p_visible) = 0;
	virtual int multimesh_get_visible_instances(RID p_multimesh) const = 0;

	/* VIEWPORT TARGET API */

	virtual RID viewport_create() = 0;

	virtual void viewport_set_size(RID p_viewport, int p_width, int p_height) = 0;
	virtual void viewport_set_active(RID p_viewport, bool p_active) = 0;
	virtual void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport) = 0;

	virtual void viewport_attach_to_screen(RID p_viewport, const Rect2 &p_rect = Rect2(), int p_screen = 0) = 0;
	virtual void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable) = 0;
	virtual void viewport_detach(RID p_viewport) = 0;

	enum ViewportUpdateMode {
		VIEWPORT_UPDATE_DISABLED,
		VIEWPORT_UPDATE_ONCE, //then goes to disabled, must be manually updated
		VIEWPORT_UPDATE_WHEN_VISIBLE, // default
		VIEWPORT_UPDATE_ALWAYS
	};

	virtual void viewport_set_update_mode(RID p_viewport, ViewportUpdateMode p_mode) = 0;
	virtual void viewport_set_vflip(RID p_viewport, bool p_enable) = 0;

	enum ViewportClearMode {

		VIEWPORT_CLEAR_ALWAYS,
		VIEWPORT_CLEAR_NEVER,
		VIEWPORT_CLEAR_ONLY_NEXT_FRAME
	};

	virtual void viewport_set_clear_mode(RID p_viewport, ViewportClearMode p_clear_mode) = 0;

	virtual RID viewport_get_texture(RID p_viewport) const = 0;

	virtual void viewport_set_hide_canvas(RID p_viewport, bool p_hide) = 0;
	
	virtual void viewport_attach_canvas(RID p_viewport, RID p_canvas) = 0;
	virtual void viewport_remove_canvas(RID p_viewport, RID p_canvas) = 0;
	virtual void viewport_set_canvas_transform(RID p_viewport, RID p_canvas, const Transform2D &p_offset) = 0;
	virtual void viewport_set_transparent_background(RID p_viewport, bool p_enabled) = 0;

	virtual void viewport_set_global_canvas_transform(RID p_viewport, const Transform2D &p_transform) = 0;
	virtual void viewport_set_canvas_stacking(RID p_viewport, RID p_canvas, int p_layer, int p_sublayer) = 0;

	enum ViewportMSAA {
		VIEWPORT_MSAA_DISABLED,
		VIEWPORT_MSAA_2X,
		VIEWPORT_MSAA_4X,
		VIEWPORT_MSAA_8X,
		VIEWPORT_MSAA_16X,
		VIEWPORT_MSAA_EXT_2X,
		VIEWPORT_MSAA_EXT_4X,
	};

	virtual void viewport_set_msaa(RID p_viewport, ViewportMSAA p_msaa) = 0;
	virtual void viewport_set_use_fxaa(RID p_viewport, bool p_fxaa) = 0;
	virtual void viewport_set_use_debanding(RID p_viewport, bool p_debanding) = 0;
	virtual void viewport_set_sharpen_intensity(RID p_viewport, float p_intensity) = 0;

	enum ViewportUsage {
		VIEWPORT_USAGE_2D,
		VIEWPORT_USAGE_2D_NO_SAMPLING,
	};

	virtual void viewport_set_hdr(RID p_viewport, bool p_enabled) = 0;
	virtual void viewport_set_use_32_bpc_depth(RID p_viewport, bool p_enabled) = 0;
	virtual void viewport_set_usage(RID p_viewport, ViewportUsage p_usage) = 0;

	enum ViewportRenderInfo {

		VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME,
		VIEWPORT_RENDER_INFO_VERTICES_IN_FRAME,
		VIEWPORT_RENDER_INFO_MATERIAL_CHANGES_IN_FRAME,
		VIEWPORT_RENDER_INFO_SHADER_CHANGES_IN_FRAME,
		VIEWPORT_RENDER_INFO_SURFACE_CHANGES_IN_FRAME,
		VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME,
		VIEWPORT_RENDER_INFO_2D_ITEMS_IN_FRAME,
		VIEWPORT_RENDER_INFO_2D_DRAW_CALLS_IN_FRAME,
		VIEWPORT_RENDER_INFO_MAX
	};

	virtual int viewport_get_render_info(RID p_viewport, ViewportRenderInfo p_info) = 0;

	enum ViewportDebugDraw {
		VIEWPORT_DEBUG_DRAW_DISABLED,
		VIEWPORT_DEBUG_DRAW_UNSHADED,
		VIEWPORT_DEBUG_DRAW_OVERDRAW,
		VIEWPORT_DEBUG_DRAW_WIREFRAME,
	};

	virtual void viewport_set_debug_draw(RID p_viewport, ViewportDebugDraw p_draw) = 0;

	/* INTERPOLATION API */

	virtual void set_physics_interpolation_enabled(bool p_enabled) = 0;

	/* INSTANCING API */

	enum InstanceType {

		INSTANCE_NONE,
		INSTANCE_MESH,
		INSTANCE_MULTIMESH,
		INSTANCE_MAX,

		INSTANCE_GEOMETRY_MASK = (1 << INSTANCE_MESH) | (1 << INSTANCE_MULTIMESH)
	};

	enum InstanceFlags {
		INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE,
		INSTANCE_FLAG_MAX
	};

	/* CANVAS (2D) */

	virtual RID canvas_create() = 0;
	virtual void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2 &p_mirroring) = 0;
	virtual void canvas_set_modulate(RID p_canvas, const Color &p_color) = 0;
	virtual void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale) = 0;

	virtual void canvas_set_disable_scale(bool p_disable) = 0;

	virtual RID canvas_item_create() = 0;
	virtual void canvas_item_set_parent(RID p_item, RID p_parent) = 0;
	virtual void canvas_item_set_name(RID p_item, String p_name) = 0;

	virtual void canvas_item_set_visible(RID p_item, bool p_visible) = 0;
	virtual void canvas_item_set_light_mask(RID p_item, int p_mask) = 0;

	virtual void canvas_item_set_update_when_visible(RID p_item, bool p_update) = 0;

	virtual void canvas_item_set_transform(RID p_item, const Transform2D &p_transform) = 0;
	virtual void canvas_item_set_clip(RID p_item, bool p_clip) = 0;
	virtual void canvas_item_set_distance_field_mode(RID p_item, bool p_enable) = 0;
	virtual void canvas_item_set_custom_rect(RID p_item, bool p_custom_rect, const Rect2 &p_rect = Rect2()) = 0;
	virtual void canvas_item_set_modulate(RID p_item, const Color &p_color) = 0;
	virtual void canvas_item_set_self_modulate(RID p_item, const Color &p_color) = 0;

	virtual void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable) = 0;
	virtual void canvas_item_set_use_identity_transform(RID p_item, bool p_enable) = 0;

	enum NinePatchAxisMode {
		NINE_PATCH_STRETCH,
		NINE_PATCH_TILE,
		NINE_PATCH_TILE_FIT,
	};

	virtual void canvas_item_add_line(RID p_item, const Point2 &p_from, const Point2 &p_to, const Color &p_color, float p_width = 1.0, bool p_antialiased = false) = 0;
	virtual void canvas_item_add_polyline(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, float p_width = 1.0, bool p_antialiased = false) = 0;
	virtual void canvas_item_add_multiline(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, float p_width = 1.0, bool p_antialiased = false) = 0;
	virtual void canvas_item_add_rect(RID p_item, const Rect2 &p_rect, const Color &p_color) = 0;
	virtual void canvas_item_add_circle(RID p_item, const Point2 &p_pos, float p_radius, const Color &p_color) = 0;
	virtual void canvas_item_add_texture_rect(RID p_item, const Rect2 &p_rect, RID p_texture, bool p_tile = false, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID()) = 0;
	virtual void canvas_item_add_texture_rect_region(RID p_item, const Rect2 &p_rect, RID p_texture, const Rect2 &p_src_rect, const Color &p_modulate = Color(1, 1, 1), bool p_transpose = false, RID p_normal_map = RID(), bool p_clip_uv = false) = 0;
	virtual void canvas_item_add_texture_multirect_region(RID p_item, const Vector<Rect2> &p_rects, RID p_texture, const Vector<Rect2> &p_src_rects, const Color &p_modulate = Color(1, 1, 1), uint32_t p_canvas_rect_flags = 0, RID p_normal_map = RID()) = 0;
	virtual void canvas_item_add_nine_patch(RID p_item, const Rect2 &p_rect, const Rect2 &p_source, RID p_texture, const Vector2 &p_topleft, const Vector2 &p_bottomright, NinePatchAxisMode p_x_axis_mode = NINE_PATCH_STRETCH, NinePatchAxisMode p_y_axis_mode = NINE_PATCH_STRETCH, bool p_draw_center = true, const Color &p_modulate = Color(1, 1, 1), RID p_normal_map = RID()) = 0;
	virtual void canvas_item_add_primitive(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs, RID p_texture, float p_width = 1.0, RID p_normal_map = RID()) = 0;
	virtual void canvas_item_add_polygon(RID p_item, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs = Vector<Point2>(), RID p_texture = RID(), RID p_normal_map = RID(), bool p_antialiased = false) = 0;
	virtual void canvas_item_add_triangle_array(RID p_item, const Vector<int> &p_indices, const Vector<Point2> &p_points, const Vector<Color> &p_colors, const Vector<Point2> &p_uvs = Vector<Point2>(), const Vector<int> &p_bones = Vector<int>(), const Vector<float> &p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1, RID p_normal_map = RID(), bool p_antialiased = false, bool p_antialiasing_use_indices = false) = 0;
	virtual void canvas_item_add_mesh(RID p_item, const RID &p_mesh, const Transform2D &p_transform = Transform2D(), const Color &p_modulate = Color(1, 1, 1), RID p_texture = RID(), RID p_normal_map = RID()) = 0;
	virtual void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID(), RID p_normal_map = RID()) = 0;
	virtual void canvas_item_add_set_transform(RID p_item, const Transform2D &p_transform) = 0;
	virtual void canvas_item_add_clip_ignore(RID p_item, bool p_ignore) = 0;
	virtual void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable) = 0;
	virtual void canvas_item_set_z_index(RID p_item, int p_z) = 0;
	virtual void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable) = 0;
	virtual void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2 &p_rect) = 0;
	virtual void canvas_item_clear(RID p_item) = 0;
	virtual void canvas_item_set_draw_index(RID p_item, int p_index) = 0;
	virtual void canvas_item_set_material(RID p_item, RID p_material) = 0;
	virtual void canvas_item_set_use_parent_material(RID p_item, bool p_enable) = 0;

	virtual void canvas_item_attach_skeleton(RID p_item, RID p_skeleton) = 0;
	virtual void canvas_item_set_skeleton_relative_xform(RID p_item, Transform2D p_relative_xform) = 0;

#ifdef TOOLS_ENABLED
	Rect2 debug_canvas_item_get_rect(RID p_item) { return _debug_canvas_item_get_rect(p_item); }
	Rect2 debug_canvas_item_get_local_bound(RID p_item) { return _debug_canvas_item_get_local_bound(p_item); }
#else
	Rect2 debug_canvas_item_get_rect(RID p_item) { return Rect2(); }
	Rect2 debug_canvas_item_get_local_bound(RID p_item) { return Rect2(); }
#endif

	virtual Rect2 _debug_canvas_item_get_rect(RID p_item) = 0;
	virtual Rect2 _debug_canvas_item_get_local_bound(RID p_item) = 0;

	virtual void canvas_item_set_interpolated(RID p_item, bool p_interpolated) = 0;
	virtual void canvas_item_reset_physics_interpolation(RID p_item) = 0;
	virtual void canvas_item_transform_physics_interpolation(RID p_item, Transform2D p_transform) = 0;

	/* BLACK BARS */

	virtual void black_bars_set_margins(int p_left, int p_top, int p_right, int p_bottom) = 0;
	virtual void black_bars_set_images(RID p_left, RID p_top, RID p_right, RID p_bottom) = 0;

	/* FREE */

	virtual void free(RID p_rid) = 0; ///< free RIDs associated with the visual server

	virtual void request_frame_drawn_callback(Object *p_where, const StringName &p_method, const Variant &p_userdata) = 0;

	/* EVENT QUEUING */

	enum ChangedPriority {
		CHANGED_PRIORITY_ANY = 0,
		CHANGED_PRIORITY_LOW,
		CHANGED_PRIORITY_HIGH,
	};

	virtual void draw(bool p_swap_buffers = true, double frame_step = 0.0) = 0;
	virtual void sync() = 0;
	virtual bool has_changed(ChangedPriority p_priority = CHANGED_PRIORITY_ANY) const = 0;
	virtual void init() = 0;
	virtual void finish() = 0;
	virtual void tick() = 0;
	virtual void pre_draw(bool p_will_draw) = 0;

	/* STATUS INFORMATION */

	enum RenderInfo {

		INFO_OBJECTS_IN_FRAME,
		INFO_VERTICES_IN_FRAME,
		INFO_MATERIAL_CHANGES_IN_FRAME,
		INFO_SHADER_CHANGES_IN_FRAME,
		INFO_SHADER_COMPILES_IN_FRAME,
		INFO_SURFACE_CHANGES_IN_FRAME,
		INFO_DRAW_CALLS_IN_FRAME,
		INFO_2D_ITEMS_IN_FRAME,
		INFO_2D_DRAW_CALLS_IN_FRAME,
		INFO_USAGE_VIDEO_MEM_TOTAL,
		INFO_VIDEO_MEM_USED,
		INFO_TEXTURE_MEM_USED,
		INFO_VERTEX_MEM_USED,
	};

	virtual uint64_t get_render_info(RenderInfo p_info) = 0;
	virtual String get_video_adapter_name() const = 0;
	virtual String get_video_adapter_vendor() const = 0;

	/* Materials for 2D on 3D */

	/* TESTING */

	virtual RID get_test_cube() = 0;

	virtual RID get_test_texture();
	virtual RID get_white_texture();

	virtual RID make_sphere_mesh(int p_lats, int p_lons, float p_radius);

	virtual void mesh_add_surface_from_mesh_data(RID p_mesh, const Geometry::MeshData &p_mesh_data);
	virtual void mesh_add_surface_from_planes(RID p_mesh, const PoolVector<Plane> &p_planes);

	virtual void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) = 0;
	virtual void set_default_clear_color(const Color &p_color) = 0;
	virtual void set_shader_time_scale(float p_scale) = 0;

	enum Features {
		FEATURE_SHADERS,
		FEATURE_MULTITHREADED,
	};

	virtual bool has_feature(Features p_feature) const = 0;

	virtual bool has_os_feature(const String &p_feature) const = 0;

	virtual void set_debug_generate_wireframes(bool p_generate) = 0;

	virtual void call_set_use_vsync(bool p_enable) = 0;

	virtual bool is_low_end() const = 0;

	bool is_render_loop_enabled() const;
	void set_render_loop_enabled(bool p_enabled);

#ifdef DEBUG_ENABLED
	bool is_force_shader_fallbacks_enabled() const;
	void set_force_shader_fallbacks_enabled(bool p_enabled);
#endif

	RenderingServer();
	virtual ~RenderingServer();
};

// make variant understand the enums
VARIANT_ENUM_CAST(RenderingServer::CubeMapSide);
VARIANT_ENUM_CAST(RenderingServer::TextureFlags);
VARIANT_ENUM_CAST(RenderingServer::ShaderMode);
VARIANT_ENUM_CAST(RenderingServer::ArrayType);
VARIANT_ENUM_CAST(RenderingServer::ArrayFormat);
VARIANT_ENUM_CAST(RenderingServer::PrimitiveType);
VARIANT_ENUM_CAST(RenderingServer::BlendShapeMode);
VARIANT_ENUM_CAST(RenderingServer::ViewportUpdateMode);
VARIANT_ENUM_CAST(RenderingServer::ViewportClearMode);
VARIANT_ENUM_CAST(RenderingServer::ViewportMSAA);
VARIANT_ENUM_CAST(RenderingServer::ViewportUsage);
VARIANT_ENUM_CAST(RenderingServer::ViewportRenderInfo);
VARIANT_ENUM_CAST(RenderingServer::ViewportDebugDraw);
VARIANT_ENUM_CAST(RenderingServer::InstanceType);
VARIANT_ENUM_CAST(RenderingServer::NinePatchAxisMode);
VARIANT_ENUM_CAST(RenderingServer::RenderInfo);
VARIANT_ENUM_CAST(RenderingServer::Features);
VARIANT_ENUM_CAST(RenderingServer::MultimeshTransformFormat);
VARIANT_ENUM_CAST(RenderingServer::MultimeshColorFormat);
VARIANT_ENUM_CAST(RenderingServer::MultimeshCustomDataFormat);
VARIANT_ENUM_CAST(RenderingServer::MultimeshPhysicsInterpolationQuality);
VARIANT_ENUM_CAST(RenderingServer::InstanceFlags);
VARIANT_ENUM_CAST(RenderingServer::TextureType);
VARIANT_ENUM_CAST(RenderingServer::ChangedPriority);

//typedef RenderingServer RS; // makes it easier to use
#define RS RenderingServer

#endif

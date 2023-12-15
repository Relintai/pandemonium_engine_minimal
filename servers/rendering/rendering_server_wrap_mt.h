#ifndef RENDERING_SERVER_WRAP_MT_H
#define RENDERING_SERVER_WRAP_MT_H
/*************************************************************************/
/*  rendering_server_wrap_mt.h                                              */
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

#include "core/containers/command_queue_mt.h"
#include "core/os/safe_refcount.h"
#include "core/os/thread.h"
#include "servers/rendering_server.h"

class RenderingServerWrapMT : public RenderingServer {
	// the real visual server
	mutable RenderingServer *rendering_server;

	mutable CommandQueueMT command_queue;

	static void _thread_callback(void *_instance);
	void thread_loop();

	Thread::ID server_thread;
	SafeFlag exit;
	Thread thread;
	SafeFlag draw_thread_up;
	bool create_thread;

	SafeNumeric<uint64_t> draw_pending;
	void thread_draw(bool p_swap_buffers, double frame_step);
	void thread_flush();

	void thread_exit();

	Mutex alloc_mutex;

	int pool_max_size;

	//#define DEBUG_SYNC

	static RenderingServerWrapMT *singleton_mt;

#ifdef DEBUG_SYNC
#define SYNC_DEBUG print_line("sync on: " + String(__FUNCTION__));
#else
#define SYNC_DEBUG
#endif

public:
#define ServerName RenderingServer
#define ServerNameWrapMT RenderingServerWrapMT
#define server_name rendering_server
#include "servers/server_wrap_mt_common.h"

	/* EVENT QUEUING */
	FUNCRID(texture)
	FUNC7(texture_allocate, RID, int, int, int, Image::Format, TextureType, uint32_t)
	FUNC3(texture_set_data, RID, const Ref<Image> &, int)
	FUNC10(texture_set_data_partial, RID, const Ref<Image> &, int, int, int, int, int, int, int, int)
	FUNC2RC(Ref<Image>, texture_get_data, RID, int)
	FUNC2(texture_set_flags, RID, uint32_t)
	FUNC1RC(uint32_t, texture_get_flags, RID)
	FUNC1RC(Image::Format, texture_get_format, RID)
	FUNC1RC(TextureType, texture_get_type, RID)
	FUNC1RC(uint32_t, texture_get_texid, RID)
	FUNC1RC(uint32_t, texture_get_width, RID)
	FUNC1RC(uint32_t, texture_get_height, RID)
	FUNC1RC(uint32_t, texture_get_depth, RID)
	FUNC4(texture_set_size_override, RID, int, int, int)
	FUNC2(texture_bind, RID, uint32_t)

	FUNC3(texture_set_detect_3d_callback, RID, TextureDetectCallback, void *)
	FUNC3(texture_set_detect_srgb_callback, RID, TextureDetectCallback, void *)
	FUNC3(texture_set_detect_normal_callback, RID, TextureDetectCallback, void *)

	FUNC2(texture_set_path, RID, const String &)
	FUNC1RC(String, texture_get_path, RID)
	FUNC1(texture_set_shrink_all_x2_on_set_data, bool)
	FUNC1S(texture_debug_usage, List<TextureInfo> *)

	FUNC1(textures_keep_original, bool)

	FUNC2(texture_set_proxy, RID, RID)

	FUNC2(texture_set_force_redraw_if_visible, RID, bool)

	/* SHADER API */

	FUNCRID(shader)

	FUNC2(shader_set_code, RID, const String &)
	FUNC1RC(String, shader_get_code, RID)

	FUNC2SC(shader_get_param_list, RID, List<PropertyInfo> *)

	FUNC3(shader_set_default_texture_param, RID, const StringName &, RID)
	FUNC2RC(RID, shader_get_default_texture_param, RID, const StringName &)

	FUNC2(shader_add_custom_define, RID, const String &)
	FUNC2SC(shader_get_custom_defines, RID, Vector<String> *)
	FUNC2(shader_remove_custom_define, RID, const String &)

	FUNC1(set_shader_async_hidden_forbidden, bool)

	/* COMMON MATERIAL API */

	FUNCRID(material)

	FUNC2(material_set_shader, RID, RID)
	FUNC1RC(RID, material_get_shader, RID)

	FUNC3(material_set_param, RID, const StringName &, const Variant &)
	FUNC2RC(Variant, material_get_param, RID, const StringName &)
	FUNC2RC(Variant, material_get_param_default, RID, const StringName &)

	FUNC2(material_set_render_priority, RID, int)
	FUNC2(material_set_line_width, RID, float)
	FUNC2(material_set_next_pass, RID, RID)

	/* MESH API */

	FUNCRID(mesh)

	FUNC10(mesh_add_surface, RID, uint32_t, PrimitiveType, const PoolVector<uint8_t> &, int, const PoolVector<uint8_t> &, int, const AABB &, const Vector<PoolVector<uint8_t>> &, const Vector<AABB> &)

	FUNC2(mesh_set_blend_shape_count, RID, int)
	FUNC1RC(int, mesh_get_blend_shape_count, RID)

	FUNC2(mesh_set_blend_shape_mode, RID, BlendShapeMode)
	FUNC1RC(BlendShapeMode, mesh_get_blend_shape_mode, RID)

	FUNC4(mesh_surface_update_region, RID, int, int, const PoolVector<uint8_t> &)

	FUNC3(mesh_surface_set_material, RID, int, RID)
	FUNC2RC(RID, mesh_surface_get_material, RID, int)

	FUNC2RC(int, mesh_surface_get_array_len, RID, int)
	FUNC2RC(int, mesh_surface_get_array_index_len, RID, int)

	FUNC2RC(PoolVector<uint8_t>, mesh_surface_get_array, RID, int)
	FUNC2RC(PoolVector<uint8_t>, mesh_surface_get_index_array, RID, int)

	FUNC2RC(uint32_t, mesh_surface_get_format, RID, int)
	FUNC2RC(PrimitiveType, mesh_surface_get_primitive_type, RID, int)

	FUNC2RC(AABB, mesh_surface_get_aabb, RID, int)
	FUNC2RC(Vector<PoolVector<uint8_t>>, mesh_surface_get_blend_shapes, RID, int)

	FUNC2(mesh_remove_surface, RID, int)
	FUNC1RC(int, mesh_get_surface_count, RID)

	FUNC2(mesh_set_custom_aabb, RID, const AABB &)
	FUNC1RC(AABB, mesh_get_custom_aabb, RID)

	FUNC1(mesh_clear, RID)

	/* MULTIMESH API */

	FUNCRID(multimesh)

	FUNC5(multimesh_allocate, RID, int, MultimeshTransformFormat, MultimeshColorFormat, MultimeshCustomDataFormat)
	FUNC1RC(int, multimesh_get_instance_count, RID)

	FUNC2(multimesh_set_mesh, RID, RID)
	FUNC3(multimesh_instance_set_transform, RID, int, const Transform &)
	FUNC3(multimesh_instance_set_transform_2d, RID, int, const Transform2D &)
	FUNC3(multimesh_instance_set_color, RID, int, const Color &)
	FUNC3(multimesh_instance_set_custom_data, RID, int, const Color &)

	FUNC1RC(RID, multimesh_get_mesh, RID)
	FUNC1RC(AABB, multimesh_get_aabb, RID)

	FUNC2RC(Transform, multimesh_instance_get_transform, RID, int)
	FUNC2RC(Transform2D, multimesh_instance_get_transform_2d, RID, int)
	FUNC2RC(Color, multimesh_instance_get_color, RID, int)
	FUNC2RC(Color, multimesh_instance_get_custom_data, RID, int)

	FUNC2(multimesh_set_as_bulk_array, RID, const PoolVector<float> &)

	FUNC3(multimesh_set_as_bulk_array_interpolated, RID, const PoolVector<float> &, const PoolVector<float> &)
	FUNC2(multimesh_set_physics_interpolated, RID, bool)
	FUNC2(multimesh_set_physics_interpolation_quality, RID, MultimeshPhysicsInterpolationQuality)
	FUNC2(multimesh_instance_reset_physics_interpolation, RID, int)

	FUNC2(multimesh_set_visible_instances, RID, int)
	FUNC1RC(int, multimesh_get_visible_instances, RID)

	/* VIEWPORT TARGET API */

	FUNCRID(viewport)

	FUNC3(viewport_set_size, RID, int, int)

	FUNC2(viewport_set_active, RID, bool)
	FUNC2(viewport_set_parent_viewport, RID, RID)

	FUNC2(viewport_set_clear_mode, RID, ViewportClearMode)

	FUNC3(viewport_attach_to_screen, RID, const Rect2 &, int)
	FUNC2(viewport_set_render_direct_to_screen, RID, bool)
	FUNC1(viewport_detach, RID)

	FUNC2(viewport_set_update_mode, RID, ViewportUpdateMode)
	FUNC2(viewport_set_vflip, RID, bool)

	FUNC1RC(RID, viewport_get_texture, RID)

	FUNC2(viewport_set_hide_canvas, RID, bool)
	FUNC2(viewport_set_disable_environment, RID, bool)
	FUNC2(viewport_set_disable_3d, RID, bool)
	FUNC2(viewport_set_keep_3d_linear, RID, bool)

	FUNC2(viewport_attach_canvas, RID, RID)

	FUNC2(viewport_remove_canvas, RID, RID)
	FUNC3(viewport_set_canvas_transform, RID, RID, const Transform2D &)
	FUNC2(viewport_set_transparent_background, RID, bool)

	FUNC2(viewport_set_global_canvas_transform, RID, const Transform2D &)
	FUNC4(viewport_set_canvas_stacking, RID, RID, int, int)
	FUNC2(viewport_set_msaa, RID, ViewportMSAA)
	FUNC2(viewport_set_use_fxaa, RID, bool)
	FUNC2(viewport_set_use_debanding, RID, bool)
	FUNC2(viewport_set_sharpen_intensity, RID, float)
	FUNC2(viewport_set_hdr, RID, bool)
	FUNC2(viewport_set_use_32_bpc_depth, RID, bool)
	FUNC2(viewport_set_usage, RID, ViewportUsage)

	//this passes directly to avoid stalling, but it's pretty dangerous, so don't call after freeing a viewport
	virtual int viewport_get_render_info(RID p_viewport, ViewportRenderInfo p_info) {
		return rendering_server->viewport_get_render_info(p_viewport, p_info);
	}

	FUNC2(viewport_set_debug_draw, RID, ViewportDebugDraw)

	/* CANVAS (2D) */

	FUNCRID(canvas)
	FUNC3(canvas_set_item_mirroring, RID, RID, const Point2 &)
	FUNC2(canvas_set_modulate, RID, const Color &)
	FUNC3(canvas_set_parent, RID, RID, float)
	FUNC1(canvas_set_disable_scale, bool)

	FUNCRID(canvas_item)
	FUNC2(canvas_item_set_parent, RID, RID)
	FUNC2(canvas_item_set_name, RID, String)

	FUNC2(canvas_item_set_visible, RID, bool)
	FUNC2(canvas_item_set_light_mask, RID, int)

	FUNC2(canvas_item_set_update_when_visible, RID, bool)

	FUNC2(canvas_item_set_transform, RID, const Transform2D &)
	FUNC2(canvas_item_set_clip, RID, bool)
	FUNC2(canvas_item_set_distance_field_mode, RID, bool)
	FUNC3(canvas_item_set_custom_rect, RID, bool, const Rect2 &)
	FUNC2(canvas_item_set_modulate, RID, const Color &)
	FUNC2(canvas_item_set_self_modulate, RID, const Color &)

	FUNC2(canvas_item_set_draw_behind_parent, RID, bool)
	FUNC2(canvas_item_set_use_identity_transform, RID, bool)

	FUNC6(canvas_item_add_line, RID, const Point2 &, const Point2 &, const Color &, float, bool)
	FUNC5(canvas_item_add_polyline, RID, const Vector<Point2> &, const Vector<Color> &, float, bool)
	FUNC5(canvas_item_add_multiline, RID, const Vector<Point2> &, const Vector<Color> &, float, bool)
	FUNC3(canvas_item_add_rect, RID, const Rect2 &, const Color &)
	FUNC4(canvas_item_add_circle, RID, const Point2 &, float, const Color &)
	FUNC7(canvas_item_add_texture_rect, RID, const Rect2 &, RID, bool, const Color &, bool, RID)
	FUNC8(canvas_item_add_texture_rect_region, RID, const Rect2 &, RID, const Rect2 &, const Color &, bool, RID, bool)
	FUNC7(canvas_item_add_texture_multirect_region, RID, const Vector<Rect2> &, RID, const Vector<Rect2> &, const Color &, uint32_t, RID)
	FUNC11(canvas_item_add_nine_patch, RID, const Rect2 &, const Rect2 &, RID, const Vector2 &, const Vector2 &, NinePatchAxisMode, NinePatchAxisMode, bool, const Color &, RID)
	FUNC7(canvas_item_add_primitive, RID, const Vector<Point2> &, const Vector<Color> &, const Vector<Point2> &, RID, float, RID)
	FUNC7(canvas_item_add_polygon, RID, const Vector<Point2> &, const Vector<Color> &, const Vector<Point2> &, RID, RID, bool)
	FUNC12(canvas_item_add_triangle_array, RID, const Vector<int> &, const Vector<Point2> &, const Vector<Color> &, const Vector<Point2> &, const Vector<int> &, const Vector<float> &, RID, int, RID, bool, bool)
	FUNC6(canvas_item_add_mesh, RID, const RID &, const Transform2D &, const Color &, RID, RID)
	FUNC4(canvas_item_add_multimesh, RID, RID, RID, RID)
	FUNC2(canvas_item_add_set_transform, RID, const Transform2D &)
	FUNC2(canvas_item_add_clip_ignore, RID, bool)
	FUNC2(canvas_item_set_sort_children_by_y, RID, bool)
	FUNC2(canvas_item_set_z_index, RID, int)
	FUNC2(canvas_item_set_z_as_relative_to_parent, RID, bool)
	FUNC3(canvas_item_set_copy_to_backbuffer, RID, bool, const Rect2 &)
	FUNC1(canvas_item_clear, RID)
	FUNC2(canvas_item_set_draw_index, RID, int)
	FUNC2(canvas_item_set_material, RID, RID)
	FUNC2(canvas_item_set_use_parent_material, RID, bool)

	FUNC2(canvas_item_attach_skeleton, RID, RID)
	FUNC2(canvas_item_set_skeleton_relative_xform, RID, Transform2D)
	FUNC1R(Rect2, _debug_canvas_item_get_rect, RID)
	FUNC1R(Rect2, _debug_canvas_item_get_local_bound, RID)

	FUNC2(canvas_item_set_interpolated, RID, bool)
	FUNC1(canvas_item_reset_physics_interpolation, RID)
	FUNC2(canvas_item_transform_physics_interpolation, RID, Transform2D)

	/* BLACK BARS */

	FUNC4(black_bars_set_margins, int, int, int, int)
	FUNC4(black_bars_set_images, RID, RID, RID, RID)

	/* FREE */

	FUNC1(free, RID)

	/* EVENT QUEUING */

	FUNC3(request_frame_drawn_callback, Object *, const StringName &, const Variant &)

	virtual void init();
	virtual void finish();
	virtual void tick();
	virtual void pre_draw(bool p_will_draw);
	virtual void draw(bool p_swap_buffers, double frame_step);
	virtual void sync();
	FUNC1RC(bool, has_changed, ChangedPriority)
	virtual void set_physics_interpolation_enabled(bool p_enabled);

	/* RENDER INFO */

	//this passes directly to avoid stalling
	virtual uint64_t get_render_info(RenderInfo p_info) {
		return rendering_server->get_render_info(p_info);
	}

	virtual String get_video_adapter_name() const {
		return rendering_server->get_video_adapter_name();
	}

	virtual String get_video_adapter_vendor() const {
		return rendering_server->get_video_adapter_vendor();
	}

	FUNC4(set_boot_image, const Ref<Image> &, const Color &, bool, bool)
	FUNC1(set_default_clear_color, const Color &)
	FUNC1(set_shader_time_scale, float)

	FUNC0R(RID, get_test_cube)

	FUNC1(set_debug_generate_wireframes, bool)

	virtual bool has_feature(Features p_feature) const {
		return rendering_server->has_feature(p_feature);
	}
	virtual bool has_os_feature(const String &p_feature) const {
		return rendering_server->has_os_feature(p_feature);
	}

	FUNC1(call_set_use_vsync, bool)

	static void set_use_vsync_callback(bool p_enable);

	virtual bool is_low_end() const {
		return rendering_server->is_low_end();
	}

	RenderingServerWrapMT(RenderingServer *p_contained, bool p_create_thread);
	~RenderingServerWrapMT();

#undef ServerName
#undef ServerNameWrapMT
#undef server_name
};

#ifdef DEBUG_SYNC
#undef DEBUG_SYNC
#endif
#undef SYNC_DEBUG

#endif

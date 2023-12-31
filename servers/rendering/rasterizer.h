#ifndef RASTERIZER_H
#define RASTERIZER_H

/*  rasterizer.h                                                         */


#include "core/math/projection.h"
#include "core/math/transform_interpolator.h"
#include "servers/rendering_server.h"

#include "core/containers/self_list.h"

class RasterizerStorage {
public:
	/* TEXTURE API */

	virtual RID texture_create() = 0;
	virtual void texture_allocate(RID p_texture,
			int p_width,
			int p_height,
			int p_depth_3d,
			Image::Format p_format,
			RS::TextureType p_type,
			uint32_t p_flags = RS::TEXTURE_FLAGS_DEFAULT) = 0;

	virtual void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_level = 0) = 0;

	virtual void texture_set_data_partial(RID p_texture,
			const Ref<Image> &p_image,
			int src_x, int src_y,
			int src_w, int src_h,
			int dst_x, int dst_y,
			int p_dst_mip,
			int p_level = 0) = 0;

	virtual Ref<Image> texture_get_data(RID p_texture, int p_level = 0) const = 0;
	virtual void texture_set_flags(RID p_texture, uint32_t p_flags) = 0;
	virtual uint32_t texture_get_flags(RID p_texture) const = 0;
	virtual Image::Format texture_get_format(RID p_texture) const = 0;
	virtual RS::TextureType texture_get_type(RID p_texture) const = 0;
	virtual uint32_t texture_get_texid(RID p_texture) const = 0;
	virtual uint32_t texture_get_width(RID p_texture) const = 0;
	virtual uint32_t texture_get_height(RID p_texture) const = 0;
	virtual uint32_t texture_get_depth(RID p_texture) const = 0;
	virtual void texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth_3d) = 0;
	virtual void texture_bind(RID p_texture, uint32_t p_texture_no) = 0;

	virtual void texture_set_path(RID p_texture, const String &p_path) = 0;
	virtual String texture_get_path(RID p_texture) const = 0;

	virtual void texture_set_shrink_all_x2_on_set_data(bool p_enable) = 0;

	virtual void texture_debug_usage(List<RS::TextureInfo> *r_info) = 0;

	virtual RID texture_create_radiance_cubemap(RID p_source, int p_resolution = -1) const = 0;

	virtual void texture_set_detect_3d_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) = 0;
	virtual void texture_set_detect_srgb_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) = 0;
	virtual void texture_set_detect_normal_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) = 0;

	virtual void textures_keep_original(bool p_enable) = 0;

	virtual void texture_set_proxy(RID p_proxy, RID p_base) = 0;
	virtual Size2 texture_size_with_proxy(RID p_texture) const = 0;
	virtual void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) = 0;

	/* SHADER API */

	virtual RID shader_create() = 0;

	virtual void shader_set_code(RID p_shader, const String &p_code) = 0;
	virtual String shader_get_code(RID p_shader) const = 0;
	virtual void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const = 0;

	virtual void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) = 0;
	virtual RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const = 0;

	virtual void shader_add_custom_define(RID p_shader, const String &p_define) = 0;
	virtual void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const = 0;
	virtual void shader_remove_custom_define(RID p_shader, const String &p_define) = 0;

	virtual void set_shader_async_hidden_forbidden(bool p_forbidden) = 0;
	virtual bool is_shader_async_hidden_forbidden() = 0;

	/* COMMON MATERIAL API */

	virtual RID material_create() = 0;

	virtual void material_set_render_priority(RID p_material, int priority) = 0;
	virtual void material_set_shader(RID p_shader_material, RID p_shader) = 0;
	virtual RID material_get_shader(RID p_shader_material) const = 0;

	virtual void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) = 0;
	virtual Variant material_get_param(RID p_material, const StringName &p_param) const = 0;
	virtual Variant material_get_param_default(RID p_material, const StringName &p_param) const = 0;

	virtual void material_set_line_width(RID p_material, float p_width) = 0;

	virtual void material_set_next_pass(RID p_material, RID p_next_material) = 0;

	virtual bool material_is_animated(RID p_material) = 0;
	virtual bool material_casts_shadows(RID p_material) = 0;
	virtual bool material_uses_tangents(RID p_material);
	virtual bool material_uses_ensure_correct_normals(RID p_material);

	/* MESH API */

	virtual RID mesh_create() = 0;

	virtual void mesh_add_surface(RID p_mesh, uint32_t p_format, RS::PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes = Vector<PoolVector<uint8_t>>(), const Vector<AABB> &p_bone_aabbs = Vector<AABB>()) = 0;

	virtual void mesh_set_blend_shape_count(RID p_mesh, int p_amount) = 0;
	virtual int mesh_get_blend_shape_count(RID p_mesh) const = 0;

	virtual void mesh_set_blend_shape_mode(RID p_mesh, RS::BlendShapeMode p_mode) = 0;
	virtual RS::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const = 0;

	virtual void mesh_set_blend_shape_values(RID p_mesh, PoolVector<float> p_values) = 0;
	virtual PoolVector<float> mesh_get_blend_shape_values(RID p_mesh) const = 0;

	virtual void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data) = 0;

	virtual void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) = 0;
	virtual RID mesh_surface_get_material(RID p_mesh, int p_surface) const = 0;

	virtual int mesh_surface_get_array_len(RID p_mesh, int p_surface) const = 0;
	virtual int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const = 0;

	virtual PoolVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const = 0;
	virtual PoolVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const = 0;

	virtual uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const = 0;
	virtual RS::PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const = 0;

	virtual AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const = 0;
	virtual Vector<PoolVector<uint8_t>> mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const = 0;

	virtual void mesh_remove_surface(RID p_mesh, int p_index) = 0;
	virtual int mesh_get_surface_count(RID p_mesh) const = 0;

	virtual void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) = 0;
	virtual AABB mesh_get_custom_aabb(RID p_mesh) const = 0;

	virtual AABB mesh_get_aabb(RID p_mesh) const = 0;

	virtual void mesh_clear(RID p_mesh) = 0;

	/* MULTIMESH API */
	struct MMInterpolator {
		RS::MultimeshTransformFormat _transform_format = RS::MULTIMESH_TRANSFORM_3D;
		RS::MultimeshColorFormat _color_format = RS::MULTIMESH_COLOR_NONE;
		RS::MultimeshCustomDataFormat _data_format = RS::MULTIMESH_CUSTOM_DATA_NONE;

		// in floats
		int _stride = 0;

		// Vertex format sizes in floats
		int _vf_size_xform = 0;
		int _vf_size_color = 0;
		int _vf_size_data = 0;

		// Set by allocate, can be used to prevent indexing out of range.
		int _num_instances = 0;

		// Quality determines whether to use lerp or slerp etc.
		int quality = 0;
		bool interpolated = false;
		bool on_interpolate_update_list = false;
		bool on_transform_update_list = false;

		PoolVector<float> _data_prev;
		PoolVector<float> _data_curr;
		PoolVector<float> _data_interpolated;
	};

	virtual RID multimesh_create();
	virtual void multimesh_allocate(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, RS::MultimeshColorFormat p_color_format, RS::MultimeshCustomDataFormat p_data = RS::MULTIMESH_CUSTOM_DATA_NONE);
	virtual int multimesh_get_instance_count(RID p_multimesh) const;
	virtual void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
	virtual void multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform);
	virtual void multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform);
	virtual void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color);
	virtual void multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color);
	virtual RID multimesh_get_mesh(RID p_multimesh) const;
	virtual Transform multimesh_instance_get_transform(RID p_multimesh, int p_index) const;
	virtual Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const;
	virtual Color multimesh_instance_get_color(RID p_multimesh, int p_index) const;
	virtual Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const;
	virtual void multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array);

	virtual void multimesh_set_as_bulk_array_interpolated(RID p_multimesh, const PoolVector<float> &p_array, const PoolVector<float> &p_array_prev);
	virtual void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
	virtual void multimesh_set_physics_interpolation_quality(RID p_multimesh, RS::MultimeshPhysicsInterpolationQuality p_quality);
	virtual void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);

	virtual void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	virtual int multimesh_get_visible_instances(RID p_multimesh) const;
	virtual AABB multimesh_get_aabb(RID p_multimesh) const;
	virtual void multimesh_attach_canvas_item(RID p_multimesh, RID p_canvas_item, bool p_attach) = 0;

	virtual RID _multimesh_create() = 0;
	virtual void _multimesh_allocate(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, RS::MultimeshColorFormat p_color_format, RS::MultimeshCustomDataFormat p_data = RS::MULTIMESH_CUSTOM_DATA_NONE) = 0;
	virtual int _multimesh_get_instance_count(RID p_multimesh) const = 0;
	virtual void _multimesh_set_mesh(RID p_multimesh, RID p_mesh) = 0;
	virtual void _multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) = 0;
	virtual void _multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) = 0;
	virtual void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) = 0;
	virtual void _multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_color) = 0;
	virtual RID _multimesh_get_mesh(RID p_multimesh) const = 0;
	virtual Transform _multimesh_instance_get_transform(RID p_multimesh, int p_index) const = 0;
	virtual Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const = 0;
	virtual Color _multimesh_instance_get_color(RID p_multimesh, int p_index) const = 0;
	virtual Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const = 0;
	virtual void _multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array) = 0;
	virtual void _multimesh_set_visible_instances(RID p_multimesh, int p_visible) = 0;
	virtual int _multimesh_get_visible_instances(RID p_multimesh) const = 0;
	virtual AABB _multimesh_get_aabb(RID p_multimesh) const = 0;

	// Multimesh is responsible for allocating / destroying an MMInterpolator object.
	// This allows shared functionality for interpolation across backends.
	virtual MMInterpolator *_multimesh_get_interpolator(RID p_multimesh) const = 0;

private:
	void _multimesh_add_to_interpolation_lists(RID p_multimesh, MMInterpolator &r_mmi);

public:
	/* RENDER TARGET */

	enum RenderTargetFlags {
		RENDER_TARGET_VFLIP,
		RENDER_TARGET_TRANSPARENT,
		RENDER_TARGET_NO_3D_EFFECTS,
		RENDER_TARGET_NO_3D,
		RENDER_TARGET_NO_SAMPLING,
		RENDER_TARGET_HDR,
		RENDER_TARGET_KEEP_3D_LINEAR,
		RENDER_TARGET_DIRECT_TO_SCREEN,
		RENDER_TARGET_USE_32_BPC_DEPTH,
		RENDER_TARGET_FLAG_MAX
	};

	virtual RID render_target_create() = 0;
	virtual void render_target_set_position(RID p_render_target, int p_x, int p_y) = 0;
	virtual void render_target_set_size(RID p_render_target, int p_width, int p_height) = 0;
	virtual RID render_target_get_texture(RID p_render_target) const = 0;
	virtual uint32_t render_target_get_depth_texture_id(RID p_render_target) const = 0;
	virtual void render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id, unsigned int p_depth_id) = 0;
	virtual void render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) = 0;
	virtual bool render_target_was_used(RID p_render_target) = 0;
	virtual void render_target_clear_used(RID p_render_target) = 0;
	virtual void render_target_set_msaa(RID p_render_target, RS::ViewportMSAA p_msaa) = 0;
	virtual void render_target_set_use_fxaa(RID p_render_target, bool p_fxaa) = 0;
	virtual void render_target_set_use_debanding(RID p_render_target, bool p_debanding) = 0;
	virtual void render_target_set_sharpen_intensity(RID p_render_target, float p_intensity) = 0;

	/* INTERPOLATION */
	struct InterpolationData {
		void notify_free_multimesh(RID p_rid);
		LocalVector<RID> multimesh_interpolate_update_list;
		LocalVector<RID> multimesh_transform_update_lists[2];
		LocalVector<RID> *multimesh_transform_update_list_curr = &multimesh_transform_update_lists[0];
		LocalVector<RID> *multimesh_transform_update_list_prev = &multimesh_transform_update_lists[1];
	} _interpolation_data;

	void update_interpolation_tick(bool p_process = true);
	void update_interpolation_frame(bool p_process = true);

private:
	_FORCE_INLINE_ void _interpolate_RGBA8(const uint8_t *p_a, const uint8_t *p_b, uint8_t *r_dest, float p_f) const;

public:
	virtual RS::InstanceType get_base_type(RID p_rid) const = 0;
	virtual bool free(RID p_rid) = 0;

	virtual bool has_os_feature(const String &p_feature) const = 0;

	virtual void update_dirty_resources() = 0;

	virtual void set_debug_generate_wireframes(bool p_generate) = 0;

	virtual void render_info_begin_capture() = 0;
	virtual void render_info_end_capture() = 0;
	virtual int get_captured_render_info(RS::RenderInfo p_info) = 0;

	virtual uint64_t get_render_info(RS::RenderInfo p_info) = 0;
	virtual String get_video_adapter_name() const = 0;
	virtual String get_video_adapter_vendor() const = 0;

	static RasterizerStorage *base_singleton;
	RasterizerStorage();
	virtual ~RasterizerStorage() {}
};

class RasterizerCanvas {
public:
	enum CanvasRectFlags {

		CANVAS_RECT_REGION = 1,
		CANVAS_RECT_TILE = 2,
		CANVAS_RECT_FLIP_H = 4,
		CANVAS_RECT_FLIP_V = 8,
		CANVAS_RECT_TRANSPOSE = 16,
		CANVAS_RECT_CLIP_UV = 32
	};

	struct Item : public RID_Data {
		struct Command {
			enum Type {

				TYPE_LINE,
				TYPE_POLYLINE,
				TYPE_RECT,
				TYPE_NINEPATCH,
				TYPE_PRIMITIVE,
				TYPE_POLYGON,
				TYPE_MESH,
				TYPE_MULTIMESH,
				TYPE_CIRCLE,
				TYPE_TRANSFORM,
				TYPE_CLIP_IGNORE,
				TYPE_MULTIRECT,
			};

			virtual bool contains_reference(const RID &p_rid) const { return false; }

			Type type;
			virtual ~Command() {}
		};

		struct CommandLine : public Command {
			Point2 from, to;
			Color color;
			float width;
			bool antialiased;
			CommandLine() { type = TYPE_LINE; }
		};
		struct CommandPolyLine : public Command {
			bool antialiased;
			bool multiline;
			Vector<Point2> triangles;
			Vector<Color> triangle_colors;
			Vector<Point2> lines;
			Vector<Color> line_colors;
			CommandPolyLine() {
				type = TYPE_POLYLINE;
				antialiased = false;
				multiline = false;
			}
		};

		struct CommandRect : public Command {
			Rect2 rect;
			RID texture;
			RID normal_map;
			Color modulate;
			Rect2 source;
			uint8_t flags;

			CommandRect() {
				flags = 0;
				type = TYPE_RECT;
			}
		};

		struct CommandMultiRect : public Command {
			RID texture;
			RID normal_map;
			Color modulate;
			Vector<Rect2> rects;
			Vector<Rect2> sources;
			uint8_t flags;

			CommandMultiRect() {
				flags = 0;
				type = TYPE_MULTIRECT;
			}
		};

		struct CommandNinePatch : public Command {
			Rect2 rect;
			Rect2 source;
			RID texture;
			RID normal_map;
			float margin[4];
			bool draw_center;
			Color color;
			RS::NinePatchAxisMode axis_x;
			RS::NinePatchAxisMode axis_y;
			CommandNinePatch() {
				draw_center = true;
				type = TYPE_NINEPATCH;
			}
		};

		struct CommandPrimitive : public Command {
			Vector<Point2> points;
			Vector<Point2> uvs;
			Vector<Color> colors;
			RID texture;
			RID normal_map;
			float width;

			CommandPrimitive() {
				type = TYPE_PRIMITIVE;
				width = 1;
			}
		};

		struct CommandPolygon : public Command {
			Vector<int> indices;
			Vector<Point2> points;
			Vector<Point2> uvs;
			Vector<Color> colors;
			Vector<int> bones;
			Vector<float> weights;
			RID texture;
			RID normal_map;
			int count;
			bool antialiased;
			bool antialiasing_use_indices;

			struct SkinningData {
				bool dirty = true;
				LocalVector<Rect2> active_bounds;
				LocalVector<uint16_t> active_bone_ids;
				Rect2 untransformed_bound;
			};

			mutable SkinningData *skinning_data;

			CommandPolygon() {
				type = TYPE_POLYGON;
				count = 0;
				skinning_data = NULL;
			}

			virtual ~CommandPolygon() {
				if (skinning_data) {
					memdelete(skinning_data);
					skinning_data = NULL;
				}
			}
		};

		struct CommandMesh : public Command {
			RID mesh;
			RID texture;
			RID normal_map;
			Transform2D transform;
			Color modulate;
			CommandMesh() { type = TYPE_MESH; }
		};

		struct CommandMultiMesh : public Command {
			RID multimesh;
			RID texture;
			RID normal_map;
			RID canvas_item;
			virtual bool contains_reference(const RID &p_rid) const { return multimesh == p_rid; }

			CommandMultiMesh() { type = TYPE_MULTIMESH; }

			virtual ~CommandMultiMesh() {
				// Remove any backlinks from multimesh to canvas item.
				if (multimesh.is_valid()) {
					RasterizerStorage::base_singleton->multimesh_attach_canvas_item(multimesh, canvas_item, false);
				}
			}
		};

		struct CommandCircle : public Command {
			Point2 pos;
			float radius;
			Color color;
			CommandCircle() { type = TYPE_CIRCLE; }
		};

		struct CommandTransform : public Command {
			Transform2D xform;
			CommandTransform() { type = TYPE_TRANSFORM; }
		};

		struct CommandClipIgnore : public Command {
			bool ignore;
			CommandClipIgnore() {
				type = TYPE_CLIP_IGNORE;
				ignore = false;
			}
		};

		struct ViewportRender {
			RenderingServer *owner;
			void *udata;
			Rect2 rect;
		};

		// For interpolation we store the current local xform,
		// and the previous xform from the previous tick.
		Transform2D xform_curr;
		Transform2D xform_prev;

		bool clip : 1;
		bool visible : 1;
		bool behind : 1;
		bool update_when_visible : 1;
		bool distance_field : 1;
		bool light_masked : 1;
		bool on_interpolate_transform_list : 1;
		bool interpolated : 1;
		bool use_identity_xform : 1;
		mutable bool custom_rect : 1;
		mutable bool rect_dirty : 1;
		mutable bool bound_dirty : 1;

		Vector<Command *> commands;
		mutable Rect2 rect;
		RID material;

		//RS::MaterialBlendMode blend_mode;
		int32_t light_mask;
		mutable uint32_t skeleton_revision;

		Item *next;

		struct SkinningData {
			Transform2D skeleton_relative_xform;
			Transform2D skeleton_relative_xform_inv;
		};
		SkinningData *skinning_data;

		struct CopyBackBuffer {
			Rect2 rect;
			Rect2 screen_rect;
			bool full;
		};
		CopyBackBuffer *copy_back_buffer;

		Color final_modulate;
		Transform2D final_transform;
		Rect2 final_clip_rect;
		Item *final_clip_owner;
		Item *material_owner;
		ViewportRender *vp_render;

		Rect2 global_rect_cache;

	private:
		Rect2 calculate_polygon_bounds(const Item::CommandPolygon &p_polygon) const;

	public:
		// the rect containing this item and all children,
		// in local space.
		Rect2 local_bound;

		// When using interpolation, the local bound for culling
		// should be a combined bound of the previous and current.
		// To keep this up to date, we need to keep track of the previous
		// bound separately rather than just the combined bound.
		Rect2 local_bound_prev;
		uint32_t local_bound_last_update_tick;

		const Rect2 &get_rect() const {
			if (custom_rect) {
				return rect;
			}

			if (!rect_dirty && !update_when_visible) {
				return rect;
			}

			//must update rect
			int s = commands.size();
			if (s == 0) {
				rect = Rect2();
				rect_dirty = false;
				return rect;
			}

			Transform2D xf;
			bool found_xform = false;
			bool first = true;

			const Item::Command *const *cmd = &commands[0];

			for (int i = 0; i < s; i++) {
				const Item::Command *c = cmd[i];
				Rect2 r;

				switch (c->type) {
					case Item::Command::TYPE_LINE: {
						const Item::CommandLine *line = static_cast<const Item::CommandLine *>(c);
						r.position = line->from;
						r.expand_to(line->to);
					} break;
					case Item::Command::TYPE_POLYLINE: {
						const Item::CommandPolyLine *pline = static_cast<const Item::CommandPolyLine *>(c);
						if (pline->triangles.size()) {
							for (int j = 0; j < pline->triangles.size(); j++) {
								if (j == 0) {
									r.position = pline->triangles[j];
								} else {
									r.expand_to(pline->triangles[j]);
								}
							}
						} else {
							for (int j = 0; j < pline->lines.size(); j++) {
								if (j == 0) {
									r.position = pline->lines[j];
								} else {
									r.expand_to(pline->lines[j]);
								}
							}
						}

					} break;
					case Item::Command::TYPE_RECT: {
						const Item::CommandRect *crect = static_cast<const Item::CommandRect *>(c);
						r = crect->rect;

					} break;
					case Item::Command::TYPE_MULTIRECT: {
						const Item::CommandMultiRect *mrect = static_cast<const Item::CommandMultiRect *>(c);
						int num_rects = mrect->rects.size();
						if (num_rects) {
							r = mrect->rects[0];
							for (int n = 1; n < num_rects; n++) {
								r = mrect->rects[n].merge(r);
							}
						}
					} break;
					case Item::Command::TYPE_NINEPATCH: {
						const Item::CommandNinePatch *style = static_cast<const Item::CommandNinePatch *>(c);
						r = style->rect;
					} break;
					case Item::Command::TYPE_PRIMITIVE: {
						const Item::CommandPrimitive *primitive = static_cast<const Item::CommandPrimitive *>(c);
						r.position = primitive->points[0];
						for (int j = 1; j < primitive->points.size(); j++) {
							r.expand_to(primitive->points[j]);
						}
					} break;
					case Item::Command::TYPE_POLYGON: {
						const Item::CommandPolygon *polygon = static_cast<const Item::CommandPolygon *>(c);
						DEV_ASSERT(polygon);
						r = calculate_polygon_bounds(*polygon);
					} break;
					case Item::Command::TYPE_MESH: {
						const Item::CommandMesh *mesh = static_cast<const Item::CommandMesh *>(c);
						AABB aabb = RasterizerStorage::base_singleton->mesh_get_aabb(mesh->mesh);

						r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

					} break;
					case Item::Command::TYPE_MULTIMESH: {
						const Item::CommandMultiMesh *multimesh = static_cast<const Item::CommandMultiMesh *>(c);
						AABB aabb = RasterizerStorage::base_singleton->multimesh_get_aabb(multimesh->multimesh);

						r = Rect2(aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);

					} break;
					case Item::Command::TYPE_CIRCLE: {
						const Item::CommandCircle *circle = static_cast<const Item::CommandCircle *>(c);
						r.position = Point2(-circle->radius, -circle->radius) + circle->pos;
						r.size = Point2(circle->radius * 2.0, circle->radius * 2.0);
					} break;
					case Item::Command::TYPE_TRANSFORM: {
						const Item::CommandTransform *transform = static_cast<const Item::CommandTransform *>(c);
						xf = transform->xform;
						found_xform = true;
						continue;
					} break;

					case Item::Command::TYPE_CLIP_IGNORE: {
					} break;
				}

				if (found_xform) {
					r = xf.xform(r);
				}

				if (first) {
					rect = r;
					first = false;
				} else {
					rect = rect.merge(r);
				}
			}

			rect_dirty = false;
			return rect;
		}

		void remove_references(const RID &p_rid) {
			for (int i = commands.size() - 1; i >= 0; i--) {
				if (commands[i]->contains_reference(p_rid)) {
					memdelete(commands[i]);

					// This could possibly be unordered if occurring close
					// to canvas_item deletion, but is
					// unlikely to make much performance difference,
					// and is safer.
					commands.remove(i);
				}
			}
		}

		void clear() {
			for (int i = 0; i < commands.size(); i++) {
				memdelete(commands[i]);
			}
			commands.clear();
			clip = false;
			rect_dirty = true;
			final_clip_owner = nullptr;
			material_owner = nullptr;
			light_masked = false;

			if (skinning_data) {
				memdelete(skinning_data);
				skinning_data = NULL;
			}

			on_interpolate_transform_list = false;
		}

		Item() {
			light_mask = 1;
			skeleton_revision = 0;
			vp_render = nullptr;
			next = nullptr;
			skinning_data = NULL;
			final_clip_owner = nullptr;
			clip = false;
			final_modulate = Color(1, 1, 1, 1);
			visible = true;
			rect_dirty = true;
			bound_dirty = true;
			custom_rect = false;
			behind = false;
			material_owner = nullptr;
			copy_back_buffer = nullptr;
			distance_field = false;
			light_masked = false;
			update_when_visible = false;
			on_interpolate_transform_list = false;
			interpolated = true;
			use_identity_xform = false;
			local_bound_last_update_tick = 0;
		}

		virtual ~Item() {
			clear();
			if (copy_back_buffer) {
				memdelete(copy_back_buffer);
			}
		}
	};

	virtual void canvas_begin() = 0;
	virtual void canvas_end() = 0;

	virtual void canvas_render_items_begin(const Color &p_modulate, const Transform2D &p_base_transform) {}
	virtual void canvas_render_items_end() {}
	virtual void canvas_render_items(Item *p_item_list, int p_z, const Color &p_modulate, const Transform2D &p_base_transform) = 0;

	virtual void reset_canvas() = 0;

	virtual void draw_window_margins(int *p_margins, RID *p_margin_textures) = 0;

	virtual ~RasterizerCanvas() {}
};

class Rasterizer {
protected:
	static Rasterizer *(*_create_func)();

public:
	static Rasterizer *create();

	virtual RasterizerStorage *get_storage() = 0;
	virtual RasterizerCanvas *get_canvas() = 0;

	virtual void set_boot_image(const Ref<Image> &p_image, const Color &p_color, bool p_scale, bool p_use_filter = true) = 0;
	virtual void set_shader_time_scale(float p_scale) = 0;

	virtual void initialize() = 0;
	virtual void begin_frame(double frame_step) = 0;
	virtual void set_current_render_target(RID p_render_target) = 0;
	virtual void restore_render_target(bool p_3d) = 0;
	virtual void clear_render_target(const Color &p_color) = 0;
	virtual void blit_render_target_to_screen(RID p_render_target, const Rect2 &p_screen_rect, int p_screen = 0) = 0;
	virtual void output_lens_distorted_to_screen(RID p_render_target, const Rect2 &p_screen_rect, float p_k1, float p_k2, const Vector2 &p_eye_center, float p_oversample) = 0;
	virtual void end_frame(bool p_swap_buffers) = 0;
	virtual void finalize() = 0;

	virtual bool is_low_end() const = 0;

	virtual ~Rasterizer() {}
};

// Use float rather than real_t as cheaper and no need for 64 bit.
_FORCE_INLINE_ void RasterizerStorage::_interpolate_RGBA8(const uint8_t *p_a, const uint8_t *p_b, uint8_t *r_dest, float p_f) const {
	// Todo, jiggle these values and test for correctness.
	// Integer interpolation is finicky.. :)
	p_f *= 256.0f;
	int32_t mult = CLAMP(int32_t(p_f), 0, 255);

	for (int n = 0; n < 4; n++) {
		int32_t a = p_a[n];
		int32_t b = p_b[n];

		int32_t diff = b - a;

		diff *= mult;
		diff /= 255;

		int32_t res = a + diff;

		// may not be needed
		res = CLAMP(res, 0, 255);
		r_dest[n] = res;
	}
}

#endif // RASTERIZER_H

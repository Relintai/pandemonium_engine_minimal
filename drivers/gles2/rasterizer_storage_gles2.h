#ifndef RASTERIZERSTORAGEGLES2_H
#define RASTERIZERSTORAGEGLES2_H

/*  rasterizer_storage_gles2.h                                           */


#include "core/containers/bitfield_dynamic.h"
#include "core/containers/pool_vector.h"
#include "core/containers/self_list.h"
#include "drivers/gles_common/rasterizer_asserts.h"
#include "servers/rendering/rasterizer.h"
#include "servers/rendering/shader_language.h"
#include "shader_compiler_gles2.h"
#include "shader_gles2.h"

#include "shaders/copy.glsl.gen.h"

class RasterizerCanvasGLES2;

#define WRAPPED_GL_ACTIVE_TEXTURE storage->gl_wrapper.gl_active_texture
class RasterizerStorageGLES2 : public RasterizerStorage {
public:
	RasterizerCanvasGLES2 *canvas;

	static GLuint system_fbo;

	struct Config {
		bool shrink_textures_x2;
		bool use_fast_texture_filter;
		bool use_anisotropic_filter;
		bool use_physical_light_attenuation;

		int max_vertex_texture_image_units;
		int max_texture_image_units;
		int max_texture_size;
		int max_cubemap_texture_size;
		int max_viewport_dimensions[2];

		// TODO implement wireframe in GLES2
		// bool generate_wireframes;

		RBSet<String> extensions;

		bool float_texture_supported;
		bool s3tc_supported;
		bool etc1_supported;
		bool pvrtc_supported;
		bool rgtc_supported;
		bool bptc_supported;

		bool keep_original_textures;

		bool force_vertex_shading;

		float anisotropic_level;

		bool support_32_bits_indices;
		bool support_write_depth;
		bool support_half_float_vertices;
		bool support_npot_repeat_mipmap;
		bool support_depth_texture;
		bool support_depth_cubemaps;

		bool support_shadow_cubemaps;

		bool multisample_supported;
		bool render_to_mipmap_supported;

		GLuint depth_internalformat;
		GLuint depth_type;
		GLuint depth_buffer_internalformat;

		// in some cases the legacy render didn't orphan. We will mark these
		// so the user can switch orphaning off for them.
		bool should_orphan;
	} config;

	struct Resources {
		GLuint white_tex;
		GLuint black_tex;
		GLuint transparent_tex;
		GLuint normal_tex;
		GLuint aniso_tex;

		GLuint mipmap_blur_fbo;
		GLuint mipmap_blur_color;

		GLuint radical_inverse_vdc_cache_tex;
		
		GLuint quadie;

		size_t blend_shape_transform_cpu_buffer_size;
		PoolVector<float> blend_shape_transform_cpu_buffer;
	} resources;

	mutable struct Shaders {
		ShaderCompilerGLES2 compiler;

		CopyShaderGLES2 copy;

		ShaderCompilerGLES2::IdentifierActions actions_canvas;
		ShaderCompilerGLES2::IdentifierActions actions_scene;
		ShaderCompilerGLES2::IdentifierActions actions_particles;

	} shaders;

	struct Info {
		uint64_t texture_mem;
		uint64_t vertex_mem;

		struct Render {
			uint32_t object_count;
			uint32_t draw_call_count;
			uint32_t material_switch_count;
			uint32_t surface_switch_count;
			uint32_t shader_rebind_count;
			uint32_t vertices_count;
			uint32_t _2d_item_count;
			uint32_t _2d_draw_call_count;

			void reset() {
				object_count = 0;
				draw_call_count = 0;
				material_switch_count = 0;
				surface_switch_count = 0;
				shader_rebind_count = 0;
				vertices_count = 0;
				_2d_item_count = 0;
				_2d_draw_call_count = 0;
			}
		} render, render_final, snap;

		Info() :
				texture_mem(0),
				vertex_mem(0) {
			render.reset();
			render_final.reset();
		}

	} info;

	void bind_quad_array() const;

	/////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////DATA///////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////

	struct Instantiable : public RID_Data {
		_FORCE_INLINE_ void instance_change_notify(bool p_aabb, bool p_materials) {
		}

		_FORCE_INLINE_ void instance_remove_deps() {
		}

		Instantiable() {}

		virtual ~Instantiable() {}
	};

	struct GeometryOwner : public Instantiable {
	};

	struct Geometry : public Instantiable {
		enum Type {
			GEOMETRY_INVALID,
			GEOMETRY_SURFACE,
			GEOMETRY_IMMEDIATE,
			GEOMETRY_MULTISURFACE
		};

		Type type;
		RID material;
		uint64_t last_pass;
		uint32_t index;

		virtual void material_changed_notify() {}

		Geometry() {
			last_pass = 0;
			index = 0;
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////API////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////

	/* TEXTURE API */

	struct RenderTarget;

	struct Texture : RID_Data {
		Texture *proxy;
		RBSet<Texture *> proxy_owners;

		String path;
		uint32_t flags;
		int width, height, depth;
		int alloc_width, alloc_height;
		Image::Format format;
		RS::TextureType type;

		GLenum target;
		GLenum gl_format_cache;
		GLenum gl_internal_format_cache;
		GLenum gl_type_cache;

		int data_size;
		int total_data_size;
		bool ignore_mipmaps;

		bool compressed;

		bool srgb;

		int mipmaps;

		bool resize_to_po2;

		bool active;
		GLenum tex_id;

		uint16_t stored_cube_sides;

		RenderTarget *render_target;

		Vector<Ref<Image>> images;

		bool redraw_if_visible;

		RenderingServer::TextureDetectCallback detect_3d;
		void *detect_3d_ud;

		RenderingServer::TextureDetectCallback detect_srgb;
		void *detect_srgb_ud;

		RenderingServer::TextureDetectCallback detect_normal;
		void *detect_normal_ud;

		Texture() :
				proxy(nullptr),
				flags(0),
				width(0),
				height(0),
				alloc_width(0),
				alloc_height(0),
				format(Image::FORMAT_L8),
				type(RS::TEXTURE_TYPE_2D),
				target(0),
				data_size(0),
				total_data_size(0),
				ignore_mipmaps(false),
				compressed(false),
				mipmaps(0),
				resize_to_po2(false),
				active(false),
				tex_id(0),
				stored_cube_sides(0),
				render_target(nullptr),
				redraw_if_visible(false),
				detect_3d(nullptr),
				detect_3d_ud(nullptr),
				detect_srgb(nullptr),
				detect_srgb_ud(nullptr),
				detect_normal(nullptr),
				detect_normal_ud(nullptr) {
		}

		_ALWAYS_INLINE_ Texture *get_ptr() {
			if (proxy) {
				return proxy; //->get_ptr(); only one level of indirection, else not inlining possible.
			} else {
				return this;
			}
		}

		~Texture() {
			if (tex_id != 0) {
				glDeleteTextures(1, &tex_id);
			}

			for (RBSet<Texture *>::Element *E = proxy_owners.front(); E; E = E->next()) {
				E->get()->proxy = nullptr;
			}

			if (proxy) {
				proxy->proxy_owners.erase(this);
			}
		}
	};

	mutable RID_Owner<Texture> texture_owner;

	Ref<Image> _get_gl_image_and_format(const Ref<Image> &p_image, Image::Format p_format, uint32_t p_flags, Image::Format &r_real_format, GLenum &r_gl_format, GLenum &r_gl_internal_format, GLenum &r_gl_type, bool &r_compressed, bool p_force_decompress) const;

	virtual RID texture_create();
	virtual void texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, RS::TextureType p_type, uint32_t p_flags = RS::TEXTURE_FLAGS_DEFAULT);
	virtual void texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_layer = 0);
	virtual void texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_layer = 0);
	virtual Ref<Image> texture_get_data(RID p_texture, int p_layer = 0) const;
	virtual void texture_set_flags(RID p_texture, uint32_t p_flags);
	virtual uint32_t texture_get_flags(RID p_texture) const;
	virtual Image::Format texture_get_format(RID p_texture) const;
	virtual RS::TextureType texture_get_type(RID p_texture) const;
	virtual uint32_t texture_get_texid(RID p_texture) const;
	virtual uint32_t texture_get_width(RID p_texture) const;
	virtual uint32_t texture_get_height(RID p_texture) const;
	virtual uint32_t texture_get_depth(RID p_texture) const;
	virtual void texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth);
	virtual void texture_bind(RID p_texture, uint32_t p_texture_no);

	virtual void texture_set_path(RID p_texture, const String &p_path);
	virtual String texture_get_path(RID p_texture) const;

	virtual void texture_set_shrink_all_x2_on_set_data(bool p_enable);

	virtual void texture_debug_usage(List<RS::TextureInfo> *r_info);

	virtual RID texture_create_radiance_cubemap(RID p_source, int p_resolution = -1) const;

	virtual void textures_keep_original(bool p_enable);

	virtual void texture_set_proxy(RID p_texture, RID p_proxy);
	virtual Size2 texture_size_with_proxy(RID p_texture) const;

	virtual void texture_set_detect_3d_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata);
	virtual void texture_set_detect_srgb_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata);
	virtual void texture_set_detect_normal_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata);

	virtual void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	/* SHADER API */

	struct Material;

	struct Shader : public RID_Data {
		RID self;

		RS::ShaderMode mode;
		ShaderGLES2 *shader;
		String code;
		SelfList<Material>::List materials;

		RBMap<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;

		uint32_t texture_count;

		uint32_t custom_code_id;
		uint32_t version;

		SelfList<Shader> dirty_list;

		RBMap<StringName, RID> default_textures;

		Vector<ShaderLanguage::ShaderNode::Uniform::Hint> texture_hints;

		bool valid;

		String path;

		uint32_t index;
		uint64_t last_pass;

		struct CanvasItem {
			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
				BLEND_MODE_PMALPHA,
			};

			int blend_mode;

			enum LightMode {
				LIGHT_MODE_NORMAL,
				LIGHT_MODE_UNSHADED,
				LIGHT_MODE_LIGHT_ONLY
			};

			int light_mode;

			// these flags are specifically for batching
			// some of the logic is thus in rasterizer_storage.cpp
			// we could alternatively set bitflags for each 'uses' and test on the fly
			// defined in RasterizerStorageCommon::BatchFlags
			unsigned int batch_flags;

			bool uses_screen_texture;
			bool uses_screen_uv;
			bool uses_time;
			bool uses_modulate;
			bool uses_color;
			bool uses_vertex;

			// all these should disable item joining if used in a custom shader
			bool uses_world_matrix;
			bool uses_extra_matrix;
			bool uses_projection_matrix;
			bool uses_instance_custom;

		} canvas_item;

		struct Spatial {
			enum BlendMode {
				BLEND_MODE_MIX,
				BLEND_MODE_ADD,
				BLEND_MODE_SUB,
				BLEND_MODE_MUL,
			};

			int blend_mode;

			enum DepthDrawMode {
				DEPTH_DRAW_OPAQUE,
				DEPTH_DRAW_ALWAYS,
				DEPTH_DRAW_NEVER,
				DEPTH_DRAW_ALPHA_PREPASS,
			};

			int depth_draw_mode;

			enum CullMode {
				CULL_MODE_FRONT,
				CULL_MODE_BACK,
				CULL_MODE_DISABLED,
			};

			int cull_mode;

			bool uses_alpha;
			bool uses_alpha_scissor;
			bool unshaded;
			bool no_depth_test;
			bool uses_vertex;
			bool uses_discard;
			bool uses_sss;
			bool uses_screen_texture;
			bool uses_depth_texture;
			bool uses_time;
			bool uses_tangent;
			bool uses_ensure_correct_normals;
			bool writes_modelview_or_projection;
			bool uses_vertex_lighting;
			bool uses_world_coordinates;

		} spatial;

		struct Particles {
		} particles;

		bool uses_vertex_time;
		bool uses_fragment_time;

		Shader() :
				dirty_list(this) {
			shader = nullptr;
			valid = false;
			custom_code_id = 0;
			version = 1;
			last_pass = 0;
		}
	};

	mutable RID_Owner<Shader> shader_owner;
	mutable SelfList<Shader>::List _shader_dirty_list;

	void _shader_make_dirty(Shader *p_shader);

	virtual RID shader_create();

	virtual void shader_set_code(RID p_shader, const String &p_code);
	virtual String shader_get_code(RID p_shader) const;
	virtual void shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const;

	virtual void shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture);
	virtual RID shader_get_default_texture_param(RID p_shader, const StringName &p_name) const;

	virtual void shader_add_custom_define(RID p_shader, const String &p_define);
	virtual void shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const;
	virtual void shader_remove_custom_define(RID p_shader, const String &p_define);

	void set_shader_async_hidden_forbidden(bool p_forbidden) {}
	bool is_shader_async_hidden_forbidden() { return false; }

	void _update_shader(Shader *p_shader) const;
	void update_dirty_shaders();

	/* COMMON MATERIAL API */

	struct Material : public RID_Data {
		Shader *shader;
		RBMap<StringName, Variant> params;
		SelfList<Material> list;
		SelfList<Material> dirty_list;
		Vector<Pair<StringName, RID>> textures;
		float line_width;
		int render_priority;

		RID next_pass;

		uint32_t index;
		uint64_t last_pass;

		RBMap<Geometry *, int> geometry_owners;

		bool can_cast_shadow_cache;
		bool is_animated_cache;

		Material() :
				list(this),
				dirty_list(this) {
			can_cast_shadow_cache = false;
			is_animated_cache = false;
			shader = nullptr;
			line_width = 1.0;
			last_pass = 0;
			render_priority = 0;
		}
	};

	mutable SelfList<Material>::List _material_dirty_list;
	void _material_make_dirty(Material *p_material) const;

	void _material_add_geometry(RID p_material, Geometry *p_geometry);
	void _material_remove_geometry(RID p_material, Geometry *p_geometry);

	void _update_material(Material *p_material);

	mutable RID_Owner<Material> material_owner;

	virtual RID material_create();

	virtual void material_set_shader(RID p_material, RID p_shader);
	virtual RID material_get_shader(RID p_material) const;

	virtual void material_set_param(RID p_material, const StringName &p_param, const Variant &p_value);
	virtual Variant material_get_param(RID p_material, const StringName &p_param) const;
	virtual Variant material_get_param_default(RID p_material, const StringName &p_param) const;

	virtual void material_set_line_width(RID p_material, float p_width);
	virtual void material_set_next_pass(RID p_material, RID p_next_material);

	virtual bool material_is_animated(RID p_material);
	virtual bool material_casts_shadows(RID p_material);
	virtual bool material_uses_tangents(RID p_material);
	virtual bool material_uses_ensure_correct_normals(RID p_material);

	virtual void material_set_render_priority(RID p_material, int priority);

	void update_dirty_materials();

	/* MESH API */

	struct Mesh;

	struct Surface : public Geometry {
		struct Attrib {
			bool enabled;
			bool integer;
			GLuint index;
			GLint size;
			GLenum type;
			GLboolean normalized;
			GLsizei stride;
			uint32_t offset;
		};

		Attrib attribs[RS::ARRAY_MAX];

		Mesh *mesh;
		uint32_t format;

		GLuint vertex_id;
		GLuint index_id;

		AABB aabb;

		int array_len;
		int index_array_len;
		int max_bone;

		int array_byte_size;
		int index_array_byte_size;

		RS::PrimitiveType primitive;

		bool active;

		PoolVector<uint8_t> data;
		PoolVector<uint8_t> index_data;

		Vector<PoolVector<uint8_t>> blend_shape_data;

		GLuint blend_shape_buffer_id;
		size_t blend_shape_buffer_size;

		int total_data_size;

		Surface() :
				mesh(nullptr),
				array_len(0),
				index_array_len(0),
				array_byte_size(0),
				index_array_byte_size(0),
				primitive(RS::PRIMITIVE_POINTS),
				active(false),
				total_data_size(0) {
		}
	};

	struct MultiMesh;

	struct Mesh : public GeometryOwner {
		bool active;

		Vector<Surface *> surfaces;

		int blend_shape_count;
		RS::BlendShapeMode blend_shape_mode;
		PoolRealArray blend_shape_values;

		SelfList<Mesh> update_list;

		AABB custom_aabb;

		mutable uint64_t last_pass;

		SelfList<MultiMesh>::List multimeshes;

		_FORCE_INLINE_ void update_multimeshes() {
			SelfList<MultiMesh> *mm = multimeshes.first();

			while (mm) {
				mm->self()->instance_change_notify(false, true);
				mm = mm->next();
			}
		}

		Mesh() :
				blend_shape_count(0),
				blend_shape_mode(RS::BLEND_SHAPE_MODE_NORMALIZED),
				blend_shape_values(PoolRealArray()),
				update_list(this) {
		}
	};

	mutable RID_Owner<Mesh> mesh_owner;
	SelfList<Mesh>::List blend_shapes_update_list;

	virtual RID mesh_create();

	virtual void mesh_add_surface(RID p_mesh, uint32_t p_format, RS::PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes = Vector<PoolVector<uint8_t>>(), const Vector<AABB> &p_bone_aabbs = Vector<AABB>());

	virtual void mesh_set_blend_shape_count(RID p_mesh, int p_amount);
	virtual int mesh_get_blend_shape_count(RID p_mesh) const;

	virtual void mesh_set_blend_shape_mode(RID p_mesh, RS::BlendShapeMode p_mode);
	virtual RS::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh) const;

	virtual void mesh_set_blend_shape_values(RID p_mesh, PoolVector<float> p_values);
	virtual PoolVector<float> mesh_get_blend_shape_values(RID p_mesh) const;

	virtual void mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data);

	virtual void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	virtual RID mesh_surface_get_material(RID p_mesh, int p_surface) const;

	virtual int mesh_surface_get_array_len(RID p_mesh, int p_surface) const;
	virtual int mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const;

	virtual PoolVector<uint8_t> mesh_surface_get_array(RID p_mesh, int p_surface) const;
	virtual PoolVector<uint8_t> mesh_surface_get_index_array(RID p_mesh, int p_surface) const;

	virtual uint32_t mesh_surface_get_format(RID p_mesh, int p_surface) const;
	virtual RS::PrimitiveType mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const;

	virtual AABB mesh_surface_get_aabb(RID p_mesh, int p_surface) const;
	virtual Vector<PoolVector<uint8_t>> mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const;

	virtual void mesh_remove_surface(RID p_mesh, int p_surface);
	virtual int mesh_get_surface_count(RID p_mesh) const;

	virtual void mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb);
	virtual AABB mesh_get_custom_aabb(RID p_mesh) const;

	virtual AABB mesh_get_aabb(RID p_mesh) const;
	virtual void mesh_clear(RID p_mesh);

	void update_dirty_blend_shapes();

	/* MULTIMESH API */

	struct MultiMesh : public GeometryOwner {
		RID mesh;
		int size;

		RS::MultimeshTransformFormat transform_format;
		RS::MultimeshColorFormat color_format;
		RS::MultimeshCustomDataFormat custom_data_format;

		Vector<float> data;

		AABB aabb;

		SelfList<MultiMesh> update_list;
		SelfList<MultiMesh> mesh_list;

		int visible_instances;

		int xform_floats;
		int color_floats;
		int custom_data_floats;

		bool dirty_aabb;
		bool dirty_data;

		MMInterpolator interpolator;
		LocalVector<RID> linked_canvas_items;

		MultiMesh() :
				size(0),
				transform_format(RS::MULTIMESH_TRANSFORM_2D),
				color_format(RS::MULTIMESH_COLOR_NONE),
				custom_data_format(RS::MULTIMESH_CUSTOM_DATA_NONE),
				update_list(this),
				mesh_list(this),
				visible_instances(-1),
				xform_floats(0),
				color_floats(0),
				custom_data_floats(0),
				dirty_aabb(true),
				dirty_data(true) {
		}
	};

	mutable RID_Owner<MultiMesh> multimesh_owner;

	SelfList<MultiMesh>::List multimesh_update_list;

	virtual RID _multimesh_create();

	virtual void _multimesh_allocate(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, RS::MultimeshColorFormat p_color_format, RS::MultimeshCustomDataFormat p_data = RS::MULTIMESH_CUSTOM_DATA_NONE);
	virtual int _multimesh_get_instance_count(RID p_multimesh) const;

	virtual void _multimesh_set_mesh(RID p_multimesh, RID p_mesh);
	virtual void _multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform);
	virtual void _multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform);
	virtual void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color);
	virtual void _multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_custom_data);

	virtual RID _multimesh_get_mesh(RID p_multimesh) const;

	virtual Transform _multimesh_instance_get_transform(RID p_multimesh, int p_index) const;
	virtual Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const;
	virtual Color _multimesh_instance_get_color(RID p_multimesh, int p_index) const;
	virtual Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const;

	virtual void _multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array);

	virtual void _multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	virtual int _multimesh_get_visible_instances(RID p_multimesh) const;

	virtual AABB _multimesh_get_aabb(RID p_multimesh) const;
	virtual MMInterpolator *_multimesh_get_interpolator(RID p_multimesh) const;
	virtual void multimesh_attach_canvas_item(RID p_multimesh, RID p_canvas_item, bool p_attach);

	void update_dirty_multimeshes();

	/* RENDER TARGET */

	struct RenderTarget : public RID_Data {
		GLuint fbo;
		GLuint color;
		GLuint depth;

		GLuint multisample_fbo;
		GLuint multisample_color;
		GLuint multisample_depth;
		bool multisample_active;

		struct Effect {
			GLuint fbo;
			int width;
			int height;

			GLuint color;

			Effect() :
					fbo(0),
					width(0),
					height(0),
					color(0) {
			}
		};

		Effect copy_screen_effect;

		struct MipMaps {
			struct Size {
				GLuint fbo;
				GLuint color;
				int width;
				int height;
			};

			Vector<Size> sizes;
			GLuint color;
			int levels;

			MipMaps() :
					color(0),
					levels(0) {
			}
		};

		MipMaps mip_maps[2];

		struct External {
			GLuint fbo;
			GLuint color;
			GLuint depth;
			bool depth_owned;

			External() :
					fbo(0),
					color(0),
					depth(0),
					depth_owned(false) {
			}
		} external;

		int x, y, width, height;

		bool flags[RENDER_TARGET_FLAG_MAX];

		bool used_in_frame;
		RS::ViewportMSAA msaa;

		bool use_fxaa;
		bool use_debanding;
		float sharpen_intensity;

		RID texture;

		bool used_dof_blur_near;
		bool mip_maps_allocated;

		RenderTarget() :
				fbo(0),
				color(0),
				depth(0),
				multisample_fbo(0),
				multisample_color(0),
				multisample_depth(0),
				multisample_active(false),
				x(0),
				y(0),
				width(0),
				height(0),
				used_in_frame(false),
				msaa(RS::VIEWPORT_MSAA_DISABLED),
				use_fxaa(false),
				use_debanding(false),
				sharpen_intensity(0.0),
				used_dof_blur_near(false),
				mip_maps_allocated(false) {
			for (int i = 0; i < RENDER_TARGET_FLAG_MAX; ++i) {
				flags[i] = false;
			}
			external.fbo = 0;
		}
	};

	mutable RID_Owner<RenderTarget> render_target_owner;

	void _render_target_clear(RenderTarget *rt);
	void _render_target_allocate(RenderTarget *rt);

	virtual RID render_target_create();
	virtual void render_target_set_position(RID p_render_target, int p_x, int p_y);
	virtual void render_target_set_size(RID p_render_target, int p_width, int p_height);
	virtual RID render_target_get_texture(RID p_render_target) const;
	virtual uint32_t render_target_get_depth_texture_id(RID p_render_target) const;
	virtual void render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id, unsigned int p_depth_id);

	virtual void render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value);
	virtual bool render_target_was_used(RID p_render_target);
	virtual void render_target_clear_used(RID p_render_target);
	virtual void render_target_set_msaa(RID p_render_target, RS::ViewportMSAA p_msaa);
	virtual void render_target_set_use_fxaa(RID p_render_target, bool p_fxaa);
	virtual void render_target_set_use_debanding(RID p_render_target, bool p_debanding);
	virtual void render_target_set_sharpen_intensity(RID p_render_target, float p_intensity);

	virtual RS::InstanceType get_base_type(RID p_rid) const;

	virtual bool free(RID p_rid);

	struct Frame {
		RenderTarget *current_rt;

		bool clear_request;
		Color clear_request_color;
		float time[4];
		float delta;
		uint64_t count;

	} frame;

	struct GLWrapper {
		mutable BitFieldDynamic texture_unit_table;
		mutable LocalVector<uint32_t> texture_units_bound;

		void gl_active_texture(GLenum p_texture) const {
			::glActiveTexture(p_texture);

			p_texture -= GL_TEXTURE0;

			// Check for below zero and above max in one check.
			ERR_FAIL_COND((unsigned int)p_texture >= texture_unit_table.get_num_bits());

			// Set if the first occurrence in the table.
			if (texture_unit_table.check_and_set(p_texture)) {
				texture_units_bound.push_back(p_texture);
			}
		}
		void initialize(int p_max_texture_image_units);
		void reset();
	} gl_wrapper;

	void initialize();
	void finalize();

	void _copy_screen();

	virtual bool has_os_feature(const String &p_feature) const;

	virtual void update_dirty_resources();

	virtual void set_debug_generate_wireframes(bool p_generate);

	virtual void render_info_begin_capture();
	virtual void render_info_end_capture();
	virtual int get_captured_render_info(RS::RenderInfo p_info);

	virtual uint64_t get_render_info(RS::RenderInfo p_info);
	virtual String get_video_adapter_name() const;
	virtual String get_video_adapter_vendor() const;

	// NOTE : THESE SIZES ARE IN BYTES. BUFFER SIZES MAY NOT BE SPECIFIED IN BYTES SO REMEMBER TO CONVERT THEM WHEN CALLING.
	void buffer_orphan_and_upload(unsigned int p_buffer_size_bytes, unsigned int p_offset_bytes, unsigned int p_data_size_bytes, const void *p_data, GLenum p_target = GL_ARRAY_BUFFER, GLenum p_usage = GL_DYNAMIC_DRAW, bool p_optional_orphan = false) const;
	bool safe_buffer_sub_data(unsigned int p_total_buffer_size, GLenum p_target, unsigned int p_offset, unsigned int p_data_size, const void *p_data, unsigned int &r_offset_after) const;

	RasterizerStorageGLES2();
};

inline bool RasterizerStorageGLES2::safe_buffer_sub_data(unsigned int p_total_buffer_size, GLenum p_target, unsigned int p_offset, unsigned int p_data_size, const void *p_data, unsigned int &r_offset_after) const {
	r_offset_after = p_offset + p_data_size;
#ifdef DEBUG_ENABLED
	// we are trying to write across the edge of the buffer
	if (r_offset_after > p_total_buffer_size) {
		return false;
	}
#endif
	glBufferSubData(p_target, p_offset, p_data_size, p_data);
	return true;
}

// standardize the orphan / upload in one place so it can be changed per platform as necessary, and avoid future
// bugs causing pipeline stalls
// NOTE : THESE SIZES ARE IN BYTES. BUFFER SIZES MAY NOT BE SPECIFIED IN BYTES SO REMEMBER TO CONVERT THEM WHEN CALLING.
inline void RasterizerStorageGLES2::buffer_orphan_and_upload(unsigned int p_buffer_size_bytes, unsigned int p_offset_bytes, unsigned int p_data_size_bytes, const void *p_data, GLenum p_target, GLenum p_usage, bool p_optional_orphan) const {
	// Orphan the buffer to avoid CPU/GPU sync points caused by glBufferSubData
	// Was previously #ifndef GLES_OVER_GL however this causes stalls on desktop mac also (and possibly other)
	if (!p_optional_orphan || (config.should_orphan)) {
		glBufferData(p_target, p_buffer_size_bytes, nullptr, p_usage);
#ifdef RASTERIZER_EXTRA_CHECKS
		// fill with garbage off the end of the array
		if (p_buffer_size_bytes) {
			unsigned int start = p_offset_bytes + p_data_size_bytes;
			unsigned int end = start + 1024;
			if (end < p_buffer_size_bytes) {
				uint8_t *garbage = (uint8_t *)alloca(1024);
				for (int n = 0; n < 1024; n++) {
					garbage[n] = Math::random(0, 255);
				}
				glBufferSubData(p_target, start, 1024, garbage);
			}
		}
#endif
	}
	ERR_FAIL_COND((p_offset_bytes + p_data_size_bytes) > p_buffer_size_bytes);
	glBufferSubData(p_target, p_offset_bytes, p_data_size_bytes, p_data);
}

#endif // RASTERIZERSTORAGEGLES2_H

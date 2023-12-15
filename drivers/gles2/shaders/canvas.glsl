/* clang-format off */
[vertex]

#ifdef USE_GLES_OVER_GL
#define lowp
#define mediump
#define highp
#else
precision highp float;
precision highp int;
#endif

uniform highp mat4 projection_matrix;
/* clang-format on */

#include "stdlib.glsl"

uniform highp mat4 modelview_matrix;
uniform highp mat4 extra_matrix;
attribute highp vec2 vertex; // attrib:0

attribute vec4 color_attrib; // attrib:3
attribute vec2 uv_attrib; // attrib:4

#ifdef USE_ATTRIB_MODULATE
attribute highp vec4 modulate_attrib; // attrib:5
#endif

// Usually, final_modulate is passed as a uniform. However during batching
// If larger fvfs are used, final_modulate is passed as an attribute.
// we need to read from the attribute in custom vertex shader
// rather than the uniform. We do this by specifying final_modulate_alias
// in shaders rather than final_modulate directly.
#ifdef USE_ATTRIB_MODULATE
#define final_modulate_alias modulate_attrib
#else
#define final_modulate_alias final_modulate
#endif

#ifdef USE_INSTANCING

attribute highp vec4 instance_xform0; //attrib:8
attribute highp vec4 instance_xform1; //attrib:9
attribute highp vec4 instance_xform2; //attrib:10
attribute highp vec4 instance_color; //attrib:11

#ifdef USE_INSTANCE_CUSTOM
attribute highp vec4 instance_custom_data; //attrib:12
#endif

#endif

varying vec2 uv_interp;
varying vec4 color_interp;

#ifdef USE_ATTRIB_MODULATE
// modulate doesn't need interpolating but we need to send it to the fragment shader
varying vec4 modulate_interp;
#endif

#ifdef MODULATE_USED
uniform vec4 final_modulate;
#endif

uniform highp vec2 color_texpixel_size;

#ifdef USE_TEXTURE_RECT

uniform vec4 dst_rect;
uniform vec4 src_rect;

#endif

uniform highp float time;

const bool at_light_pass = false;

/* clang-format off */

VERTEX_SHADER_GLOBALS

/* clang-format on */

vec2 select(vec2 a, vec2 b, bvec2 c) {
	vec2 ret;

	ret.x = c.x ? b.x : a.x;
	ret.y = c.y ? b.y : a.y;

	return ret;
}

void main() {
	vec4 color = color_attrib;
	vec2 uv;

#ifdef USE_INSTANCING
	mat4 extra_matrix_instance = extra_matrix * transpose(mat4(instance_xform0, instance_xform1, instance_xform2, vec4(0.0, 0.0, 0.0, 1.0)));
	color *= instance_color;

#ifdef USE_INSTANCE_CUSTOM
	vec4 instance_custom = instance_custom_data;
#else
	vec4 instance_custom = vec4(0.0);
#endif

#else
	mat4 extra_matrix_instance = extra_matrix;
	vec4 instance_custom = vec4(0.0);
#endif

#ifdef USE_TEXTURE_RECT

	if (dst_rect.z < 0.0) { // Transpose is encoded as negative dst_rect.z
		uv = src_rect.xy + abs(src_rect.zw) * vertex.yx;
	} else {
		uv = src_rect.xy + abs(src_rect.zw) * vertex;
	}

	vec4 outvec = vec4(0.0, 0.0, 0.0, 1.0);

	// This is what is done in the GLES 3 bindings and should
	// take care of flipped rects.
	//
	// But it doesn't.
	// I don't know why, will need to investigate further.

	outvec.xy = dst_rect.xy + abs(dst_rect.zw) * select(vertex, vec2(1.0, 1.0) - vertex, lessThan(src_rect.zw, vec2(0.0, 0.0)));

	// outvec.xy = dst_rect.xy + abs(dst_rect.zw) * vertex;
#else
	vec4 outvec = vec4(vertex.xy, 0.0, 1.0);

	uv = uv_attrib;
#endif

	float point_size = 1.0;

	{
		vec2 src_vtx = outvec.xy;
		/* clang-format off */

VERTEX_SHADER_CODE

		/* clang-format on */
	}

	gl_PointSize = point_size;

#ifdef USE_ATTRIB_MODULATE
	// modulate doesn't need interpolating but we need to send it to the fragment shader
	modulate_interp = modulate_attrib;
#endif

	// transform is in uniforms
#if !defined(SKIP_TRANSFORM_USED)
	outvec = extra_matrix_instance * outvec;
	outvec = modelview_matrix * outvec;
#endif

	color_interp = color;

#ifdef USE_PIXEL_SNAP
	outvec.xy = floor(outvec + 0.5).xy;
	// precision issue on some hardware creates artifacts within texture
	// offset uv by a small amount to avoid
	uv += 1e-5;
#endif

	uv_interp = uv;
	gl_Position = projection_matrix * outvec;
}

/* clang-format off */
[fragment]

// texture2DLodEXT and textureCubeLodEXT are fragment shader specific.
// Do not copy these defines in the vertex section.
#ifndef USE_GLES_OVER_GL
#ifdef GL_EXT_shader_texture_lod
#extension GL_EXT_shader_texture_lod : enable
#define texture2DLod(img, coord, lod) texture2DLodEXT(img, coord, lod)
#define textureCubeLod(img, coord, lod) textureCubeLodEXT(img, coord, lod)
#endif
#endif // !USE_GLES_OVER_GL

#ifdef GL_ARB_shader_texture_lod
#extension GL_ARB_shader_texture_lod : enable
#endif

#if !defined(GL_EXT_shader_texture_lod) && !defined(GL_ARB_shader_texture_lod)
#define texture2DLod(img, coord, lod) texture2D(img, coord, lod)
#define textureCubeLod(img, coord, lod) textureCube(img, coord, lod)
#endif

#ifdef USE_GLES_OVER_GL
#define lowp
#define mediump
#define highp
#else
#if defined(USE_HIGHP_PRECISION)
precision highp float;
precision highp int;
#else
precision mediump float;
precision mediump int;
#endif
#endif

#include "stdlib.glsl"

uniform sampler2D color_texture; // texunit:-1
/* clang-format on */
uniform highp vec2 color_texpixel_size;
uniform mediump sampler2D normal_texture; // texunit:-2

varying mediump vec2 uv_interp;
varying mediump vec4 color_interp;

#ifdef USE_ATTRIB_MODULATE
varying mediump vec4 modulate_interp;
#endif

uniform highp float time;

uniform vec4 final_modulate;

#ifdef SCREEN_TEXTURE_USED

uniform sampler2D screen_texture; // texunit:-4

#endif

#ifdef SCREEN_UV_USED

uniform vec2 screen_pixel_size;

#endif

const bool at_light_pass = false;

uniform bool use_default_normal;

/* clang-format off */

FRAGMENT_SHADER_GLOBALS

/* clang-format on */

void light_compute(
		inout vec4 light,
		inout vec2 light_vec,
		inout float light_height,
		inout vec4 light_color,
		vec2 light_uv,
		inout vec4 shadow_color,
		inout vec2 shadow_vec,
		vec3 normal,
		vec2 uv,
#if defined(SCREEN_UV_USED)
		vec2 screen_uv,
#endif
		vec4 color) {

#if defined(USE_LIGHT_SHADER_CODE)

	/* clang-format off */

LIGHT_SHADER_CODE

	/* clang-format on */

#endif
}

void main() {
	vec4 color = color_interp;
	vec2 uv = uv_interp;
#ifdef USE_FORCE_REPEAT
	//needs to use this to workaround GLES2/WebGL1 forcing tiling that textures that don't support it
	uv = mod(uv, vec2(1.0, 1.0));
#endif

#if !defined(COLOR_USED)
	//default behavior, texture by color
	color *= texture2D(color_texture, uv);
#endif

#ifdef SCREEN_UV_USED
	vec2 screen_uv = gl_FragCoord.xy * screen_pixel_size;
#endif

	vec3 normal;

#if defined(NORMAL_USED)

	bool normal_used = true;
#else
	bool normal_used = false;
#endif

	if (use_default_normal) {
		normal.xy = texture2D(normal_texture, uv).xy * 2.0 - 1.0;
		normal.z = sqrt(max(0.0, 1.0 - dot(normal.xy, normal.xy)));
		normal_used = true;
	} else {
		normal = vec3(0.0, 0.0, 1.0);
	}

	{
		float normal_depth = 1.0;

#if defined(NORMALMAP_USED)
		vec3 normal_map = vec3(0.0, 0.0, 1.0);
		normal_used = true;
#endif

		// If larger fvfs are used, final_modulate is passed as an attribute.
		// we need to read from this in custom fragment shaders or applying in the post step,
		// rather than using final_modulate directly.
#if defined(final_modulate_alias)
#undef final_modulate_alias
#endif
#ifdef USE_ATTRIB_MODULATE
#define final_modulate_alias modulate_interp
#else
#define final_modulate_alias final_modulate
#endif

		/* clang-format off */

FRAGMENT_SHADER_CODE

		/* clang-format on */

#if defined(NORMALMAP_USED)
		normal = mix(vec3(0.0, 0.0, 1.0), normal_map * vec3(2.0, -2.0, 1.0) - vec3(1.0, -1.0, 0.0), normal_depth);
#endif
	}

#if !defined(MODULATE_USED)
	color *= final_modulate_alias;
#endif

#ifdef LINEAR_TO_SRGB
	// regular Linear -> SRGB conversion
	vec3 a = vec3(0.055);
	color.rgb = mix((vec3(1.0) + a) * pow(color.rgb, vec3(1.0 / 2.4)) - a, 12.92 * color.rgb, vec3(lessThan(color.rgb, vec3(0.0031308))));
#endif

	gl_FragColor = color;
}

/*************************************************************************/
/*  rasterizer_storage_gles2.cpp                                         */
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

#include "rasterizer_storage_gles2.h"

#include "core/config/project_settings.h"
#include "core/math/transform.h"
#include "rasterizer_canvas_gles2.h"
#include "servers/rendering/rendering_server_canvas.h"
#include "servers/rendering/rendering_server_globals.h"
#include "servers/rendering/shader_language.h"

GLuint RasterizerStorageGLES2::system_fbo = 0;

/* TEXTURE API */

#define _EXT_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

#define _EXT_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#define _EXT_COMPRESSED_RED_RGTC1 0x8DBB
#define _EXT_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define _EXT_COMPRESSED_RG_RGTC2 0x8DBD
#define _EXT_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define _EXT_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define _EXT_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#define _EXT_ETC1_RGB8_OES 0x8D64

#define _EXT_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define _EXT_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define _EXT_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define _EXT_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03

#define _EXT_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define _EXT_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT 0x8A55
#define _EXT_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define _EXT_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT 0x8A57

#define _EXT_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define _EXT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#define _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F

#define _GL_TEXTURE_EXTERNAL_OES 0x8D65

#ifdef GLES_OVER_GL
#define _GL_HALF_FLOAT_OES 0x140B
#else
#define _GL_HALF_FLOAT_OES 0x8D61
#endif

#define _EXT_TEXTURE_CUBE_MAP_SEAMLESS 0x884F

#define _GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define _GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF

#define _RED_OES 0x1903

#define _DEPTH_COMPONENT24_OES 0x81A6

#ifndef GLES_OVER_GL
#define glClearDepth glClearDepthf

// enable extensions manually for android and ios
#ifndef UWP_ENABLED
#include <dlfcn.h> // needed to load extensions
#endif

#ifdef IPHONE_ENABLED

#include <OpenGLES/ES2/glext.h>
//void *glRenderbufferStorageMultisampleAPPLE;
//void *glResolveMultisampleFramebufferAPPLE;
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleAPPLE
#elif defined(ANDROID_ENABLED)

#include <GLES2/gl2ext.h>
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT;
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleEXT
#define glFramebufferTexture2DMultisample glFramebufferTexture2DMultisampleEXT

#elif defined(UWP_ENABLED)
#include <GLES2/gl2ext.h>
#define glRenderbufferStorageMultisample glRenderbufferStorageMultisampleANGLE
#define glFramebufferTexture2DMultisample glFramebufferTexture2DMultisampleANGLE
#endif

#define GL_TEXTURE_3D 0x806F
#define GL_MAX_SAMPLES 0x8D57
#endif //!GLES_OVER_GL

void RasterizerStorageGLES2::GLWrapper::initialize(int p_max_texture_image_units) {
	texture_unit_table.create(p_max_texture_image_units);
}

void RasterizerStorageGLES2::GLWrapper::reset() {
	for (uint32_t i = 0; i < texture_units_bound.size(); i++) {
		::glActiveTexture(GL_TEXTURE0 + texture_units_bound[i]);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	texture_units_bound.clear();
	texture_unit_table.blank();
}

void RasterizerStorageGLES2::bind_quad_array() const {
	glBindBuffer(GL_ARRAY_BUFFER, resources.quadie);
	glVertexAttribPointer(RS::ARRAY_VERTEX, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	glVertexAttribPointer(RS::ARRAY_TEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, CAST_INT_TO_UCHAR_PTR(8));

	glEnableVertexAttribArray(RS::ARRAY_VERTEX);
	glEnableVertexAttribArray(RS::ARRAY_TEX_UV);
}

Ref<Image> RasterizerStorageGLES2::_get_gl_image_and_format(const Ref<Image> &p_image, Image::Format p_format, uint32_t p_flags, Image::Format &r_real_format, GLenum &r_gl_format, GLenum &r_gl_internal_format, GLenum &r_gl_type, bool &r_compressed, bool p_force_decompress) const {
	r_gl_format = 0;
	Ref<Image> image = p_image;
	r_compressed = false;
	r_real_format = p_format;

	bool need_decompress = false;

	switch (p_format) {
		case Image::FORMAT_L8: {
			r_gl_internal_format = GL_LUMINANCE;
			r_gl_format = GL_LUMINANCE;
			r_gl_type = GL_UNSIGNED_BYTE;
		} break;
		case Image::FORMAT_LA8: {
			r_gl_internal_format = GL_LUMINANCE_ALPHA;
			r_gl_format = GL_LUMINANCE_ALPHA;
			r_gl_type = GL_UNSIGNED_BYTE;
		} break;
		case Image::FORMAT_R8: {
			r_gl_internal_format = GL_ALPHA;
			r_gl_format = GL_ALPHA;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RG8: {
			ERR_PRINT("RG texture not supported, converting to RGB8.");
			if (image.is_valid()) {
				image->convert(Image::FORMAT_RGB8);
			}
			r_real_format = Image::FORMAT_RGB8;
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RGB8: {
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RGBA8: {
			r_gl_format = GL_RGBA;
			r_gl_internal_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_BYTE;

		} break;
		case Image::FORMAT_RGBA4444: {
			r_gl_internal_format = GL_RGBA;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_SHORT_4_4_4_4;

		} break;
		case Image::FORMAT_RGBA5551: {
			r_gl_internal_format = GL_RGB5_A1;
			r_gl_format = GL_RGBA;
			r_gl_type = GL_UNSIGNED_SHORT_5_5_5_1;

		} break;
		case Image::FORMAT_RF: {
			if (!config.float_texture_supported) {
				ERR_PRINT("R float texture not supported, converting to RGB8.");
				if (image.is_valid()) {
					image->convert(Image::FORMAT_RGB8);
				}
				r_real_format = Image::FORMAT_RGB8;
				r_gl_internal_format = GL_RGB;
				r_gl_format = GL_RGB;
				r_gl_type = GL_UNSIGNED_BYTE;
			} else {
				r_gl_internal_format = GL_ALPHA;
				r_gl_format = GL_ALPHA;
				r_gl_type = GL_FLOAT;
			}
		} break;
		case Image::FORMAT_RGF: {
			ERR_PRINT("RG float texture not supported, converting to RGB8.");
			if (image.is_valid()) {
				image->convert(Image::FORMAT_RGB8);
			}
			r_real_format = Image::FORMAT_RGB8;
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;
		} break;
		case Image::FORMAT_RGBF: {
			if (!config.float_texture_supported) {
				ERR_PRINT("RGB float texture not supported, converting to RGB8.");
				if (image.is_valid()) {
					image->convert(Image::FORMAT_RGB8);
				}
				r_real_format = Image::FORMAT_RGB8;
				r_gl_internal_format = GL_RGB;
				r_gl_format = GL_RGB;
				r_gl_type = GL_UNSIGNED_BYTE;
			} else {
				r_gl_internal_format = GL_RGB;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
			}
		} break;
		case Image::FORMAT_RGBAF: {
			if (!config.float_texture_supported) {
				ERR_PRINT("RGBA float texture not supported, converting to RGBA8.");
				if (image.is_valid()) {
					image->convert(Image::FORMAT_RGBA8);
				}
				r_real_format = Image::FORMAT_RGBA8;
				r_gl_internal_format = GL_RGBA;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
			} else {
				r_gl_internal_format = GL_RGBA;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_FLOAT;
			}
		} break;
		case Image::FORMAT_RH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGBH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGBAH: {
			need_decompress = true;
		} break;
		case Image::FORMAT_RGBE9995: {
			r_gl_internal_format = GL_RGB;
			r_gl_format = GL_RGB;
			r_gl_type = GL_UNSIGNED_BYTE;

			if (image.is_valid()) {
				image = image->rgbe_to_srgb();
			}

			return image;

		} break;
		case Image::FORMAT_DXT1: {
			if (config.s3tc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_DXT3: {
			if (config.s3tc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_DXT5: {
			if (config.s3tc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_RGTC_R: {
			if (config.rgtc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RED_RGTC1_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_RGTC_RG: {
			if (config.rgtc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_BPTC_RGBA: {
			if (config.bptc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_BPTC_UNORM;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {
				need_decompress = true;
			}
		} break;
		case Image::FORMAT_BPTC_RGBF: {
			if (config.bptc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
				r_compressed = true;
			} else {
				need_decompress = true;
			}
		} break;
		case Image::FORMAT_BPTC_RGBFU: {
			if (config.bptc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT;
				r_gl_format = GL_RGB;
				r_gl_type = GL_FLOAT;
				r_compressed = true;
			} else {
				need_decompress = true;
			}
		} break;
		case Image::FORMAT_PVRTC2: {
			if (config.pvrtc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {
				need_decompress = true;
			}
		} break;
		case Image::FORMAT_PVRTC2A: {
			if (config.pvrtc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_PVRTC4: {
			if (config.pvrtc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_PVRTC4A: {
			if (config.pvrtc_supported) {
				r_gl_internal_format = _EXT_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;

			} else {
				need_decompress = true;
			}

		} break;
		case Image::FORMAT_ETC: {
			if (config.etc1_supported) {
				r_gl_internal_format = _EXT_ETC1_RGB8_OES;
				r_gl_format = GL_RGBA;
				r_gl_type = GL_UNSIGNED_BYTE;
				r_compressed = true;
			} else {
				need_decompress = true;
			}
		} break;
		case Image::FORMAT_ETC2_R11: {
			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_R11S: {
			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RG11: {
			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RG11S: {
			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RGB8: {
			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RGBA8: {
			need_decompress = true;
		} break;
		case Image::FORMAT_ETC2_RGB8A1: {
			need_decompress = true;
		} break;
		default: {
			ERR_FAIL_V(Ref<Image>());
		}
	}

	if (need_decompress || p_force_decompress) {
		if (!image.is_null()) {
			image = image->duplicate();
			image->decompress();
			ERR_FAIL_COND_V(image->is_compressed(), image);
			switch (image->get_format()) {
				case Image::FORMAT_RGB8: {
					r_gl_format = GL_RGB;
					r_gl_internal_format = GL_RGB;
					r_gl_type = GL_UNSIGNED_BYTE;
					r_real_format = Image::FORMAT_RGB8;
					r_compressed = false;
				} break;
				case Image::FORMAT_RGBA8: {
					r_gl_format = GL_RGBA;
					r_gl_internal_format = GL_RGBA;
					r_gl_type = GL_UNSIGNED_BYTE;
					r_real_format = Image::FORMAT_RGBA8;
					r_compressed = false;
				} break;
				default: {
					image->convert(Image::FORMAT_RGBA8);
					r_gl_format = GL_RGBA;
					r_gl_internal_format = GL_RGBA;
					r_gl_type = GL_UNSIGNED_BYTE;
					r_real_format = Image::FORMAT_RGBA8;
					r_compressed = false;

				} break;
			}
		}

		return image;
	}

	return p_image;
}

static const GLenum _cube_side_enum[6] = {

	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,

};

RID RasterizerStorageGLES2::texture_create() {
	Texture *texture = memnew(Texture);
	ERR_FAIL_COND_V(!texture, RID());
	glGenTextures(1, &texture->tex_id);
	texture->active = false;
	texture->total_data_size = 0;

	return texture_owner.make_rid(texture);
}

void RasterizerStorageGLES2::texture_allocate(RID p_texture, int p_width, int p_height, int p_depth_3d, Image::Format p_format, RenderingServer::TextureType p_type, uint32_t p_flags) {
	GLenum format;
	GLenum internal_format;
	GLenum type;

	bool compressed = false;

	if (p_flags & RS::TEXTURE_FLAG_USED_FOR_STREAMING) {
		p_flags &= ~RS::TEXTURE_FLAG_MIPMAPS; // no mipies for video
	}

	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);
	texture->width = p_width;
	texture->height = p_height;
	texture->format = p_format;

	if (texture->width > config.max_texture_size || texture->height > config.max_texture_size) {
		WARN_PRINT("Cannot create texture larger than maximum hardware supported size of " + itos(config.max_texture_size) + ". Setting size to maximum.");
		texture->width = MIN(texture->width, config.max_texture_size);
		texture->height = MIN(texture->height, config.max_texture_size);
	}

	texture->flags = p_flags;
	texture->stored_cube_sides = 0;
	texture->type = p_type;

	switch (p_type) {
		case RS::TEXTURE_TYPE_2D: {
			texture->target = GL_TEXTURE_2D;
			texture->images.resize(1);
		} break;
		case RS::TEXTURE_TYPE_EXTERNAL: {
#ifdef ANDROID_ENABLED
			texture->target = _GL_TEXTURE_EXTERNAL_OES;
#else
			texture->target = GL_TEXTURE_2D;
#endif
			texture->images.resize(0);
		} break;
		case RS::TEXTURE_TYPE_CUBEMAP: {
			texture->target = GL_TEXTURE_CUBE_MAP;
			texture->images.resize(6);
		} break;
		case RS::TEXTURE_TYPE_2D_ARRAY:
		case RS::TEXTURE_TYPE_3D: {
			texture->target = GL_TEXTURE_3D;
			ERR_PRINT("3D textures and Texture Arrays are not supported in GLES2. Please switch to the GLES3 backend.");
			return;
		} break;
		default: {
			ERR_PRINT("Unknown texture type!");
			return;
		}
	}

	if (p_type != RS::TEXTURE_TYPE_EXTERNAL) {
		texture->alloc_width = texture->width;
		texture->alloc_height = texture->height;
		texture->resize_to_po2 = false;
		if (!config.support_npot_repeat_mipmap) {
			int po2_width = next_power_of_2(p_width);
			int po2_height = next_power_of_2(p_height);

			bool is_po2 = p_width == po2_width && p_height == po2_height;

			if (!is_po2 && (p_flags & RS::TEXTURE_FLAG_REPEAT || p_flags & RS::TEXTURE_FLAG_MIPMAPS)) {
				if (p_flags & RS::TEXTURE_FLAG_USED_FOR_STREAMING) {
					//not supported
					ERR_PRINT("Streaming texture for non power of 2 or has mipmaps on this hardware: " + texture->path + "'. Mipmaps and repeat disabled.");
					texture->flags &= ~(RS::TEXTURE_FLAG_REPEAT | RS::TEXTURE_FLAG_MIPMAPS);
				} else {
					texture->alloc_height = po2_height;
					texture->alloc_width = po2_width;
					texture->resize_to_po2 = true;
				}
			}
		}

		Image::Format real_format;
		_get_gl_image_and_format(Ref<Image>(),
				texture->format,
				texture->flags,
				real_format,
				format,
				internal_format,
				type,
				compressed,
				texture->resize_to_po2);

		texture->gl_format_cache = format;
		texture->gl_type_cache = type;
		texture->gl_internal_format_cache = internal_format;
		texture->data_size = 0;
		texture->mipmaps = 1;

		texture->compressed = compressed;
	}

	gl_wrapper.gl_active_texture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	if (p_type == RS::TEXTURE_TYPE_EXTERNAL) {
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else if (p_flags & RS::TEXTURE_FLAG_USED_FOR_STREAMING) {
		//prealloc if video
		glTexImage2D(texture->target, 0, internal_format, texture->alloc_width, texture->alloc_height, 0, format, type, nullptr);
	}

	texture->active = true;
}

void RasterizerStorageGLES2::texture_set_data(RID p_texture, const Ref<Image> &p_image, int p_layer) {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND(!texture);
	if (texture->target == GL_TEXTURE_3D) {
		// Target is set to a 3D texture or array texture, exit early to avoid spamming errors
		return;
	}
	ERR_FAIL_COND(!texture->active);
	ERR_FAIL_COND(texture->render_target);
	ERR_FAIL_COND(texture->format != p_image->get_format());
	ERR_FAIL_COND(p_image.is_null());
	ERR_FAIL_COND(texture->type == RS::TEXTURE_TYPE_EXTERNAL);

	GLenum type;
	GLenum format;
	GLenum internal_format;
	bool compressed = false;

	if (config.keep_original_textures && !(texture->flags & RS::TEXTURE_FLAG_USED_FOR_STREAMING)) {
		texture->images.write[p_layer] = p_image;
	}

	Image::Format real_format;
	Ref<Image> img = _get_gl_image_and_format(p_image, p_image->get_format(), texture->flags, real_format, format, internal_format, type, compressed, texture->resize_to_po2);

	if (texture->resize_to_po2) {
		if (p_image->is_compressed()) {
			ERR_PRINT("Texture '" + texture->path + "' is required to be a power of 2 because it uses either mipmaps or repeat, so it was decompressed. This will hurt performance and memory usage.");
		}

		if (img == p_image) {
			img = img->duplicate();
		}
		img->resize_to_po2(false, texture->flags & RS::TEXTURE_FLAG_FILTER ? Image::INTERPOLATE_BILINEAR : Image::INTERPOLATE_NEAREST);
	}

	if (config.shrink_textures_x2 && (p_image->has_mipmaps() || !p_image->is_compressed()) && !(texture->flags & RS::TEXTURE_FLAG_USED_FOR_STREAMING)) {
		texture->alloc_height = MAX(1, texture->alloc_height / 2);
		texture->alloc_width = MAX(1, texture->alloc_width / 2);

		if (texture->alloc_width == img->get_width() / 2 && texture->alloc_height == img->get_height() / 2) {
			img->shrink_x2();
		} else if (img->get_format() <= Image::FORMAT_RGBA8) {
			img->resize(texture->alloc_width, texture->alloc_height, Image::INTERPOLATE_BILINEAR);
		}
	}

	GLenum blit_target = (texture->target == GL_TEXTURE_CUBE_MAP) ? _cube_side_enum[p_layer] : GL_TEXTURE_2D;

	texture->data_size = img->get_data().size();
	PoolVector<uint8_t>::Read read = img->get_data().read();
	ERR_FAIL_COND(!read.ptr());

	gl_wrapper.gl_active_texture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	texture->ignore_mipmaps = compressed && !img->has_mipmaps();

	if ((texture->flags & RS::TEXTURE_FLAG_MIPMAPS) && !texture->ignore_mipmaps) {
		if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
		}
	} else {
		if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear Filtering

	} else {
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // raw Filtering
	}

	if (((texture->flags & RS::TEXTURE_FLAG_REPEAT) || (texture->flags & RS::TEXTURE_FLAG_MIRRORED_REPEAT)) && texture->target != GL_TEXTURE_CUBE_MAP) {
		if (texture->flags & RS::TEXTURE_FLAG_MIRRORED_REPEAT) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		} else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	} else {
		//glTexParameterf( texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (config.use_anisotropic_filter) {
		if (texture->flags & RS::TEXTURE_FLAG_ANISOTROPIC_FILTER) {
			glTexParameterf(texture->target, _GL_TEXTURE_MAX_ANISOTROPY_EXT, config.anisotropic_level);
		} else {
			glTexParameterf(texture->target, _GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
		}
	}

	int mipmaps = ((texture->flags & RS::TEXTURE_FLAG_MIPMAPS) && img->has_mipmaps()) ? img->get_mipmap_count() + 1 : 1;

	int w = img->get_width();
	int h = img->get_height();

	int tsize = 0;

	for (int i = 0; i < mipmaps; i++) {
		int size, ofs;
		img->get_mipmap_offset_and_size(i, ofs, size);

		if (compressed) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

			int bw = w;
			int bh = h;

			glCompressedTexImage2D(blit_target, i, internal_format, bw, bh, 0, size, &read[ofs]);
		} else {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			if (texture->flags & RS::TEXTURE_FLAG_USED_FOR_STREAMING) {
				glTexSubImage2D(blit_target, i, 0, 0, w, h, format, type, &read[ofs]);
			} else {
				glTexImage2D(blit_target, i, internal_format, w, h, 0, format, type, &read[ofs]);
			}
		}

		tsize += size;

		w = MAX(1, w >> 1);
		h = MAX(1, h >> 1);
	}

	info.texture_mem -= texture->total_data_size;
	texture->total_data_size = tsize;
	info.texture_mem += texture->total_data_size;

	// printf("texture: %i x %i - size: %i - total: %i\n", texture->width, texture->height, tsize, info.texture_mem);

	texture->stored_cube_sides |= (1 << p_layer);

	if ((texture->flags & RS::TEXTURE_FLAG_MIPMAPS) && mipmaps == 1 && !texture->ignore_mipmaps && (texture->type != RS::TEXTURE_TYPE_CUBEMAP || texture->stored_cube_sides == (1 << 6) - 1)) {
		//generate mipmaps if they were requested and the image does not contain them
		glGenerateMipmap(texture->target);
	}

	texture->mipmaps = mipmaps;
}

void RasterizerStorageGLES2::texture_set_data_partial(RID p_texture, const Ref<Image> &p_image, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, int p_dst_mip, int p_layer) {
	// TODO
	ERR_PRINT("Not implemented (ask Karroffel to do it :p)");
}

Ref<Image> RasterizerStorageGLES2::texture_get_data(RID p_texture, int p_layer) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, Ref<Image>());
	ERR_FAIL_COND_V(!texture->active, Ref<Image>());
	ERR_FAIL_COND_V(texture->data_size == 0 && !texture->render_target, Ref<Image>());

	if (texture->type == RS::TEXTURE_TYPE_CUBEMAP && p_layer < 6 && p_layer >= 0 && !texture->images[p_layer].is_null()) {
		return texture->images[p_layer];
	}

#ifdef GLES_OVER_GL

	Image::Format real_format;
	GLenum gl_format;
	GLenum gl_internal_format;
	GLenum gl_type;
	bool compressed;
	_get_gl_image_and_format(Ref<Image>(), texture->format, texture->flags, real_format, gl_format, gl_internal_format, gl_type, compressed, false);

	PoolVector<uint8_t> data;

	int data_size = Image::get_image_data_size(texture->alloc_width, texture->alloc_height, real_format, texture->mipmaps > 1);

	data.resize(data_size * 2); //add some memory at the end, just in case for buggy drivers
	PoolVector<uint8_t>::Write wb = data.write();

	gl_wrapper.gl_active_texture(GL_TEXTURE0);

	glBindTexture(texture->target, texture->tex_id);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	for (int i = 0; i < texture->mipmaps; i++) {
		int ofs = Image::get_image_mipmap_offset(texture->alloc_width, texture->alloc_height, real_format, i);

		if (texture->compressed) {
			glPixelStorei(GL_PACK_ALIGNMENT, 4);
			glGetCompressedTexImage(texture->target, i, &wb[ofs]);
		} else {
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glGetTexImage(texture->target, i, texture->gl_format_cache, texture->gl_type_cache, &wb[ofs]);
		}
	}

	wb.release();

	data.resize(data_size);

	Image *img = memnew(Image(texture->alloc_width, texture->alloc_height, texture->mipmaps > 1, real_format, data));

	return Ref<Image>(img);
#else

	Image::Format real_format;
	GLenum gl_format;
	GLenum gl_internal_format;
	GLenum gl_type;
	bool compressed;
	_get_gl_image_and_format(Ref<Image>(), texture->format, texture->flags, real_format, gl_format, gl_internal_format, gl_type, compressed, texture->resize_to_po2);

	PoolVector<uint8_t> data;

	int data_size = Image::get_image_data_size(texture->alloc_width, texture->alloc_height, Image::FORMAT_RGBA8, false);

	data.resize(data_size * 2); //add some memory at the end, just in case for buggy drivers
	PoolVector<uint8_t>::Write wb = data.write();

	GLuint temp_framebuffer;
	glGenFramebuffers(1, &temp_framebuffer);

	GLuint temp_color_texture;
	glGenTextures(1, &temp_color_texture);

	glBindFramebuffer(GL_FRAMEBUFFER, temp_framebuffer);

	glBindTexture(GL_TEXTURE_2D, temp_color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->alloc_width, texture->alloc_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, temp_color_texture, 0);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glColorMask(1, 1, 1, 1);
	gl_wrapper.gl_active_texture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->tex_id);

	glViewport(0, 0, texture->alloc_width, texture->alloc_height);

	shaders.copy.bind();

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	bind_quad_array();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glReadPixels(0, 0, texture->alloc_width, texture->alloc_height, GL_RGBA, GL_UNSIGNED_BYTE, &wb[0]);

	glDeleteTextures(1, &temp_color_texture);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &temp_framebuffer);

	wb.release();

	data.resize(data_size);

	Image *img = memnew(Image(texture->alloc_width, texture->alloc_height, false, Image::FORMAT_RGBA8, data));
	if (!texture->compressed) {
		img->convert(real_format);
	}

	return Ref<Image>(img);

#endif
}

void RasterizerStorageGLES2::texture_set_flags(RID p_texture, uint32_t p_flags) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	bool had_mipmaps = texture->flags & RS::TEXTURE_FLAG_MIPMAPS;

	texture->flags = p_flags;

	gl_wrapper.gl_active_texture(GL_TEXTURE0);
	glBindTexture(texture->target, texture->tex_id);

	if (((texture->flags & RS::TEXTURE_FLAG_REPEAT) || (texture->flags & RS::TEXTURE_FLAG_MIRRORED_REPEAT)) && texture->target != GL_TEXTURE_CUBE_MAP) {
		if (texture->flags & RS::TEXTURE_FLAG_MIRRORED_REPEAT) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		} else {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	} else {
		//glTexParameterf( texture->target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (config.use_anisotropic_filter) {
		if (texture->flags & RS::TEXTURE_FLAG_ANISOTROPIC_FILTER) {
			glTexParameterf(texture->target, _GL_TEXTURE_MAX_ANISOTROPY_EXT, config.anisotropic_level);
		} else {
			glTexParameterf(texture->target, _GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
		}
	}

	if ((texture->flags & RS::TEXTURE_FLAG_MIPMAPS) && !texture->ignore_mipmaps) {
		if (!had_mipmaps && texture->mipmaps == 1) {
			glGenerateMipmap(texture->target);
		}
		if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, config.use_fast_texture_filter ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_LINEAR);
		}

	} else {
		if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Linear Filtering

	} else {
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // raw Filtering
	}
}

uint32_t RasterizerStorageGLES2::texture_get_flags(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->flags;
}

Image::Format RasterizerStorageGLES2::texture_get_format(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, Image::FORMAT_L8);

	return texture->format;
}

RenderingServer::TextureType RasterizerStorageGLES2::texture_get_type(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, RS::TEXTURE_TYPE_2D);

	return texture->type;
}

uint32_t RasterizerStorageGLES2::texture_get_texid(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->tex_id;
}

void RasterizerStorageGLES2::texture_bind(RID p_texture, uint32_t p_texture_no) {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND(!texture);

	gl_wrapper.gl_active_texture(GL_TEXTURE0 + p_texture_no);
	glBindTexture(texture->target, texture->tex_id);
}

uint32_t RasterizerStorageGLES2::texture_get_width(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->width;
}

uint32_t RasterizerStorageGLES2::texture_get_height(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->height;
}

uint32_t RasterizerStorageGLES2::texture_get_depth(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND_V(!texture, 0);

	return texture->depth;
}

void RasterizerStorageGLES2::texture_set_size_override(RID p_texture, int p_width, int p_height, int p_depth) {
	Texture *texture = texture_owner.getornull(p_texture);

	ERR_FAIL_COND(!texture);
	ERR_FAIL_COND(texture->render_target);

	ERR_FAIL_COND(p_width <= 0 || p_width > 16384);
	ERR_FAIL_COND(p_height <= 0 || p_height > 16384);
	//real texture size is in alloc width and height
	texture->width = p_width;
	texture->height = p_height;
}

void RasterizerStorageGLES2::texture_set_path(RID p_texture, const String &p_path) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->path = p_path;
}

String RasterizerStorageGLES2::texture_get_path(RID p_texture) const {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!texture, "");

	return texture->path;
}

void RasterizerStorageGLES2::texture_debug_usage(List<RS::TextureInfo> *r_info) {
	List<RID> textures;
	texture_owner.get_owned_list(&textures);

	for (List<RID>::Element *E = textures.front(); E; E = E->next()) {
		Texture *t = texture_owner.getornull(E->get());
		if (!t) {
			continue;
		}
		RS::TextureInfo tinfo;
		tinfo.texture = E->get();
		tinfo.path = t->path;
		tinfo.format = t->format;
		tinfo.width = t->alloc_width;
		tinfo.height = t->alloc_height;
		tinfo.depth = 0;
		tinfo.bytes = t->total_data_size;
		r_info->push_back(tinfo);
	}
}

void RasterizerStorageGLES2::texture_set_shrink_all_x2_on_set_data(bool p_enable) {
	config.shrink_textures_x2 = p_enable;
}

void RasterizerStorageGLES2::textures_keep_original(bool p_enable) {
	config.keep_original_textures = p_enable;
}

Size2 RasterizerStorageGLES2::texture_size_with_proxy(RID p_texture) const {
	const Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND_V(!texture, Size2());
	if (texture->proxy) {
		return Size2(texture->proxy->width, texture->proxy->height);
	} else {
		return Size2(texture->width, texture->height);
	}
}

void RasterizerStorageGLES2::texture_set_proxy(RID p_texture, RID p_proxy) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	if (texture->proxy) {
		texture->proxy->proxy_owners.erase(texture);
		texture->proxy = nullptr;
	}

	if (p_proxy.is_valid()) {
		Texture *proxy = texture_owner.get(p_proxy);
		ERR_FAIL_COND(!proxy);
		ERR_FAIL_COND(proxy == texture);
		proxy->proxy_owners.insert(texture);
		texture->proxy = proxy;
	}
}

void RasterizerStorageGLES2::texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {
	Texture *texture = texture_owner.getornull(p_texture);
	ERR_FAIL_COND(!texture);

	texture->redraw_if_visible = p_enable;
}

void RasterizerStorageGLES2::texture_set_detect_3d_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) {
	Texture *texture = texture_owner.get(p_texture);
	ERR_FAIL_COND(!texture);

	texture->detect_3d = p_callback;
	texture->detect_3d_ud = p_userdata;
}

void RasterizerStorageGLES2::texture_set_detect_srgb_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) {
	Texture *texture = texture_owner.get(p_texture);
	ERR_FAIL_COND(!texture);

	texture->detect_srgb = p_callback;
	texture->detect_srgb_ud = p_userdata;
}

void RasterizerStorageGLES2::texture_set_detect_normal_callback(RID p_texture, RenderingServer::TextureDetectCallback p_callback, void *p_userdata) {
	Texture *texture = texture_owner.get(p_texture);
	ERR_FAIL_COND(!texture);

	texture->detect_normal = p_callback;
	texture->detect_normal_ud = p_userdata;
}

RID RasterizerStorageGLES2::texture_create_radiance_cubemap(RID p_source, int p_resolution) const {
	return RID();
}

/* SHADER API */

RID RasterizerStorageGLES2::shader_create() {
	Shader *shader = memnew(Shader);
	shader->mode = RS::SHADER_CANVAS_ITEM;
	shader->shader = &canvas->state.canvas_shader;
	RID rid = shader_owner.make_rid(shader);
	_shader_make_dirty(shader);
	shader->self = rid;

	return rid;
}

void RasterizerStorageGLES2::_shader_make_dirty(Shader *p_shader) {
	if (p_shader->dirty_list.in_list()) {
		return;
	}

	_shader_dirty_list.add(&p_shader->dirty_list);
}

void RasterizerStorageGLES2::shader_set_code(RID p_shader, const String &p_code) {
	Shader *shader = shader_owner.getornull(p_shader);
	ERR_FAIL_COND(!shader);

	shader->code = p_code;

	String mode_string = ShaderLanguage::get_shader_type(p_code);
	RS::ShaderMode mode;

	mode = RS::SHADER_CANVAS_ITEM;

	if (shader->custom_code_id && mode != shader->mode) {
		shader->shader->free_custom_shader(shader->custom_code_id);
		shader->custom_code_id = 0;
	}

	shader->mode = mode;

	// TODO handle all shader types
	if (mode == RS::SHADER_CANVAS_ITEM) {
		shader->shader = &canvas->state.canvas_shader;
	} else {
		return;
	}

	if (shader->custom_code_id == 0) {
		shader->custom_code_id = shader->shader->create_custom_shader();
	}

	_shader_make_dirty(shader);
}

String RasterizerStorageGLES2::shader_get_code(RID p_shader) const {
	const Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND_V(!shader, "");

	return shader->code;
}

void RasterizerStorageGLES2::_update_shader(Shader *p_shader) const {
	_shader_dirty_list.remove(&p_shader->dirty_list);

	p_shader->valid = false;

	p_shader->uniforms.clear();

	if (p_shader->code == String()) {
		return; //just invalid, but no error
	}

	ShaderCompilerGLES2::GeneratedCode gen_code;
	ShaderCompilerGLES2::IdentifierActions *actions = nullptr;

	switch (p_shader->mode) {
		case RS::SHADER_CANVAS_ITEM: {
			p_shader->canvas_item.light_mode = Shader::CanvasItem::LIGHT_MODE_NORMAL;
			p_shader->canvas_item.blend_mode = Shader::CanvasItem::BLEND_MODE_MIX;

			p_shader->canvas_item.uses_screen_texture = false;
			p_shader->canvas_item.uses_screen_uv = false;
			p_shader->canvas_item.uses_time = false;
			p_shader->canvas_item.uses_modulate = false;
			p_shader->canvas_item.uses_color = false;
			p_shader->canvas_item.uses_vertex = false;
			p_shader->canvas_item.batch_flags = 0;

			p_shader->canvas_item.uses_world_matrix = false;
			p_shader->canvas_item.uses_extra_matrix = false;
			p_shader->canvas_item.uses_projection_matrix = false;
			p_shader->canvas_item.uses_instance_custom = false;

			shaders.actions_canvas.render_mode_values["blend_add"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_ADD);
			shaders.actions_canvas.render_mode_values["blend_mix"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_MIX);
			shaders.actions_canvas.render_mode_values["blend_sub"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_SUB);
			shaders.actions_canvas.render_mode_values["blend_mul"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_MUL);
			shaders.actions_canvas.render_mode_values["blend_premul_alpha"] = Pair<int *, int>(&p_shader->canvas_item.blend_mode, Shader::CanvasItem::BLEND_MODE_PMALPHA);

			shaders.actions_canvas.render_mode_values["unshaded"] = Pair<int *, int>(&p_shader->canvas_item.light_mode, Shader::CanvasItem::LIGHT_MODE_UNSHADED);
			shaders.actions_canvas.render_mode_values["light_only"] = Pair<int *, int>(&p_shader->canvas_item.light_mode, Shader::CanvasItem::LIGHT_MODE_LIGHT_ONLY);

			shaders.actions_canvas.usage_flag_pointers["SCREEN_UV"] = &p_shader->canvas_item.uses_screen_uv;
			shaders.actions_canvas.usage_flag_pointers["SCREEN_PIXEL_SIZE"] = &p_shader->canvas_item.uses_screen_uv;
			shaders.actions_canvas.usage_flag_pointers["SCREEN_TEXTURE"] = &p_shader->canvas_item.uses_screen_texture;
			shaders.actions_canvas.usage_flag_pointers["TIME"] = &p_shader->canvas_item.uses_time;
			shaders.actions_canvas.usage_flag_pointers["MODULATE"] = &p_shader->canvas_item.uses_modulate;
			shaders.actions_canvas.usage_flag_pointers["COLOR"] = &p_shader->canvas_item.uses_color;

			shaders.actions_canvas.usage_flag_pointers["VERTEX"] = &p_shader->canvas_item.uses_vertex;

			shaders.actions_canvas.usage_flag_pointers["WORLD_MATRIX"] = &p_shader->canvas_item.uses_world_matrix;
			shaders.actions_canvas.usage_flag_pointers["EXTRA_MATRIX"] = &p_shader->canvas_item.uses_extra_matrix;
			shaders.actions_canvas.usage_flag_pointers["PROJECTION_MATRIX"] = &p_shader->canvas_item.uses_projection_matrix;
			shaders.actions_canvas.usage_flag_pointers["INSTANCE_CUSTOM"] = &p_shader->canvas_item.uses_instance_custom;

			actions = &shaders.actions_canvas;
			actions->uniforms = &p_shader->uniforms;
		} break;

		default: {
			return;
		} break;
	}

	Error err = shaders.compiler.compile(p_shader->mode, p_shader->code, actions, p_shader->path, gen_code);
	if (err != OK) {
		return;
	}

	p_shader->shader->set_custom_shader_code(p_shader->custom_code_id, gen_code.vertex, gen_code.vertex_global, gen_code.fragment, gen_code.light, gen_code.fragment_global, gen_code.uniforms, gen_code.texture_uniforms, gen_code.custom_defines);

	p_shader->texture_count = gen_code.texture_uniforms.size();
	p_shader->texture_hints = gen_code.texture_hints;

	p_shader->uses_vertex_time = gen_code.uses_vertex_time;
	p_shader->uses_fragment_time = gen_code.uses_fragment_time;

	// some logic for batching
	if (p_shader->mode == RS::SHADER_CANVAS_ITEM) {
		if (p_shader->canvas_item.uses_modulate | p_shader->canvas_item.uses_color) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_COLOR_BAKING;
		}
		if (p_shader->canvas_item.uses_vertex) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_VERTEX_BAKING;
		}
		if (p_shader->canvas_item.uses_world_matrix | p_shader->canvas_item.uses_extra_matrix | p_shader->canvas_item.uses_projection_matrix | p_shader->canvas_item.uses_instance_custom) {
			p_shader->canvas_item.batch_flags |= RasterizerStorageCommon::PREVENT_ITEM_JOINING;
		}
	}

	p_shader->shader->set_custom_shader(p_shader->custom_code_id);
	p_shader->shader->bind();

	// cache uniform locations

	for (SelfList<Material> *E = p_shader->materials.first(); E; E = E->next()) {
		_material_make_dirty(E->self());
	}

	p_shader->valid = true;
	p_shader->version++;
}

void RasterizerStorageGLES2::update_dirty_shaders() {
	while (_shader_dirty_list.first()) {
		_update_shader(_shader_dirty_list.first()->self());
	}
}

void RasterizerStorageGLES2::shader_get_param_list(RID p_shader, List<PropertyInfo> *p_param_list) const {
	Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND(!shader);

	if (shader->dirty_list.in_list()) {
		_update_shader(shader);
	}

	RBMap<int, StringName> order;

	for (RBMap<StringName, ShaderLanguage::ShaderNode::Uniform>::Element *E = shader->uniforms.front(); E; E = E->next()) {
		if (E->get().texture_order >= 0) {
			order[E->get().texture_order + 100000] = E->key();
		} else {
			order[E->get().order] = E->key();
		}
	}

	for (RBMap<int, StringName>::Element *E = order.front(); E; E = E->next()) {
		PropertyInfo pi;
		ShaderLanguage::ShaderNode::Uniform &u = shader->uniforms[E->get()];

		pi.name = E->get();

		switch (u.type) {
			case ShaderLanguage::TYPE_STRUCT: {
				pi.type = Variant::ARRAY;
			} break;
			case ShaderLanguage::TYPE_VOID: {
				pi.type = Variant::NIL;
			} break;

			case ShaderLanguage::TYPE_BOOL: {
				pi.type = Variant::BOOL;
			} break;

			// bool vectors
			case ShaderLanguage::TYPE_BVEC2: {
				pi.type = Variant::INT;
				pi.hint = PROPERTY_HINT_FLAGS;
				pi.hint_string = "x,y";
			} break;
			case ShaderLanguage::TYPE_BVEC3: {
				pi.type = Variant::INT;
				pi.hint = PROPERTY_HINT_FLAGS;
				pi.hint_string = "x,y,z";
			} break;
			case ShaderLanguage::TYPE_BVEC4: {
				pi.type = Variant::INT;
				pi.hint = PROPERTY_HINT_FLAGS;
				pi.hint_string = "x,y,z,w";
			} break;

				// int stuff
			case ShaderLanguage::TYPE_UINT:
			case ShaderLanguage::TYPE_INT: {
				pi.type = Variant::INT;

				if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_RANGE) {
					pi.hint = PROPERTY_HINT_RANGE;
					pi.hint_string = rtos(u.hint_range[0]) + "," + rtos(u.hint_range[1]) + "," + rtos(u.hint_range[2]);
				}
			} break;

			case ShaderLanguage::TYPE_IVEC2:
			case ShaderLanguage::TYPE_UVEC2:
			case ShaderLanguage::TYPE_IVEC3:
			case ShaderLanguage::TYPE_UVEC3:
			case ShaderLanguage::TYPE_IVEC4:
			case ShaderLanguage::TYPE_UVEC4: {
				pi.type = Variant::POOL_INT_ARRAY;
			} break;

			case ShaderLanguage::TYPE_FLOAT: {
				pi.type = Variant::REAL;
				if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_RANGE) {
					pi.hint = PROPERTY_HINT_RANGE;
					pi.hint_string = rtos(u.hint_range[0]) + "," + rtos(u.hint_range[1]) + "," + rtos(u.hint_range[2]);
				}
			} break;

			case ShaderLanguage::TYPE_VEC2: {
				pi.type = Variant::VECTOR2;
			} break;
			case ShaderLanguage::TYPE_VEC3: {
				pi.type = Variant::VECTOR3;
			} break;

			case ShaderLanguage::TYPE_VEC4: {
				if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_COLOR) {
					pi.type = Variant::COLOR;
				} else {
					pi.type = Variant::PLANE;
				}
			} break;

			case ShaderLanguage::TYPE_MAT2: {
				pi.type = Variant::TRANSFORM2D;
			} break;

			case ShaderLanguage::TYPE_MAT3: {
				pi.type = Variant::BASIS;
			} break;

			case ShaderLanguage::TYPE_MAT4: {
				pi.type = Variant::TRANSFORM;
			} break;

			case ShaderLanguage::TYPE_SAMPLER2D:
			case ShaderLanguage::TYPE_SAMPLEREXT:
			case ShaderLanguage::TYPE_ISAMPLER2D:
			case ShaderLanguage::TYPE_USAMPLER2D: {
				pi.type = Variant::OBJECT;
				pi.hint = PROPERTY_HINT_RESOURCE_TYPE;
				pi.hint_string = "Texture";
			} break;

			case ShaderLanguage::TYPE_SAMPLERCUBE: {
				pi.type = Variant::OBJECT;
				pi.hint = PROPERTY_HINT_RESOURCE_TYPE;
				pi.hint_string = "CubeMap";
			} break;

			case ShaderLanguage::TYPE_SAMPLER2DARRAY:
			case ShaderLanguage::TYPE_ISAMPLER2DARRAY:
			case ShaderLanguage::TYPE_USAMPLER2DARRAY:
			case ShaderLanguage::TYPE_SAMPLER3D:
			case ShaderLanguage::TYPE_ISAMPLER3D:
			case ShaderLanguage::TYPE_USAMPLER3D: {
				// Not implemented in GLES2
			} break;

			default: {
			}
		}

		p_param_list->push_back(pi);
	}
}

void RasterizerStorageGLES2::shader_set_default_texture_param(RID p_shader, const StringName &p_name, RID p_texture) {
	Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND(!shader);
	ERR_FAIL_COND(p_texture.is_valid() && !texture_owner.owns(p_texture));

	if (p_texture.is_valid()) {
		shader->default_textures[p_name] = p_texture;
	} else {
		shader->default_textures.erase(p_name);
	}

	_shader_make_dirty(shader);
}

RID RasterizerStorageGLES2::shader_get_default_texture_param(RID p_shader, const StringName &p_name) const {
	const Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND_V(!shader, RID());

	const RBMap<StringName, RID>::Element *E = shader->default_textures.find(p_name);

	if (!E) {
		return RID();
	}

	return E->get();
}

void RasterizerStorageGLES2::shader_add_custom_define(RID p_shader, const String &p_define) {
	Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND(!shader);

	shader->shader->add_custom_define(p_define);

	_shader_make_dirty(shader);
}

void RasterizerStorageGLES2::shader_get_custom_defines(RID p_shader, Vector<String> *p_defines) const {
	Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND(!shader);

	shader->shader->get_custom_defines(p_defines);
}

void RasterizerStorageGLES2::shader_remove_custom_define(RID p_shader, const String &p_define) {
	Shader *shader = shader_owner.get(p_shader);
	ERR_FAIL_COND(!shader);

	shader->shader->remove_custom_define(p_define);

	_shader_make_dirty(shader);
}

/* COMMON MATERIAL API */

void RasterizerStorageGLES2::_material_make_dirty(Material *p_material) const {
	if (p_material->dirty_list.in_list()) {
		return;
	}

	_material_dirty_list.add(&p_material->dirty_list);
}

RID RasterizerStorageGLES2::material_create() {
	Material *material = memnew(Material);

	return material_owner.make_rid(material);
}

void RasterizerStorageGLES2::material_set_shader(RID p_material, RID p_shader) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND(!material);

	Shader *shader = shader_owner.getornull(p_shader);

	if (material->shader) {
		// if a shader is present, remove the old shader
		material->shader->materials.remove(&material->list);
	}

	material->shader = shader;

	if (shader) {
		shader->materials.add(&material->list);
	}

	_material_make_dirty(material);
}

RID RasterizerStorageGLES2::material_get_shader(RID p_material) const {
	const Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, RID());

	if (material->shader) {
		return material->shader->self;
	}

	return RID();
}

void RasterizerStorageGLES2::material_set_param(RID p_material, const StringName &p_param, const Variant &p_value) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND(!material);

	if (p_value.get_type() == Variant::NIL) {
		material->params.erase(p_param);
	} else {
		material->params[p_param] = p_value;
	}

	_material_make_dirty(material);
}

Variant RasterizerStorageGLES2::material_get_param(RID p_material, const StringName &p_param) const {
	const Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, RID());

	if (material->params.has(p_param)) {
		return material->params[p_param];
	}

	return material_get_param_default(p_material, p_param);
}

Variant RasterizerStorageGLES2::material_get_param_default(RID p_material, const StringName &p_param) const {
	const Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, Variant());

	if (material->shader) {
		if (material->shader->uniforms.has(p_param)) {
			ShaderLanguage::ShaderNode::Uniform uniform = material->shader->uniforms[p_param];
			Vector<ShaderLanguage::ConstantNode::Value> default_value = uniform.default_value;
			return ShaderLanguage::constant_value_to_variant(default_value, uniform.type, uniform.hint);
		}
	}
	return Variant();
}

void RasterizerStorageGLES2::material_set_line_width(RID p_material, float p_width) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	material->line_width = p_width;
}

void RasterizerStorageGLES2::material_set_next_pass(RID p_material, RID p_next_material) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND(!material);

	material->next_pass = p_next_material;
}

bool RasterizerStorageGLES2::material_is_animated(RID p_material) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, false);
	if (material->dirty_list.in_list()) {
		_update_material(material);
	}

	bool animated = material->is_animated_cache;
	if (!animated && material->next_pass.is_valid()) {
		animated = material_is_animated(material->next_pass);
	}
	return animated;
}

bool RasterizerStorageGLES2::material_casts_shadows(RID p_material) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, false);
	if (material->dirty_list.in_list()) {
		_update_material(material);
	}

	bool casts_shadows = material->can_cast_shadow_cache;

	if (!casts_shadows && material->next_pass.is_valid()) {
		casts_shadows = material_casts_shadows(material->next_pass);
	}

	return casts_shadows;
}

bool RasterizerStorageGLES2::material_uses_tangents(RID p_material) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, false);

	if (!material->shader) {
		return false;
	}

	if (material->shader->dirty_list.in_list()) {
		_update_shader(material->shader);
	}

	return material->shader->spatial.uses_tangent;
}

bool RasterizerStorageGLES2::material_uses_ensure_correct_normals(RID p_material) {
	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND_V(!material, false);

	if (!material->shader) {
		return false;
	}

	if (material->shader->dirty_list.in_list()) {
		_update_shader(material->shader);
	}

	return material->shader->spatial.uses_ensure_correct_normals;
}

void RasterizerStorageGLES2::material_add_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	RBMap<RasterizerScene::InstanceBase *, int>::Element *E = material->instance_owners.find(p_instance);
	if (E) {
		E->get()++;
	} else {
		material->instance_owners[p_instance] = 1;
	}
}

void RasterizerStorageGLES2::material_remove_instance_owner(RID p_material, RasterizerScene::InstanceBase *p_instance) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	RBMap<RasterizerScene::InstanceBase *, int>::Element *E = material->instance_owners.find(p_instance);
	ERR_FAIL_COND(!E);

	E->get()--;

	if (E->get() == 0) {
		material->instance_owners.erase(E);
	}
}

void RasterizerStorageGLES2::material_set_render_priority(RID p_material, int priority) {
	ERR_FAIL_COND(priority < RS::MATERIAL_RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(priority > RS::MATERIAL_RENDER_PRIORITY_MAX);

	Material *material = material_owner.get(p_material);
	ERR_FAIL_COND(!material);

	material->render_priority = priority;
}

void RasterizerStorageGLES2::_update_material(Material *p_material) {
	if (p_material->dirty_list.in_list()) {
		_material_dirty_list.remove(&p_material->dirty_list);
	}

	if (p_material->shader && p_material->shader->dirty_list.in_list()) {
		_update_shader(p_material->shader);
	}

	if (p_material->shader && !p_material->shader->valid) {
		return;
	}

	// uniforms and other things will be set in the use_material method in ShaderGLES2

	if (p_material->shader && p_material->shader->texture_count > 0) {
		p_material->textures.resize(p_material->shader->texture_count);

		for (RBMap<StringName, ShaderLanguage::ShaderNode::Uniform>::Element *E = p_material->shader->uniforms.front(); E; E = E->next()) {
			if (E->get().texture_order < 0) {
				continue; // not a texture, does not go here
			}

			RID texture;

			RBMap<StringName, Variant>::Element *V = p_material->params.find(E->key());

			if (V) {
				texture = V->get();
			}

			if (!texture.is_valid()) {
				RBMap<StringName, RID>::Element *W = p_material->shader->default_textures.find(E->key());

				if (W) {
					texture = W->get();
				}
			}

			p_material->textures.write[E->get().texture_order] = Pair<StringName, RID>(E->key(), texture);
		}
	} else {
		p_material->textures.clear();
	}
}

void RasterizerStorageGLES2::_material_add_geometry(RID p_material, Geometry *p_geometry) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	RBMap<Geometry *, int>::Element *I = material->geometry_owners.find(p_geometry);

	if (I) {
		I->get()++;
	} else {
		material->geometry_owners[p_geometry] = 1;
	}
}

void RasterizerStorageGLES2::_material_remove_geometry(RID p_material, Geometry *p_geometry) {
	Material *material = material_owner.getornull(p_material);
	ERR_FAIL_COND(!material);

	RBMap<Geometry *, int>::Element *I = material->geometry_owners.find(p_geometry);
	ERR_FAIL_COND(!I);

	I->get()--;

	if (I->get() == 0) {
		material->geometry_owners.erase(I);
	}
}

void RasterizerStorageGLES2::update_dirty_materials() {
	while (_material_dirty_list.first()) {
		Material *material = _material_dirty_list.first()->self();
		_update_material(material);
	}
}

/* MESH API */

RID RasterizerStorageGLES2::mesh_create() {
	Mesh *mesh = memnew(Mesh);

	return mesh_owner.make_rid(mesh);
}

static PoolVector<uint8_t> _unpack_half_floats(const PoolVector<uint8_t> &array, uint32_t &format, int p_vertices) {
	uint32_t p_format = format;

	static int src_size[RS::ARRAY_MAX];
	static int dst_size[RS::ARRAY_MAX];
	static int to_convert[RS::ARRAY_MAX];

	int src_stride = 0;
	int dst_stride = 0;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		to_convert[i] = 0;
		if (!(p_format & (1 << i))) {
			src_size[i] = 0;
			dst_size[i] = 0;
			continue;
		}

		switch (i) {
			case RS::ARRAY_VERTEX: {
				if (p_format & RS::ARRAY_COMPRESS_VERTEX) {
					if (p_format & RS::ARRAY_FLAG_USE_2D_VERTICES) {
						src_size[i] = 4;
						dst_size[i] = 8;
						to_convert[i] = 2;
					} else {
						src_size[i] = 8;
						dst_size[i] = 12;
						to_convert[i] = 3;
					}

					format &= ~RS::ARRAY_COMPRESS_VERTEX;
				} else {
					if (p_format & RS::ARRAY_FLAG_USE_2D_VERTICES) {
						src_size[i] = 8;
						dst_size[i] = 8;
					} else {
						src_size[i] = 12;
						dst_size[i] = 12;
					}
				}

			} break;
			case RS::ARRAY_NORMAL: {
				if (p_format & RS::ARRAY_COMPRESS_NORMAL) {
					src_size[i] = 4;
					dst_size[i] = 4;
				} else {
					src_size[i] = 12;
					dst_size[i] = 12;
				}

			} break;
			case RS::ARRAY_TANGENT: {
				if (p_format & RS::ARRAY_COMPRESS_TANGENT) {
					src_size[i] = 4;
					dst_size[i] = 4;
				} else {
					src_size[i] = 16;
					dst_size[i] = 16;
				}

			} break;
			case RS::ARRAY_COLOR: {
				if (p_format & RS::ARRAY_COMPRESS_COLOR) {
					src_size[i] = 4;
					dst_size[i] = 4;
				} else {
					src_size[i] = 16;
					dst_size[i] = 16;
				}

			} break;
			case RS::ARRAY_TEX_UV: {
				if (p_format & RS::ARRAY_COMPRESS_TEX_UV) {
					src_size[i] = 4;
					to_convert[i] = 2;
					format &= ~RS::ARRAY_COMPRESS_TEX_UV;
				} else {
					src_size[i] = 8;
				}

				dst_size[i] = 8;

			} break;
			case RS::ARRAY_TEX_UV2: {
				if (p_format & RS::ARRAY_COMPRESS_TEX_UV2) {
					src_size[i] = 4;
					to_convert[i] = 2;
					format &= ~RS::ARRAY_COMPRESS_TEX_UV2;
				} else {
					src_size[i] = 8;
				}

				dst_size[i] = 8;

			} break;
			case RS::ARRAY_INDEX: {
				src_size[i] = 0;
				dst_size[i] = 0;

			} break;
		}

		src_stride += src_size[i];
		dst_stride += dst_size[i];
	}

	PoolVector<uint8_t> ret;
	ret.resize(p_vertices * dst_stride);

	PoolVector<uint8_t>::Read r = array.read();
	PoolVector<uint8_t>::Write w = ret.write();

	int src_offset = 0;
	int dst_offset = 0;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		if (src_size[i] == 0) {
			continue; //no go
		}
		const uint8_t *rptr = r.ptr();
		uint8_t *wptr = w.ptr();
		if (to_convert[i]) { //converting

			for (int j = 0; j < p_vertices; j++) {
				const uint16_t *src = (const uint16_t *)&rptr[src_stride * j + src_offset];
				float *dst = (float *)&wptr[dst_stride * j + dst_offset];

				for (int k = 0; k < to_convert[i]; k++) {
					dst[k] = Math::half_to_float(src[k]);
				}
			}

		} else {
			//just copy
			for (int j = 0; j < p_vertices; j++) {
				for (int k = 0; k < src_size[i]; k++) {
					wptr[dst_stride * j + dst_offset + k] = rptr[src_stride * j + src_offset + k];
				}
			}
		}

		src_offset += src_size[i];
		dst_offset += dst_size[i];
	}

	r.release();
	w.release();

	return ret;
}

void RasterizerStorageGLES2::mesh_add_surface(RID p_mesh, uint32_t p_format, RS::PrimitiveType p_primitive, const PoolVector<uint8_t> &p_array, int p_vertex_count, const PoolVector<uint8_t> &p_index_array, int p_index_count, const AABB &p_aabb, const Vector<PoolVector<uint8_t>> &p_blend_shapes, const Vector<AABB> &p_bone_aabbs) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);

	ERR_FAIL_COND(!(p_format & RS::ARRAY_FORMAT_VERTEX));

	//bool has_morph = p_blend_shapes.size();
	bool use_split_stream = GLOBAL_GET("rendering/misc/mesh_storage/split_stream") && !(p_format & RS::ARRAY_FLAG_USE_DYNAMIC_UPDATE);

	Surface::Attrib attribs[RS::ARRAY_MAX];

	int attributes_base_offset = 0;
	int attributes_stride = 0;
	int positions_stride = 0;
	bool uses_half_float = false;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		attribs[i].index = i;

		if (!(p_format & (1 << i))) {
			attribs[i].enabled = false;
			attribs[i].integer = false;
			continue;
		}

		attribs[i].enabled = true;
		attribs[i].offset = attributes_base_offset + attributes_stride;
		attribs[i].integer = false;

		switch (i) {
			case RS::ARRAY_VERTEX: {
				if (p_format & RS::ARRAY_FLAG_USE_2D_VERTICES) {
					attribs[i].size = 2;
				} else {
					attribs[i].size = (p_format & RS::ARRAY_COMPRESS_VERTEX) ? 4 : 3;
				}

				if (p_format & RS::ARRAY_COMPRESS_VERTEX) {
					attribs[i].type = _GL_HALF_FLOAT_OES;
					positions_stride += attribs[i].size * 2;
					uses_half_float = true;
				} else {
					attribs[i].type = GL_FLOAT;
					positions_stride += attribs[i].size * 4;
				}

				attribs[i].normalized = GL_FALSE;

				if (use_split_stream) {
					attributes_base_offset = positions_stride * p_vertex_count;
				} else {
					attributes_base_offset = positions_stride;
				}

			} break;
			case RS::ARRAY_NORMAL: {
				attribs[i].size = 3;

				if (p_format & RS::ARRAY_COMPRESS_NORMAL) {
					attribs[i].type = GL_BYTE;
					attributes_stride += 4; //pad extra byte
					attribs[i].normalized = GL_TRUE;
				} else {
					attribs[i].type = GL_FLOAT;
					attributes_stride += 12;
					attribs[i].normalized = GL_FALSE;
				}

			} break;
			case RS::ARRAY_TANGENT: {
				attribs[i].size = 4;

				if (p_format & RS::ARRAY_COMPRESS_TANGENT) {
					attribs[i].type = GL_BYTE;
					attributes_stride += 4;
					attribs[i].normalized = GL_TRUE;
				} else {
					attribs[i].type = GL_FLOAT;
					attributes_stride += 16;
					attribs[i].normalized = GL_FALSE;
				}

			} break;
			case RS::ARRAY_COLOR: {
				attribs[i].size = 4;

				if (p_format & RS::ARRAY_COMPRESS_COLOR) {
					attribs[i].type = GL_UNSIGNED_BYTE;
					attributes_stride += 4;
					attribs[i].normalized = GL_TRUE;
				} else {
					attribs[i].type = GL_FLOAT;
					attributes_stride += 16;
					attribs[i].normalized = GL_FALSE;
				}

			} break;
			case RS::ARRAY_TEX_UV: {
				attribs[i].size = 2;

				if (p_format & RS::ARRAY_COMPRESS_TEX_UV) {
					attribs[i].type = _GL_HALF_FLOAT_OES;
					attributes_stride += 4;
					uses_half_float = true;
				} else {
					attribs[i].type = GL_FLOAT;
					attributes_stride += 8;
				}

				attribs[i].normalized = GL_FALSE;

			} break;
			case RS::ARRAY_TEX_UV2: {
				attribs[i].size = 2;

				if (p_format & RS::ARRAY_COMPRESS_TEX_UV2) {
					attribs[i].type = _GL_HALF_FLOAT_OES;
					attributes_stride += 4;
					uses_half_float = true;
				} else {
					attribs[i].type = GL_FLOAT;
					attributes_stride += 8;
				}
				attribs[i].normalized = GL_FALSE;

			} break;
			case RS::ARRAY_INDEX: {
				attribs[i].size = 1;

				if (p_vertex_count >= (1 << 16)) {
					attribs[i].type = GL_UNSIGNED_INT;
					attribs[i].stride = 4;
				} else {
					attribs[i].type = GL_UNSIGNED_SHORT;
					attribs[i].stride = 2;
				}

				attribs[i].normalized = GL_FALSE;

			} break;
		}
	}

	if (use_split_stream) {
		attribs[RS::ARRAY_VERTEX].stride = positions_stride;
		for (int i = 1; i < RS::ARRAY_MAX - 1; i++) {
			attribs[i].stride = attributes_stride;
		}
	} else {
		for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
			attribs[i].stride = positions_stride + attributes_stride;
		}
	}

	//validate sizes
	PoolVector<uint8_t> array = p_array;

	int stride = positions_stride + attributes_stride;
	int array_size = stride * p_vertex_count;
	int index_array_size = 0;
	if (array.size() != array_size && array.size() + p_vertex_count * 2 == array_size) {
		//old format, convert
		array = PoolVector<uint8_t>();

		array.resize(p_array.size() + p_vertex_count * 2);

		PoolVector<uint8_t>::Write w = array.write();
		PoolVector<uint8_t>::Read r = p_array.read();

		uint16_t *w16 = (uint16_t *)w.ptr();
		const uint16_t *r16 = (uint16_t *)r.ptr();

		uint16_t one = Math::make_half_float(1);

		for (int i = 0; i < p_vertex_count; i++) {
			*w16++ = *r16++;
			*w16++ = *r16++;
			*w16++ = *r16++;
			*w16++ = one;
			for (int j = 0; j < (stride / 2) - 4; j++) {
				*w16++ = *r16++;
			}
		}
	}

	ERR_FAIL_COND(array.size() != array_size);

	if (!config.support_half_float_vertices && uses_half_float) {
		uint32_t new_format = p_format;
		PoolVector<uint8_t> unpacked_array = _unpack_half_floats(array, new_format, p_vertex_count);

		Vector<PoolVector<uint8_t>> unpacked_blend_shapes;
		for (int i = 0; i < p_blend_shapes.size(); i++) {
			uint32_t temp_format = p_format; // Just throw this away as it will be the same as new_format
			unpacked_blend_shapes.push_back(_unpack_half_floats(p_blend_shapes[i], temp_format, p_vertex_count));
		}

		mesh_add_surface(p_mesh, new_format, p_primitive, unpacked_array, p_vertex_count, p_index_array, p_index_count, p_aabb, unpacked_blend_shapes, p_bone_aabbs);
		return; //do not go any further, above function used unpacked stuff will be used instead.
	}

	if (p_format & RS::ARRAY_FORMAT_INDEX) {
		index_array_size = attribs[RS::ARRAY_INDEX].stride * p_index_count;
	}

	ERR_FAIL_COND(p_index_array.size() != index_array_size);

	ERR_FAIL_COND(p_blend_shapes.size() != mesh->blend_shape_count);

	for (int i = 0; i < p_blend_shapes.size(); i++) {
		ERR_FAIL_COND(p_blend_shapes[i].size() != array_size);
	}

	// all valid, create stuff

	Surface *surface = memnew(Surface);

	surface->active = true;
	surface->array_len = p_vertex_count;
	surface->index_array_len = p_index_count;
	surface->array_byte_size = array.size();
	surface->index_array_byte_size = p_index_array.size();
	surface->primitive = p_primitive;
	surface->mesh = mesh;
	surface->format = p_format;

	surface->aabb = p_aabb;
	surface->max_bone = p_bone_aabbs.size();
	surface->blend_shape_data = p_blend_shapes;

	surface->data = array;
	surface->index_data = p_index_array;
	surface->total_data_size += surface->array_byte_size + surface->index_array_byte_size;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		surface->attribs[i] = attribs[i];
	}

	// Okay, now the OpenGL stuff, wheeeeey \o/
	{
		PoolVector<uint8_t>::Read vr = array.read();

		glGenBuffers(1, &surface->vertex_id);
		glBindBuffer(GL_ARRAY_BUFFER, surface->vertex_id);
		glBufferData(GL_ARRAY_BUFFER, array_size, vr.ptr(), (p_format & RS::ARRAY_FLAG_USE_DYNAMIC_UPDATE) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		if (p_format & RS::ARRAY_FORMAT_INDEX) {
			PoolVector<uint8_t>::Read ir = p_index_array.read();

			glGenBuffers(1, &surface->index_id);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, surface->index_id);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_array_size, ir.ptr(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		} else {
			surface->index_id = 0;
		}

		// TODO generate wireframes

		// Make one blend shape buffer per surface
		{
			surface->blend_shape_buffer_size = 0;
			glGenBuffers(1, &surface->blend_shape_buffer_id);
		}
	}

	mesh->surfaces.push_back(surface);
	mesh->instance_change_notify(true, true);

	info.vertex_mem += surface->total_data_size;
}

void RasterizerStorageGLES2::mesh_set_blend_shape_count(RID p_mesh, int p_amount) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);

	ERR_FAIL_COND(mesh->surfaces.size() != 0);
	ERR_FAIL_COND(p_amount < 0);

	mesh->blend_shape_count = p_amount;
	mesh->instance_change_notify(true, false);
	if (!mesh->update_list.in_list()) {
		blend_shapes_update_list.add(&mesh->update_list);
	}
}

int RasterizerStorageGLES2::mesh_get_blend_shape_count(RID p_mesh) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, 0);
	return mesh->blend_shape_count;
}

void RasterizerStorageGLES2::mesh_set_blend_shape_mode(RID p_mesh, RS::BlendShapeMode p_mode) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);

	mesh->blend_shape_mode = p_mode;
	if (!mesh->update_list.in_list()) {
		blend_shapes_update_list.add(&mesh->update_list);
	}
}

RS::BlendShapeMode RasterizerStorageGLES2::mesh_get_blend_shape_mode(RID p_mesh) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, RS::BLEND_SHAPE_MODE_NORMALIZED);

	return mesh->blend_shape_mode;
}

void RasterizerStorageGLES2::mesh_set_blend_shape_values(RID p_mesh, PoolVector<float> p_values) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);

	mesh->blend_shape_values = p_values;
	if (!mesh->update_list.in_list()) {
		blend_shapes_update_list.add(&mesh->update_list);
	}
}

PoolVector<float> RasterizerStorageGLES2::mesh_get_blend_shape_values(RID p_mesh) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, PoolVector<float>());

	return mesh->blend_shape_values;
}

void RasterizerStorageGLES2::mesh_surface_update_region(RID p_mesh, int p_surface, int p_offset, const PoolVector<uint8_t> &p_data) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);

	ERR_FAIL_COND(!mesh);
	ERR_FAIL_INDEX(p_surface, mesh->surfaces.size());

	int total_size = p_data.size();
	ERR_FAIL_COND(p_offset + total_size > mesh->surfaces[p_surface]->array_byte_size);

	PoolVector<uint8_t>::Read r = p_data.read();

	glBindBuffer(GL_ARRAY_BUFFER, mesh->surfaces[p_surface]->vertex_id);
	glBufferSubData(GL_ARRAY_BUFFER, p_offset, total_size, r.ptr());
	glBindBuffer(GL_ARRAY_BUFFER, 0); //unbind
}

void RasterizerStorageGLES2::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);
	ERR_FAIL_INDEX(p_surface, mesh->surfaces.size());

	if (mesh->surfaces[p_surface]->material == p_material) {
		return;
	}

	if (mesh->surfaces[p_surface]->material.is_valid()) {
		_material_remove_geometry(mesh->surfaces[p_surface]->material, mesh->surfaces[p_surface]);
	}

	mesh->surfaces[p_surface]->material = p_material;

	if (mesh->surfaces[p_surface]->material.is_valid()) {
		_material_add_geometry(mesh->surfaces[p_surface]->material, mesh->surfaces[p_surface]);
	}

	mesh->instance_change_notify(false, true);
}

RID RasterizerStorageGLES2::mesh_surface_get_material(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, RID());
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), RID());

	return mesh->surfaces[p_surface]->material;
}

int RasterizerStorageGLES2::mesh_surface_get_array_len(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, 0);
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), 0);

	return mesh->surfaces[p_surface]->array_len;
}

int RasterizerStorageGLES2::mesh_surface_get_array_index_len(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, 0);
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), 0);

	return mesh->surfaces[p_surface]->index_array_len;
}

PoolVector<uint8_t> RasterizerStorageGLES2::mesh_surface_get_array(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, PoolVector<uint8_t>());
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), PoolVector<uint8_t>());

	Surface *surface = mesh->surfaces[p_surface];

	return surface->data;
}

PoolVector<uint8_t> RasterizerStorageGLES2::mesh_surface_get_index_array(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, PoolVector<uint8_t>());
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), PoolVector<uint8_t>());

	Surface *surface = mesh->surfaces[p_surface];

	return surface->index_data;
}

uint32_t RasterizerStorageGLES2::mesh_surface_get_format(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);

	ERR_FAIL_COND_V(!mesh, 0);
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), 0);

	return mesh->surfaces[p_surface]->format;
}

RS::PrimitiveType RasterizerStorageGLES2::mesh_surface_get_primitive_type(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, RS::PRIMITIVE_MAX);
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), RS::PRIMITIVE_MAX);

	return mesh->surfaces[p_surface]->primitive;
}

AABB RasterizerStorageGLES2::mesh_surface_get_aabb(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, AABB());
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), AABB());

	return mesh->surfaces[p_surface]->aabb;
}

Vector<PoolVector<uint8_t>> RasterizerStorageGLES2::mesh_surface_get_blend_shapes(RID p_mesh, int p_surface) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, Vector<PoolVector<uint8_t>>());
	ERR_FAIL_INDEX_V(p_surface, mesh->surfaces.size(), Vector<PoolVector<uint8_t>>());

	return mesh->surfaces[p_surface]->blend_shape_data;
}

void RasterizerStorageGLES2::mesh_remove_surface(RID p_mesh, int p_surface) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);
	ERR_FAIL_INDEX(p_surface, mesh->surfaces.size());

	Surface *surface = mesh->surfaces[p_surface];

	if (surface->material.is_valid()) {
		_material_remove_geometry(surface->material, mesh->surfaces[p_surface]);
	}

	glDeleteBuffers(1, &surface->vertex_id);
	if (surface->index_id) {
		glDeleteBuffers(1, &surface->index_id);
	}

	glDeleteBuffers(1, &surface->blend_shape_buffer_id);

	info.vertex_mem -= surface->total_data_size;

	memdelete(surface);

	mesh->surfaces.remove(p_surface);

	mesh->instance_change_notify(true, true);
}

int RasterizerStorageGLES2::mesh_get_surface_count(RID p_mesh) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, 0);
	return mesh->surfaces.size();
}

void RasterizerStorageGLES2::mesh_set_custom_aabb(RID p_mesh, const AABB &p_aabb) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);

	mesh->custom_aabb = p_aabb;
	mesh->instance_change_notify(true, false);
}

AABB RasterizerStorageGLES2::mesh_get_custom_aabb(RID p_mesh) const {
	const Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND_V(!mesh, AABB());

	return mesh->custom_aabb;
}

AABB RasterizerStorageGLES2::mesh_get_aabb(RID p_mesh) const {
	Mesh *mesh = mesh_owner.get(p_mesh);
	ERR_FAIL_COND_V(!mesh, AABB());

	if (mesh->custom_aabb != AABB()) {
		return mesh->custom_aabb;
	}

	AABB aabb;

	for (int i = 0; i < mesh->surfaces.size(); i++) {
		if (i == 0) {
			aabb = mesh->surfaces[i]->aabb;
		} else {
			aabb.merge_with(mesh->surfaces[i]->aabb);
		}
	}

	return aabb;
}
void RasterizerStorageGLES2::mesh_clear(RID p_mesh) {
	Mesh *mesh = mesh_owner.getornull(p_mesh);
	ERR_FAIL_COND(!mesh);

	while (mesh->surfaces.size()) {
		mesh_remove_surface(p_mesh, 0);
	}
}

/* MULTIMESH API */

RID RasterizerStorageGLES2::_multimesh_create() {
	MultiMesh *multimesh = memnew(MultiMesh);
	return multimesh_owner.make_rid(multimesh);
}

void RasterizerStorageGLES2::_multimesh_allocate(RID p_multimesh, int p_instances, RS::MultimeshTransformFormat p_transform_format, RS::MultimeshColorFormat p_color_format, RS::MultimeshCustomDataFormat p_data) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);

	if (multimesh->size == p_instances && multimesh->transform_format == p_transform_format && multimesh->color_format == p_color_format && multimesh->custom_data_format == p_data) {
		return;
	}

	multimesh->size = p_instances;

	multimesh->color_format = p_color_format;
	multimesh->transform_format = p_transform_format;
	multimesh->custom_data_format = p_data;

	if (multimesh->size) {
		multimesh->data.resize(0);
	}

	if (multimesh->transform_format == RS::MULTIMESH_TRANSFORM_2D) {
		multimesh->xform_floats = 8;
	} else {
		multimesh->xform_floats = 12;
	}

	if (multimesh->color_format == RS::MULTIMESH_COLOR_8BIT) {
		multimesh->color_floats = 1;
	} else if (multimesh->color_format == RS::MULTIMESH_COLOR_FLOAT) {
		multimesh->color_floats = 4;
	} else {
		multimesh->color_floats = 0;
	}

	if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_8BIT) {
		multimesh->custom_data_floats = 1;
	} else if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_FLOAT) {
		multimesh->custom_data_floats = 4;
	} else {
		multimesh->custom_data_floats = 0;
	}

	int format_floats = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;

	multimesh->data.resize(format_floats * p_instances);

	for (int i = 0; i < p_instances * format_floats; i += format_floats) {
		int color_from = 0;
		int custom_data_from = 0;

		if (multimesh->transform_format == RS::MULTIMESH_TRANSFORM_2D) {
			multimesh->data.write[i + 0] = 1.0;
			multimesh->data.write[i + 1] = 0.0;
			multimesh->data.write[i + 2] = 0.0;
			multimesh->data.write[i + 3] = 0.0;
			multimesh->data.write[i + 4] = 0.0;
			multimesh->data.write[i + 5] = 1.0;
			multimesh->data.write[i + 6] = 0.0;
			multimesh->data.write[i + 7] = 0.0;
			color_from = 8;
			custom_data_from = 8;
		} else {
			multimesh->data.write[i + 0] = 1.0;
			multimesh->data.write[i + 1] = 0.0;
			multimesh->data.write[i + 2] = 0.0;
			multimesh->data.write[i + 3] = 0.0;
			multimesh->data.write[i + 4] = 0.0;
			multimesh->data.write[i + 5] = 1.0;
			multimesh->data.write[i + 6] = 0.0;
			multimesh->data.write[i + 7] = 0.0;
			multimesh->data.write[i + 8] = 0.0;
			multimesh->data.write[i + 9] = 0.0;
			multimesh->data.write[i + 10] = 1.0;
			multimesh->data.write[i + 11] = 0.0;
			color_from = 12;
			custom_data_from = 12;
		}

		if (multimesh->color_format == RS::MULTIMESH_COLOR_8BIT) {
			union {
				uint32_t colu;
				float colf;
			} cu;

			cu.colu = 0xFFFFFFFF;
			multimesh->data.write[i + color_from + 0] = cu.colf;
			custom_data_from = color_from + 1;
		} else if (multimesh->color_format == RS::MULTIMESH_COLOR_FLOAT) {
			multimesh->data.write[i + color_from + 0] = 1.0;
			multimesh->data.write[i + color_from + 1] = 1.0;
			multimesh->data.write[i + color_from + 2] = 1.0;
			multimesh->data.write[i + color_from + 3] = 1.0;
			custom_data_from = color_from + 4;
		}

		if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_8BIT) {
			union {
				uint32_t colu;
				float colf;
			} cu;

			cu.colu = 0;
			multimesh->data.write[i + custom_data_from + 0] = cu.colf;
		} else if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_FLOAT) {
			multimesh->data.write[i + custom_data_from + 0] = 0.0;
			multimesh->data.write[i + custom_data_from + 1] = 0.0;
			multimesh->data.write[i + custom_data_from + 2] = 0.0;
			multimesh->data.write[i + custom_data_from + 3] = 0.0;
		}
	}

	multimesh->dirty_aabb = true;
	multimesh->dirty_data = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

int RasterizerStorageGLES2::_multimesh_get_instance_count(RID p_multimesh) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, 0);

	return multimesh->size;
}

void RasterizerStorageGLES2::_multimesh_set_mesh(RID p_multimesh, RID p_mesh) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);

	if (multimesh->mesh.is_valid()) {
		Mesh *mesh = mesh_owner.getornull(multimesh->mesh);
		if (mesh) {
			mesh->multimeshes.remove(&multimesh->mesh_list);
		}
	}

	multimesh->mesh = p_mesh;

	if (multimesh->mesh.is_valid()) {
		Mesh *mesh = mesh_owner.getornull(multimesh->mesh);
		if (mesh) {
			mesh->multimeshes.add(&multimesh->mesh_list);
		}
	}

	multimesh->dirty_aabb = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

void RasterizerStorageGLES2::_multimesh_instance_set_transform(RID p_multimesh, int p_index, const Transform &p_transform) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->size);
	ERR_FAIL_COND(multimesh->transform_format == RS::MULTIMESH_TRANSFORM_2D);

	int stride = multimesh->color_floats + multimesh->custom_data_floats + multimesh->xform_floats;

	float *dataptr = &multimesh->data.write[stride * p_index];

	dataptr[0] = p_transform.basis.rows[0][0];
	dataptr[1] = p_transform.basis.rows[0][1];
	dataptr[2] = p_transform.basis.rows[0][2];
	dataptr[3] = p_transform.origin.x;
	dataptr[4] = p_transform.basis.rows[1][0];
	dataptr[5] = p_transform.basis.rows[1][1];
	dataptr[6] = p_transform.basis.rows[1][2];
	dataptr[7] = p_transform.origin.y;
	dataptr[8] = p_transform.basis.rows[2][0];
	dataptr[9] = p_transform.basis.rows[2][1];
	dataptr[10] = p_transform.basis.rows[2][2];
	dataptr[11] = p_transform.origin.z;

	multimesh->dirty_data = true;
	multimesh->dirty_aabb = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

void RasterizerStorageGLES2::_multimesh_instance_set_transform_2d(RID p_multimesh, int p_index, const Transform2D &p_transform) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->size);
	ERR_FAIL_COND(multimesh->transform_format == RS::MULTIMESH_TRANSFORM_3D);

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index];

	dataptr[0] = p_transform.columns[0][0];
	dataptr[1] = p_transform.columns[1][0];
	dataptr[2] = 0;
	dataptr[3] = p_transform.columns[2][0];
	dataptr[4] = p_transform.columns[0][1];
	dataptr[5] = p_transform.columns[1][1];
	dataptr[6] = 0;
	dataptr[7] = p_transform.columns[2][1];

	multimesh->dirty_data = true;
	multimesh->dirty_aabb = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

void RasterizerStorageGLES2::_multimesh_instance_set_color(RID p_multimesh, int p_index, const Color &p_color) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->size);
	ERR_FAIL_COND(multimesh->color_format == RS::MULTIMESH_COLOR_NONE);
	ERR_FAIL_INDEX(multimesh->color_format, RS::MULTIMESH_COLOR_MAX);

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index + multimesh->xform_floats];

	if (multimesh->color_format == RS::MULTIMESH_COLOR_8BIT) {
		uint8_t *data8 = (uint8_t *)dataptr;
		data8[0] = CLAMP(p_color.r * 255.0, 0, 255);
		data8[1] = CLAMP(p_color.g * 255.0, 0, 255);
		data8[2] = CLAMP(p_color.b * 255.0, 0, 255);
		data8[3] = CLAMP(p_color.a * 255.0, 0, 255);

	} else if (multimesh->color_format == RS::MULTIMESH_COLOR_FLOAT) {
		dataptr[0] = p_color.r;
		dataptr[1] = p_color.g;
		dataptr[2] = p_color.b;
		dataptr[3] = p_color.a;
	}

	multimesh->dirty_data = true;
	multimesh->dirty_aabb = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

void RasterizerStorageGLES2::_multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color &p_custom_data) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->size);
	ERR_FAIL_COND(multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_NONE);
	ERR_FAIL_INDEX(multimesh->custom_data_format, RS::MULTIMESH_CUSTOM_DATA_MAX);

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index + multimesh->xform_floats + multimesh->color_floats];

	if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_8BIT) {
		uint8_t *data8 = (uint8_t *)dataptr;
		data8[0] = CLAMP(p_custom_data.r * 255.0, 0, 255);
		data8[1] = CLAMP(p_custom_data.g * 255.0, 0, 255);
		data8[2] = CLAMP(p_custom_data.b * 255.0, 0, 255);
		data8[3] = CLAMP(p_custom_data.a * 255.0, 0, 255);

	} else if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_FLOAT) {
		dataptr[0] = p_custom_data.r;
		dataptr[1] = p_custom_data.g;
		dataptr[2] = p_custom_data.b;
		dataptr[3] = p_custom_data.a;
	}

	multimesh->dirty_data = true;
	multimesh->dirty_aabb = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

RID RasterizerStorageGLES2::_multimesh_get_mesh(RID p_multimesh) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, RID());

	return multimesh->mesh;
}

Transform RasterizerStorageGLES2::_multimesh_instance_get_transform(RID p_multimesh, int p_index) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, Transform());
	ERR_FAIL_INDEX_V(p_index, multimesh->size, Transform());
	ERR_FAIL_COND_V(multimesh->transform_format == RS::MULTIMESH_TRANSFORM_2D, Transform());

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index];

	Transform xform;

	xform.basis.rows[0][0] = dataptr[0];
	xform.basis.rows[0][1] = dataptr[1];
	xform.basis.rows[0][2] = dataptr[2];
	xform.origin.x = dataptr[3];
	xform.basis.rows[1][0] = dataptr[4];
	xform.basis.rows[1][1] = dataptr[5];
	xform.basis.rows[1][2] = dataptr[6];
	xform.origin.y = dataptr[7];
	xform.basis.rows[2][0] = dataptr[8];
	xform.basis.rows[2][1] = dataptr[9];
	xform.basis.rows[2][2] = dataptr[10];
	xform.origin.z = dataptr[11];

	return xform;
}

Transform2D RasterizerStorageGLES2::_multimesh_instance_get_transform_2d(RID p_multimesh, int p_index) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, Transform2D());
	ERR_FAIL_INDEX_V(p_index, multimesh->size, Transform2D());
	ERR_FAIL_COND_V(multimesh->transform_format == RS::MULTIMESH_TRANSFORM_3D, Transform2D());

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index];

	Transform2D xform;

	xform.columns[0][0] = dataptr[0];
	xform.columns[1][0] = dataptr[1];
	xform.columns[2][0] = dataptr[3];
	xform.columns[0][1] = dataptr[4];
	xform.columns[1][1] = dataptr[5];
	xform.columns[2][1] = dataptr[7];

	return xform;
}

Color RasterizerStorageGLES2::_multimesh_instance_get_color(RID p_multimesh, int p_index) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, Color());
	ERR_FAIL_INDEX_V(p_index, multimesh->size, Color());
	ERR_FAIL_COND_V(multimesh->color_format == RS::MULTIMESH_COLOR_NONE, Color());
	ERR_FAIL_INDEX_V(multimesh->color_format, RS::MULTIMESH_COLOR_MAX, Color());

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index + multimesh->xform_floats];

	if (multimesh->color_format == RS::MULTIMESH_COLOR_8BIT) {
		union {
			uint32_t colu;
			float colf;
		} cu;

		cu.colf = dataptr[0];

		return Color::hex(BSWAP32(cu.colu));

	} else if (multimesh->color_format == RS::MULTIMESH_COLOR_FLOAT) {
		Color c;
		c.r = dataptr[0];
		c.g = dataptr[1];
		c.b = dataptr[2];
		c.a = dataptr[3];

		return c;
	}

	return Color();
}

Color RasterizerStorageGLES2::_multimesh_instance_get_custom_data(RID p_multimesh, int p_index) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, Color());
	ERR_FAIL_INDEX_V(p_index, multimesh->size, Color());
	ERR_FAIL_COND_V(multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_NONE, Color());
	ERR_FAIL_INDEX_V(multimesh->custom_data_format, RS::MULTIMESH_CUSTOM_DATA_MAX, Color());

	int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
	float *dataptr = &multimesh->data.write[stride * p_index + multimesh->xform_floats + multimesh->color_floats];

	if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_8BIT) {
		union {
			uint32_t colu;
			float colf;
		} cu;

		cu.colf = dataptr[0];

		return Color::hex(BSWAP32(cu.colu));

	} else if (multimesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_FLOAT) {
		Color c;
		c.r = dataptr[0];
		c.g = dataptr[1];
		c.b = dataptr[2];
		c.a = dataptr[3];

		return c;
	}

	return Color();
}

void RasterizerStorageGLES2::_multimesh_set_as_bulk_array(RID p_multimesh, const PoolVector<float> &p_array) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);
	ERR_FAIL_COND(!multimesh->data.ptr());

	int dsize = multimesh->data.size();

	ERR_FAIL_COND(dsize != p_array.size());

	PoolVector<float>::Read r = p_array.read();
	ERR_FAIL_COND(!r.ptr());
	memcpy(multimesh->data.ptrw(), r.ptr(), dsize * sizeof(float));

	multimesh->dirty_data = true;
	multimesh->dirty_aabb = true;

	if (!multimesh->update_list.in_list()) {
		multimesh_update_list.add(&multimesh->update_list);
	}
}

void RasterizerStorageGLES2::_multimesh_set_visible_instances(RID p_multimesh, int p_visible) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND(!multimesh);

	multimesh->visible_instances = p_visible;
}

int RasterizerStorageGLES2::_multimesh_get_visible_instances(RID p_multimesh) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, -1);

	return multimesh->visible_instances;
}

AABB RasterizerStorageGLES2::_multimesh_get_aabb(RID p_multimesh) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, AABB());

	const_cast<RasterizerStorageGLES2 *>(this)->update_dirty_multimeshes();

	return multimesh->aabb;
}

RasterizerStorage::MMInterpolator *RasterizerStorageGLES2::_multimesh_get_interpolator(RID p_multimesh) const {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_COND_V(!multimesh, nullptr);

	return &multimesh->interpolator;
}

void RasterizerStorageGLES2::multimesh_attach_canvas_item(RID p_multimesh, RID p_canvas_item, bool p_attach) {
	MultiMesh *multimesh = multimesh_owner.getornull(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_COND(!p_canvas_item.is_valid());

	if (p_attach) {
		int64_t found = multimesh->linked_canvas_items.find(p_canvas_item);
		if (found == -1) {
			multimesh->linked_canvas_items.push_back(p_canvas_item);
		}
	} else {
		int64_t found = multimesh->linked_canvas_items.find(p_canvas_item);
		if (found != -1) {
			multimesh->linked_canvas_items.remove_unordered(found);
		}
	}
}

void RasterizerStorageGLES2::update_dirty_multimeshes() {
	while (multimesh_update_list.first()) {
		MultiMesh *multimesh = multimesh_update_list.first()->self();

		if (multimesh->size && multimesh->dirty_aabb) {
			AABB mesh_aabb;

			if (multimesh->mesh.is_valid()) {
				mesh_aabb = mesh_get_aabb(multimesh->mesh);
			}

			mesh_aabb.size += Vector3(0.001, 0.001, 0.001); //in case mesh is empty in one of the sides

			int stride = multimesh->color_floats + multimesh->xform_floats + multimesh->custom_data_floats;
			int count = multimesh->data.size();
			float *data = multimesh->data.ptrw();

			AABB aabb;

			if (multimesh->transform_format == RS::MULTIMESH_TRANSFORM_2D) {
				for (int i = 0; i < count; i += stride) {
					float *dataptr = &data[i];

					Transform xform;
					xform.basis[0][0] = dataptr[0];
					xform.basis[0][1] = dataptr[1];
					xform.origin[0] = dataptr[3];
					xform.basis[1][0] = dataptr[4];
					xform.basis[1][1] = dataptr[5];
					xform.origin[1] = dataptr[7];

					AABB laabb = xform.xform(mesh_aabb);

					if (i == 0) {
						aabb = laabb;
					} else {
						aabb.merge_with(laabb);
					}
				}

			} else {
				for (int i = 0; i < count; i += stride) {
					float *dataptr = &data[i];

					Transform xform;
					xform.basis.rows[0][0] = dataptr[0];
					xform.basis.rows[0][1] = dataptr[1];
					xform.basis.rows[0][2] = dataptr[2];
					xform.origin.x = dataptr[3];
					xform.basis.rows[1][0] = dataptr[4];
					xform.basis.rows[1][1] = dataptr[5];
					xform.basis.rows[1][2] = dataptr[6];
					xform.origin.y = dataptr[7];
					xform.basis.rows[2][0] = dataptr[8];
					xform.basis.rows[2][1] = dataptr[9];
					xform.basis.rows[2][2] = dataptr[10];
					xform.origin.z = dataptr[11];

					AABB laabb = xform.xform(mesh_aabb);

					if (i == 0) {
						aabb = laabb;
					} else {
						aabb.merge_with(laabb);
					}
				}
			}

			multimesh->aabb = aabb;

			// Inform any linked canvas items that bounds have changed
			// (for hierarchical culling).
			int num_linked = multimesh->linked_canvas_items.size();
			for (int n = 0; n < num_linked; n++) {
				const RID &rid = multimesh->linked_canvas_items[n];
				RSG::canvas->_canvas_item_invalidate_local_bound(rid);
			}
		}

		multimesh->dirty_aabb = false;
		multimesh->dirty_data = false;

		multimesh->instance_change_notify(true, false);

		multimesh_update_list.remove(multimesh_update_list.first());
	}
}

void RasterizerStorageGLES2::update_dirty_blend_shapes() {
	while (blend_shapes_update_list.first()) {
		Mesh *mesh = blend_shapes_update_list.first()->self();
		for (int is = 0; is < mesh->surfaces.size(); is++) {
			RasterizerStorageGLES2::Surface *s = mesh->surfaces[is];
			if (!s->blend_shape_data.empty()) {
				PoolVector<float> &transform_buffer = resources.blend_shape_transform_cpu_buffer;
				size_t buffer_size = s->array_len * 8 * 4;
				if (resources.blend_shape_transform_cpu_buffer_size < buffer_size) {
					resources.blend_shape_transform_cpu_buffer_size = buffer_size;
					transform_buffer.resize(buffer_size);
				}

				PoolVector<uint8_t>::Read read = s->data.read();
				PoolVector<float>::Write write = transform_buffer.write();
				float base_weight = 1.0;

				if (s->mesh->blend_shape_mode == RS::BLEND_SHAPE_MODE_NORMALIZED) {
					for (int ti = 0; ti < mesh->blend_shape_values.size(); ti++) {
						base_weight -= mesh->blend_shape_values.get(ti);
					}
				}

				for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
					if (s->attribs[i].enabled) {
						// Read all attributes
						for (int j = 0; j < s->array_len; j++) {
							size_t offset = s->attribs[i].offset + (j * s->attribs[i].stride);
							const float *rd = (const float *)(read.ptr() + offset);

							size_t offset_write = i * 4 + (j * 8 * 4);
							float *wr = (float *)(write.ptr() + offset_write);

							// Set the base
							switch (i) {
								case RS::ARRAY_VERTEX: {
									if (s->format & RS::ARRAY_COMPRESS_VERTEX) {
										wr[0] = Math::halfptr_to_float(&((uint16_t *)rd)[0]) * base_weight;
										wr[1] = Math::halfptr_to_float(&((uint16_t *)rd)[1]) * base_weight;
										wr[2] = Math::halfptr_to_float(&((uint16_t *)rd)[2]) * base_weight;
										wr[3] = 1.0f;
									} else {
										float a[3] = { 0 };
										a[0] = wr[0] = rd[0] * base_weight;
										a[1] = wr[1] = rd[1] * base_weight;
										a[2] = wr[2] = rd[2] * base_weight;
										memcpy(&write[offset_write], a, sizeof(float) * s->attribs[i].size);
									}
								} break;
								case RS::ARRAY_NORMAL: {
									if (s->format & RS::ARRAY_COMPRESS_NORMAL) {
										wr[0] = (((int8_t *)rd)[0] / 127.0) * base_weight;
										wr[1] = (((int8_t *)rd)[1] / 127.0) * base_weight;
										wr[2] = (((int8_t *)rd)[2] / 127.0) * base_weight;
									} else {
										wr[0] = rd[0] * base_weight;
										wr[1] = rd[1] * base_weight;
										wr[2] = rd[2] * base_weight;
									}

								} break;
								case RS::ARRAY_TANGENT: {
									if (s->format & RS::ARRAY_COMPRESS_TANGENT) {
										wr[0] = (((int8_t *)rd)[0] / 127.0) * base_weight;
										wr[1] = (((int8_t *)rd)[1] / 127.0) * base_weight;
										wr[2] = (((int8_t *)rd)[2] / 127.0) * base_weight;
										wr[3] = (((int8_t *)rd)[3] / 127.0) * base_weight;
									} else {
										wr[0] = rd[0] * base_weight;
										wr[1] = rd[1] * base_weight;
										wr[2] = rd[2] * base_weight;
										wr[3] = rd[3] * base_weight;
									}

								} break;
								case RS::ARRAY_COLOR: {
									if (s->format & RS::ARRAY_COMPRESS_COLOR) {
										wr[0] = (((uint8_t *)rd)[0] / 255.0) * base_weight;
										wr[1] = (((uint8_t *)rd)[1] / 255.0) * base_weight;
										wr[2] = (((uint8_t *)rd)[2] / 255.0) * base_weight;
										wr[3] = (((uint8_t *)rd)[3] / 255.0) * base_weight;
									} else {
										wr[0] = rd[0] * base_weight;
										wr[1] = rd[1] * base_weight;
										wr[2] = rd[2] * base_weight;
										wr[3] = rd[3] * base_weight;
									}
								} break;
								case RS::ARRAY_TEX_UV: {
									if (s->format & RS::ARRAY_COMPRESS_TEX_UV) {
										wr[0] = Math::halfptr_to_float(&((uint16_t *)rd)[0]) * base_weight;
										wr[1] = Math::halfptr_to_float(&((uint16_t *)rd)[1]) * base_weight;
									} else {
										wr[0] = rd[0] * base_weight;
										wr[1] = rd[1] * base_weight;
									}
								} break;
								case RS::ARRAY_TEX_UV2: {
									if (s->format & RS::ARRAY_COMPRESS_TEX_UV2) {
										wr[0] = Math::halfptr_to_float(&((uint16_t *)rd)[0]) * base_weight;
										wr[1] = Math::halfptr_to_float(&((uint16_t *)rd)[1]) * base_weight;
									} else {
										wr[0] = rd[0] * base_weight;
										wr[1] = rd[1] * base_weight;
									}
								} break;
							}

							// Add all blend shapes
							for (int ti = 0; ti < mesh->blend_shape_values.size(); ti++) {
								PoolVector<uint8_t>::Read blend = s->blend_shape_data[ti].read();
								const float *br = (const float *)(blend.ptr() + offset);

								float weight = mesh->blend_shape_values.get(ti);
								if (Math::is_zero_approx(weight)) {
									continue;
								}

								switch (i) {
									case RS::ARRAY_VERTEX: {
										if (s->format & RS::ARRAY_COMPRESS_VERTEX) {
											wr[0] += Math::halfptr_to_float(&((uint16_t *)br)[0]) * weight;
											wr[1] += Math::halfptr_to_float(&((uint16_t *)br)[1]) * weight;
											wr[2] += Math::halfptr_to_float(&((uint16_t *)br)[2]) * weight;
											wr[3] = 1.0f;
										} else {
											wr[0] += br[0] * weight;
											wr[1] += br[1] * weight;
											wr[2] += br[2] * weight;
										}
									} break;
									case RS::ARRAY_NORMAL: {
										if (s->format & RS::ARRAY_COMPRESS_NORMAL) {
											wr[0] += (float(((int8_t *)br)[0]) / 127.0) * weight;
											wr[1] += (float(((int8_t *)br)[1]) / 127.0) * weight;
											wr[2] += (float(((int8_t *)br)[2]) / 127.0) * weight;
										} else {
											wr[0] += br[0] * weight;
											wr[1] += br[1] * weight;
											wr[2] += br[2] * weight;
										}

									} break;
									case RS::ARRAY_TANGENT: {
										if (s->format & RS::ARRAY_COMPRESS_TANGENT) {
											wr[0] += (float(((int8_t *)br)[0]) / 127.0) * weight;
											wr[1] += (float(((int8_t *)br)[1]) / 127.0) * weight;
											wr[2] += (float(((int8_t *)br)[2]) / 127.0) * weight;
											wr[3] = (float(((int8_t *)br)[3]) / 127.0);
										} else {
											wr[0] += br[0] * weight;
											wr[1] += br[1] * weight;
											wr[2] += br[2] * weight;
											wr[3] = br[3];
										}

									} break;
									case RS::ARRAY_COLOR: {
										if (s->format & RS::ARRAY_COMPRESS_COLOR) {
											wr[0] += (((uint8_t *)br)[0] / 255.0) * weight;
											wr[1] += (((uint8_t *)br)[1] / 255.0) * weight;
											wr[2] += (((uint8_t *)br)[2] / 255.0) * weight;
											wr[3] += (((uint8_t *)br)[3] / 255.0) * weight;
										} else {
											wr[0] += br[0] * weight;
											wr[1] += br[1] * weight;
											wr[2] += br[2] * weight;
											wr[3] += br[3] * weight;
										}
									} break;
									case RS::ARRAY_TEX_UV: {
										if (s->format & RS::ARRAY_COMPRESS_TEX_UV) {
											wr[0] += Math::halfptr_to_float(&((uint16_t *)br)[0]) * weight;
											wr[1] += Math::halfptr_to_float(&((uint16_t *)br)[1]) * weight;
										} else {
											wr[0] += br[0] * weight;
											wr[1] += br[1] * weight;
										}
									} break;
									case RS::ARRAY_TEX_UV2: {
										if (s->format & RS::ARRAY_COMPRESS_TEX_UV2) {
											wr[0] += Math::halfptr_to_float(&((uint16_t *)br)[0]) * weight;
											wr[1] += Math::halfptr_to_float(&((uint16_t *)br)[1]) * weight;
										} else {
											wr[0] += br[0] * weight;
											wr[1] += br[1] * weight;
										}
									} break;
								}
							}
						}
					}
				}

				// Store size and send changed blend shape render to GL
				glBindBuffer(GL_ARRAY_BUFFER, s->blend_shape_buffer_id);
				if (buffer_size > s->blend_shape_buffer_size) {
					s->blend_shape_buffer_size = buffer_size;
					glBufferData(GL_ARRAY_BUFFER, buffer_size * sizeof(float), transform_buffer.read().ptr(), GL_DYNAMIC_DRAW);
				} else {
					buffer_orphan_and_upload(s->blend_shape_buffer_size * sizeof(float), 0, buffer_size * sizeof(float), transform_buffer.read().ptr(), GL_ARRAY_BUFFER, true);
				}
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}
		blend_shapes_update_list.remove(blend_shapes_update_list.first());
	}
}

////////

void RasterizerStorageGLES2::instance_add_skeleton(RID p_skeleton, RasterizerScene::InstanceBase *p_instance) {
}

void RasterizerStorageGLES2::instance_remove_skeleton(RID p_skeleton, RasterizerScene::InstanceBase *p_instance) {
}

void RasterizerStorageGLES2::instance_add_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance) {
	Instantiable *inst = nullptr;
	switch (p_instance->base_type) {
		case RS::INSTANCE_MESH: {
			inst = mesh_owner.getornull(p_base);
			ERR_FAIL_COND(!inst);
		} break;
		case RS::INSTANCE_MULTIMESH: {
			inst = multimesh_owner.getornull(p_base);
			ERR_FAIL_COND(!inst);
		} break;
		default: {
			ERR_FAIL();
		}
	}

	inst->instance_list.add(&p_instance->dependency_item);
}

void RasterizerStorageGLES2::instance_remove_dependency(RID p_base, RasterizerScene::InstanceBase *p_instance) {
	Instantiable *inst = nullptr;

	switch (p_instance->base_type) {
		case RS::INSTANCE_MESH: {
			inst = mesh_owner.getornull(p_base);
			ERR_FAIL_COND(!inst);
		} break;
		case RS::INSTANCE_MULTIMESH: {
			inst = multimesh_owner.getornull(p_base);
			ERR_FAIL_COND(!inst);
		} break;
		default: {
			ERR_FAIL();
		}
	}

	inst->instance_list.remove(&p_instance->dependency_item);
}

/* RENDER TARGET */

void RasterizerStorageGLES2::_render_target_allocate(RenderTarget *rt) {
	// do not allocate a render target with no size
	if (rt->width <= 0 || rt->height <= 0) {
		return;
	}

	// do not allocate a render target that is attached to the screen
	if (rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN]) {
		rt->fbo = RasterizerStorageGLES2::system_fbo;
		return;
	}

	if (rt->width > config.max_viewport_dimensions[0] || rt->height > config.max_viewport_dimensions[1]) {
		WARN_PRINT("Cannot create render target larger than maximum hardware supported size of (" + itos(config.max_viewport_dimensions[0]) + ", " + itos(config.max_viewport_dimensions[1]) + "). Setting size to maximum.");
		rt->width = MIN(rt->width, config.max_viewport_dimensions[0]);
		rt->height = MIN(rt->height, config.max_viewport_dimensions[1]);
	}

	GLuint color_internal_format;
	GLuint color_format;
	GLuint color_type = GL_UNSIGNED_BYTE;
	Image::Format image_format;

	if (rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
#ifdef GLES_OVER_GL
		color_internal_format = GL_RGBA8;
#else
		color_internal_format = GL_RGBA;
#endif
		color_format = GL_RGBA;
		image_format = Image::FORMAT_RGBA8;
	} else {
#ifdef GLES_OVER_GL
		color_internal_format = GL_RGB8;
#else
		color_internal_format = GL_RGB;
#endif
		color_format = GL_RGB;
		image_format = Image::FORMAT_RGB8;
	}

	rt->used_dof_blur_near = false;
	rt->mip_maps_allocated = false;

	{
		/* Front FBO */

		Texture *texture = texture_owner.getornull(rt->texture);
		ERR_FAIL_COND(!texture);

		// framebuffer
		glGenFramebuffers(1, &rt->fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);

		// color
		glGenTextures(1, &rt->color);
		glBindTexture(GL_TEXTURE_2D, rt->color);

		glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, rt->width, rt->height, 0, color_format, color_type, nullptr);

		if (texture->flags & RS::TEXTURE_FLAG_FILTER) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->color, 0);

		// depth

		if (config.support_depth_texture) {
			glGenTextures(1, &rt->depth);
			glBindTexture(GL_TEXTURE_2D, rt->depth);
			glTexImage2D(GL_TEXTURE_2D, 0, config.depth_internalformat, rt->width, rt->height, 0, GL_DEPTH_COMPONENT, config.depth_type, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
		} else {
			glGenRenderbuffers(1, &rt->depth);
			glBindRenderbuffer(GL_RENDERBUFFER, rt->depth);

			glRenderbufferStorage(GL_RENDERBUFFER, config.depth_buffer_internalformat, rt->width, rt->height);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			glDeleteFramebuffers(1, &rt->fbo);
			if (config.support_depth_texture) {
				glDeleteTextures(1, &rt->depth);
			} else {
				glDeleteRenderbuffers(1, &rt->depth);
			}

			glDeleteTextures(1, &rt->color);
			rt->fbo = 0;
			rt->width = 0;
			rt->height = 0;
			rt->color = 0;
			rt->depth = 0;
			texture->tex_id = 0;
			texture->active = false;
			WARN_PRINT("Could not create framebuffer!!");
			return;
		}

		texture->format = image_format;
		texture->gl_format_cache = color_format;
		texture->gl_type_cache = GL_UNSIGNED_BYTE;
		texture->gl_internal_format_cache = color_internal_format;
		texture->tex_id = rt->color;
		texture->width = rt->width;
		texture->alloc_width = rt->width;
		texture->height = rt->height;
		texture->alloc_height = rt->height;
		texture->active = true;

		texture_set_flags(rt->texture, texture->flags);
	}

	/* BACK FBO */
	/* For MSAA */

#ifndef JAVASCRIPT_ENABLED
	if (rt->msaa >= RS::VIEWPORT_MSAA_2X && rt->msaa <= RS::VIEWPORT_MSAA_16X && config.multisample_supported) {
		rt->multisample_active = true;

		static const int msaa_value[] = { 0, 2, 4, 8, 16 };
		int msaa = msaa_value[rt->msaa];

		int max_samples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
		if (msaa > max_samples) {
			WARN_PRINT("MSAA must be <= GL_MAX_SAMPLES, falling-back to GL_MAX_SAMPLES = " + itos(max_samples));
			msaa = max_samples;
		}

		//regular fbo
		glGenFramebuffers(1, &rt->multisample_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, rt->multisample_fbo);

		glGenRenderbuffers(1, &rt->multisample_depth);
		glBindRenderbuffer(GL_RENDERBUFFER, rt->multisample_depth);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, config.depth_buffer_internalformat, rt->width, rt->height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->multisample_depth);

#if defined(GLES_OVER_GL) || defined(IPHONE_ENABLED)

		glGenRenderbuffers(1, &rt->multisample_color);
		glBindRenderbuffer(GL_RENDERBUFFER, rt->multisample_color);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, color_internal_format, rt->width, rt->height);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rt->multisample_color);
#elif ANDROID_ENABLED
		// Render to a texture in android
		glGenTextures(1, &rt->multisample_color);
		glBindTexture(GL_TEXTURE_2D, rt->multisample_color);

		glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, rt->width, rt->height, 0, color_format, color_type, NULL);

		// multisample buffer is same size as front buffer, so just use nearest
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glFramebufferTexture2DMultisample(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->multisample_color, 0, msaa);
#endif

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			// Delete allocated resources and default to no MSAA
			WARN_PRINT_ONCE("Cannot allocate back framebuffer for MSAA");
			printf("err status: %x\n", status);
			config.multisample_supported = false;
			rt->multisample_active = false;

			glDeleteFramebuffers(1, &rt->multisample_fbo);
			rt->multisample_fbo = 0;

			glDeleteRenderbuffers(1, &rt->multisample_depth);
			rt->multisample_depth = 0;
#ifdef ANDROID_ENABLED
			glDeleteTextures(1, &rt->multisample_color);
#else
			glDeleteRenderbuffers(1, &rt->multisample_color);
#endif
			rt->multisample_color = 0;
		}

		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
#ifdef ANDROID_ENABLED
		glBindTexture(GL_TEXTURE_2D, 0);
#endif

	} else
#endif // JAVASCRIPT_ENABLED
	{
		rt->multisample_active = false;
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// copy texscreen buffers
	if (!(rt->flags[RasterizerStorage::RENDER_TARGET_NO_SAMPLING])) {
		glGenTextures(1, &rt->copy_screen_effect.color);
		glBindTexture(GL_TEXTURE_2D, rt->copy_screen_effect.color);

		if (rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rt->width, rt->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rt->width, rt->height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glGenFramebuffers(1, &rt->copy_screen_effect.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, rt->copy_screen_effect.fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->copy_screen_effect.color, 0);

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			_render_target_clear(rt);
			ERR_FAIL_COND(status != GL_FRAMEBUFFER_COMPLETE);
		}
	}

	// Allocate mipmap chains for post_process effects
	if (!rt->flags[RasterizerStorage::RENDER_TARGET_NO_3D] && rt->width >= 2 && rt->height >= 2) {
		for (int i = 0; i < 2; i++) {
			ERR_FAIL_COND(rt->mip_maps[i].sizes.size());
			int w = rt->width;
			int h = rt->height;

			if (i > 0) {
				w >>= 1;
				h >>= 1;
			}

			int level = 0;
			int fb_w = w;
			int fb_h = h;

			while (true) {
				RenderTarget::MipMaps::Size mm;
				mm.fbo = 0;
				mm.color = 0;
				mm.width = w;
				mm.height = h;
				rt->mip_maps[i].sizes.push_back(mm);

				w >>= 1;
				h >>= 1;

				if (w < 2 || h < 2) {
					break;
				}

				level++;
			}

			GLsizei width = fb_w;
			GLsizei height = fb_h;

			if (config.render_to_mipmap_supported) {
				glGenTextures(1, &rt->mip_maps[i].color);
				glBindTexture(GL_TEXTURE_2D, rt->mip_maps[i].color);

				for (int l = 0; l < level + 1; l++) {
					glTexImage2D(GL_TEXTURE_2D, l, color_internal_format, width, height, 0, color_format, color_type, nullptr);
					width = MAX(1, (width / 2));
					height = MAX(1, (height / 2));
				}
#ifdef GLES_OVER_GL
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, level);
#endif
			} else {
				// Can't render to specific levels of a mipmap in ES 2.0 or Webgl so create a texture for each level
				for (int l = 0; l < level + 1; l++) {
					glGenTextures(1, &rt->mip_maps[i].sizes.write[l].color);
					glBindTexture(GL_TEXTURE_2D, rt->mip_maps[i].sizes[l].color);
					glTexImage2D(GL_TEXTURE_2D, 0, color_internal_format, width, height, 0, color_format, color_type, nullptr);
					width = MAX(1, (width / 2));
					height = MAX(1, (height / 2));
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				}
			}

			glDisable(GL_SCISSOR_TEST);
			glColorMask(1, 1, 1, 1);
			glDepthMask(GL_TRUE);

			for (int j = 0; j < rt->mip_maps[i].sizes.size(); j++) {
				RenderTarget::MipMaps::Size &mm = rt->mip_maps[i].sizes.write[j];

				glGenFramebuffers(1, &mm.fbo);
				glBindFramebuffer(GL_FRAMEBUFFER, mm.fbo);

				if (config.render_to_mipmap_supported) {
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->mip_maps[i].color, j);
				} else {
					glBindTexture(GL_TEXTURE_2D, rt->mip_maps[i].sizes[j].color);
					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->mip_maps[i].sizes[j].color, 0);
				}

				bool used_depth = false;
				if (j == 0 && i == 0) { //use always
					if (config.support_depth_texture) {
						glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
					} else {
						glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
					}
					used_depth = true;
				}

				GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				if (status != GL_FRAMEBUFFER_COMPLETE) {
					WARN_PRINT_ONCE("Cannot allocate mipmaps for 3D post processing effects");
					glBindFramebuffer(GL_FRAMEBUFFER, RasterizerStorageGLES2::system_fbo);
					return;
				}

				glClearColor(1.0, 0.0, 1.0, 0.0);
				glClear(GL_COLOR_BUFFER_BIT);
				if (used_depth) {
					glClearDepth(1.0);
					glClear(GL_DEPTH_BUFFER_BIT);
				}
			}

			rt->mip_maps[i].levels = level;

			if (config.render_to_mipmap_supported) {
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
		}
		rt->mip_maps_allocated = true;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, RasterizerStorageGLES2::system_fbo);
}

void RasterizerStorageGLES2::_render_target_clear(RenderTarget *rt) {
	// there is nothing to clear when DIRECT_TO_SCREEN is used
	if (rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN]) {
		return;
	}

	if (rt->fbo) {
		glDeleteFramebuffers(1, &rt->fbo);
		glDeleteTextures(1, &rt->color);
		rt->fbo = 0;
	}

	Texture *tex = texture_owner.get(rt->texture);
	tex->alloc_height = 0;
	tex->alloc_width = 0;
	tex->width = 0;
	tex->height = 0;
	tex->active = false;

	if (rt->external.fbo != 0) {
		// free this
		glDeleteFramebuffers(1, &rt->external.fbo);

		// reset our texture back to the original
		tex->tex_id = rt->color;

		if (rt->external.depth != 0 && rt->external.depth_owned) {
			glDeleteRenderbuffers(1, &rt->external.depth);
		}

		rt->external.fbo = 0;
		rt->external.color = 0;
		rt->external.depth = 0;
		rt->external.depth_owned = false;
	}

	if (rt->depth) {
		if (config.support_depth_texture) {
			glDeleteTextures(1, &rt->depth);
		} else {
			glDeleteRenderbuffers(1, &rt->depth);
		}

		rt->depth = 0;
	}

	if (rt->copy_screen_effect.color) {
		glDeleteFramebuffers(1, &rt->copy_screen_effect.fbo);
		rt->copy_screen_effect.fbo = 0;

		glDeleteTextures(1, &rt->copy_screen_effect.color);
		rt->copy_screen_effect.color = 0;
	}

	for (int i = 0; i < 2; i++) {
		if (rt->mip_maps[i].sizes.size()) {
			for (int j = 0; j < rt->mip_maps[i].sizes.size(); j++) {
				glDeleteFramebuffers(1, &rt->mip_maps[i].sizes[j].fbo);
				glDeleteTextures(1, &rt->mip_maps[i].sizes[j].color);
			}

			glDeleteTextures(1, &rt->mip_maps[i].color);
			rt->mip_maps[i].sizes.clear();
			rt->mip_maps[i].levels = 0;
			rt->mip_maps[i].color = 0;
		}
	}

	if (rt->multisample_active) {
		glDeleteFramebuffers(1, &rt->multisample_fbo);
		rt->multisample_fbo = 0;

		glDeleteRenderbuffers(1, &rt->multisample_depth);
		rt->multisample_depth = 0;
#ifdef ANDROID_ENABLED
		glDeleteTextures(1, &rt->multisample_color);
#else
		glDeleteRenderbuffers(1, &rt->multisample_color);
#endif
		rt->multisample_color = 0;
	}
}

RID RasterizerStorageGLES2::render_target_create() {
	RenderTarget *rt = memnew(RenderTarget);

	Texture *t = memnew(Texture);

	t->type = RS::TEXTURE_TYPE_2D;
	t->flags = 0;
	t->width = 0;
	t->height = 0;
	t->alloc_height = 0;
	t->alloc_width = 0;
	t->format = Image::FORMAT_R8;
	t->target = GL_TEXTURE_2D;
	t->gl_format_cache = 0;
	t->gl_internal_format_cache = 0;
	t->gl_type_cache = 0;
	t->data_size = 0;
	t->total_data_size = 0;
	t->ignore_mipmaps = false;
	t->compressed = false;
	t->mipmaps = 1;
	t->active = true;
	t->tex_id = 0;
	t->render_target = rt;

	rt->texture = texture_owner.make_rid(t);

	return render_target_owner.make_rid(rt);
}

void RasterizerStorageGLES2::render_target_set_position(RID p_render_target, int p_x, int p_y) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->x = p_x;
	rt->y = p_y;
}

void RasterizerStorageGLES2::render_target_set_size(RID p_render_target, int p_width, int p_height) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_width == rt->width && p_height == rt->height) {
		return;
	}

	_render_target_clear(rt);

	rt->width = p_width;
	rt->height = p_height;

	_render_target_allocate(rt);
}

RID RasterizerStorageGLES2::render_target_get_texture(RID p_render_target) const {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, RID());

	return rt->texture;
}

uint32_t RasterizerStorageGLES2::render_target_get_depth_texture_id(RID p_render_target) const {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, 0);

	if (rt->external.depth == 0) {
		return rt->depth;
	} else {
		return rt->external.depth;
	}
}

void RasterizerStorageGLES2::render_target_set_external_texture(RID p_render_target, unsigned int p_texture_id, unsigned int p_depth_id) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_texture_id == 0) {
		if (rt->external.fbo != 0) {
			// free this
			glDeleteFramebuffers(1, &rt->external.fbo);

			// and this
			if (rt->external.depth != 0 && rt->external.depth_owned) {
				glDeleteRenderbuffers(1, &rt->external.depth);
			}

			// reset our texture back to the original
			Texture *t = texture_owner.get(rt->texture);
			t->tex_id = rt->color;
			t->width = rt->width;
			t->alloc_width = rt->width;
			t->height = rt->height;
			t->alloc_height = rt->height;

			rt->external.fbo = 0;
			rt->external.color = 0;
			rt->external.depth = 0;
		}
	} else {
		if (rt->external.fbo == 0) {
			// create our fbo
			glGenFramebuffers(1, &rt->external.fbo);
		}

		// bind our frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, rt->external.fbo);

		rt->external.color = p_texture_id;

		// Set our texture to the new image, note that we expect formats to be the same (or compatible) so we don't change those
		Texture *t = texture_owner.get(rt->texture);
		t->tex_id = p_texture_id;
		t->width = rt->width;
		t->height = rt->height;
		t->alloc_height = rt->width;
		t->alloc_width = rt->height;

		// Switch our texture on our frame buffer
#if ANDROID_ENABLED
		if (rt->msaa >= RS::VIEWPORT_MSAA_EXT_2X && rt->msaa <= RS::VIEWPORT_MSAA_EXT_4X) {
			// This code only applies to the Oculus Go and Oculus Quest. Due to the the tiled nature
			// of the GPU we can do a single render pass by rendering directly into our texture chains
			// texture and apply MSAA as we render.

			// On any other hardware these two modes are ignored and we do not have any MSAA,
			// the normal MSAA modes need to be used to enable our two pass approach

			// If we created a depth buffer before and we're now passed one, we need to clear it out
			if (rt->external.depth != 0 && rt->external.depth_owned && p_depth_id != 0) {
				glDeleteRenderbuffers(1, &rt->external.depth);
				rt->external.depth_owned = false;
				rt->external.depth = 0;
			}

			if (!rt->external.depth_owned) {
				rt->external.depth = p_depth_id;
			}

			static const int msaa_value[] = { 2, 4 };
			int msaa = msaa_value[rt->msaa - RS::VIEWPORT_MSAA_EXT_2X];

			if (rt->external.depth == 0) {
				rt->external.depth_owned = true;

				// create a multisample depth buffer, we're not reusing Pandemoniums because Pandemonium's didn't get created..
				glGenRenderbuffers(1, &rt->external.depth);
				glBindRenderbuffer(GL_RENDERBUFFER, rt->external.depth);
				glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, config.depth_buffer_internalformat, rt->width, rt->height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->external.depth);
			} else if (!rt->external.depth_owned) {
				// we make an exception here, external plugin MUST make sure this is a proper multisample render buffer!
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->external.depth);
			}

			// and set our external texture as the texture...
			glFramebufferTexture2DMultisample(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_texture_id, 0, msaa);

		} else
#endif
		{
			// if MSAA as on before, clear our render buffer
			if (rt->external.depth != 0 && rt->external.depth_owned) {
				glDeleteRenderbuffers(1, &rt->external.depth);
			}
			rt->external.depth_owned = false;
			rt->external.depth = p_depth_id;

			// set our texture as the destination for our framebuffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p_texture_id, 0);

			// seeing we're rendering into this directly, better also use our depth buffer, just use our existing one :)
			if (rt->external.depth != 0) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->external.depth, 0);
			} else if (config.support_depth_texture) {
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, rt->depth, 0);
			} else {
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
			}
		}

		// check status and unbind
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, RasterizerStorageGLES2::system_fbo);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			printf("framebuffer fail, status: %x\n", status);
		}

		ERR_FAIL_COND(status != GL_FRAMEBUFFER_COMPLETE);
	}
}

void RasterizerStorageGLES2::render_target_set_flag(RID p_render_target, RenderTargetFlags p_flag, bool p_value) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	// When setting DIRECT_TO_SCREEN, you need to clear before the value is set, but allocate after as
	// those functions change how they operate depending on the value of DIRECT_TO_SCREEN
	if (p_flag == RENDER_TARGET_DIRECT_TO_SCREEN && p_value != rt->flags[RENDER_TARGET_DIRECT_TO_SCREEN]) {
		_render_target_clear(rt);
		rt->flags[p_flag] = p_value;
		_render_target_allocate(rt);
	}

	rt->flags[p_flag] = p_value;

	switch (p_flag) {
		case RENDER_TARGET_TRANSPARENT:
		case RENDER_TARGET_HDR:
		case RENDER_TARGET_NO_3D:
		case RENDER_TARGET_NO_SAMPLING:
		case RENDER_TARGET_NO_3D_EFFECTS: {
			//must reset for these formats
			_render_target_clear(rt);
			_render_target_allocate(rt);

		} break;
		default: {
		}
	}
}

bool RasterizerStorageGLES2::render_target_was_used(RID p_render_target) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND_V(!rt, false);

	return rt->used_in_frame;
}

void RasterizerStorageGLES2::render_target_clear_used(RID p_render_target) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->used_in_frame = false;
}

void RasterizerStorageGLES2::render_target_set_msaa(RID p_render_target, RS::ViewportMSAA p_msaa) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (rt->msaa == p_msaa) {
		return;
	}

	if (!config.multisample_supported) {
		ERR_PRINT("MSAA not supported on this hardware.");
		return;
	}

	_render_target_clear(rt);
	rt->msaa = p_msaa;
	_render_target_allocate(rt);
}

void RasterizerStorageGLES2::render_target_set_use_fxaa(RID p_render_target, bool p_fxaa) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	rt->use_fxaa = p_fxaa;
}

void RasterizerStorageGLES2::render_target_set_use_debanding(RID p_render_target, bool p_debanding) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_debanding) {
		WARN_PRINT_ONCE("Debanding is not supported in the GLES2 backend. To use debanding, switch to the GLES3 backend and make sure HDR is enabled.");
	}

	rt->use_debanding = p_debanding;
}

void RasterizerStorageGLES2::render_target_set_sharpen_intensity(RID p_render_target, float p_intensity) {
	RenderTarget *rt = render_target_owner.getornull(p_render_target);
	ERR_FAIL_COND(!rt);

	if (p_intensity >= 0.001) {
		WARN_PRINT_ONCE("Sharpening is not supported in the GLES2 backend. To use sharpening, switch to the GLES3 backend.");
	}

	rt->sharpen_intensity = p_intensity;
}

RS::InstanceType RasterizerStorageGLES2::get_base_type(RID p_rid) const {
	if (mesh_owner.owns(p_rid)) {
		return RS::INSTANCE_MESH;
	} else if (multimesh_owner.owns(p_rid)) {
		return RS::INSTANCE_MULTIMESH;
	} else {
		return RS::INSTANCE_NONE;
	}
}

bool RasterizerStorageGLES2::free(RID p_rid) {
	if (render_target_owner.owns(p_rid)) {
		RenderTarget *rt = render_target_owner.getornull(p_rid);
		_render_target_clear(rt);

		Texture *t = texture_owner.get(rt->texture);
		texture_owner.free(rt->texture);
		memdelete(t);
		render_target_owner.free(p_rid);
		memdelete(rt);

		return true;
	} else if (texture_owner.owns(p_rid)) {
		Texture *t = texture_owner.get(p_rid);
		// can't free a render target texture
		ERR_FAIL_COND_V(t->render_target, true);

		info.texture_mem -= t->total_data_size;
		texture_owner.free(p_rid);
		memdelete(t);

		return true;
	} else if (shader_owner.owns(p_rid)) {
		Shader *shader = shader_owner.get(p_rid);

		if (shader->shader && shader->custom_code_id) {
			shader->shader->free_custom_shader(shader->custom_code_id);
		}

		if (shader->dirty_list.in_list()) {
			_shader_dirty_list.remove(&shader->dirty_list);
		}

		while (shader->materials.first()) {
			Material *m = shader->materials.first()->self();

			m->shader = nullptr;
			_material_make_dirty(m);

			shader->materials.remove(shader->materials.first());
		}

		shader_owner.free(p_rid);
		memdelete(shader);

		return true;
	} else if (material_owner.owns(p_rid)) {
		Material *m = material_owner.get(p_rid);

		if (m->shader) {
			m->shader->materials.remove(&m->list);
		}

		for (RBMap<Geometry *, int>::Element *E = m->geometry_owners.front(); E; E = E->next()) {
			Geometry *g = E->key();
			g->material = RID();
		}

		for (RBMap<RasterizerScene::InstanceBase *, int>::Element *E = m->instance_owners.front(); E; E = E->next()) {
			RasterizerScene::InstanceBase *ins = E->key();

			if (ins->material_override == p_rid) {
				ins->material_override = RID();
			}

			if (ins->material_overlay == p_rid) {
				ins->material_overlay = RID();
			}

			for (int i = 0; i < ins->materials.size(); i++) {
				if (ins->materials[i] == p_rid) {
					ins->materials.write[i] = RID();
				}
			}
		}

		material_owner.free(p_rid);
		memdelete(m);

		return true;
	} else if (mesh_owner.owns(p_rid)) {
		Mesh *mesh = mesh_owner.get(p_rid);

		mesh->instance_remove_deps();
		mesh_clear(p_rid);

		while (mesh->multimeshes.first()) {
			MultiMesh *multimesh = mesh->multimeshes.first()->self();
			multimesh->mesh = RID();
			multimesh->dirty_aabb = true;

			mesh->multimeshes.remove(mesh->multimeshes.first());

			if (!multimesh->update_list.in_list()) {
				multimesh_update_list.add(&multimesh->update_list);
			}
		}

		mesh_owner.free(p_rid);
		memdelete(mesh);

		return true;
	} else if (multimesh_owner.owns(p_rid)) {
		// remove from interpolator
		_interpolation_data.notify_free_multimesh(p_rid);

		MultiMesh *multimesh = multimesh_owner.get(p_rid);

		// remove any references in linked canvas items
		int num_linked = multimesh->linked_canvas_items.size();
		for (int n = 0; n < num_linked; n++) {
			const RID &rid = multimesh->linked_canvas_items[n];
			RSG::canvas->_canvas_item_remove_references(rid, p_rid);
		}

		multimesh->instance_remove_deps();

		if (multimesh->mesh.is_valid()) {
			Mesh *mesh = mesh_owner.getornull(multimesh->mesh);
			if (mesh) {
				mesh->multimeshes.remove(&multimesh->mesh_list);
			}
		}

		multimesh_allocate(p_rid, 0, RS::MULTIMESH_TRANSFORM_3D, RS::MULTIMESH_COLOR_NONE);

		update_dirty_multimeshes();

		multimesh_owner.free(p_rid);
		memdelete(multimesh);

		return true;
	} else {
		return false;
	}
}

bool RasterizerStorageGLES2::has_os_feature(const String &p_feature) const {
	if (p_feature == "pvrtc") {
		return config.pvrtc_supported;
	}

	if (p_feature == "s3tc") {
		return config.s3tc_supported;
	}

	if (p_feature == "etc") {
		return config.etc1_supported;
	}

	return false;
}

////////////////////////////////////////////

void RasterizerStorageGLES2::set_debug_generate_wireframes(bool p_generate) {
}

void RasterizerStorageGLES2::render_info_begin_capture() {
	info.snap = info.render;
}

void RasterizerStorageGLES2::render_info_end_capture() {
	info.snap.object_count = info.render.object_count - info.snap.object_count;
	info.snap.draw_call_count = info.render.draw_call_count - info.snap.draw_call_count;
	info.snap.material_switch_count = info.render.material_switch_count - info.snap.material_switch_count;
	info.snap.surface_switch_count = info.render.surface_switch_count - info.snap.surface_switch_count;
	info.snap.shader_rebind_count = info.render.shader_rebind_count - info.snap.shader_rebind_count;
	info.snap.vertices_count = info.render.vertices_count - info.snap.vertices_count;
	info.snap._2d_item_count = info.render._2d_item_count - info.snap._2d_item_count;
	info.snap._2d_draw_call_count = info.render._2d_draw_call_count - info.snap._2d_draw_call_count;
}

int RasterizerStorageGLES2::get_captured_render_info(RS::RenderInfo p_info) {
	switch (p_info) {
		case RS::INFO_OBJECTS_IN_FRAME: {
			return info.snap.object_count;
		} break;
		case RS::INFO_VERTICES_IN_FRAME: {
			return info.snap.vertices_count;
		} break;
		case RS::INFO_MATERIAL_CHANGES_IN_FRAME: {
			return info.snap.material_switch_count;
		} break;
		case RS::INFO_SHADER_CHANGES_IN_FRAME: {
			return info.snap.shader_rebind_count;
		} break;
		case RS::INFO_SURFACE_CHANGES_IN_FRAME: {
			return info.snap.surface_switch_count;
		} break;
		case RS::INFO_DRAW_CALLS_IN_FRAME: {
			return info.snap.draw_call_count;
		} break;
		case RS::INFO_2D_ITEMS_IN_FRAME: {
			return info.snap._2d_item_count;
		} break;
		case RS::INFO_2D_DRAW_CALLS_IN_FRAME: {
			return info.snap._2d_draw_call_count;
		} break;
		default: {
			return get_render_info(p_info);
		}
	}
}

uint64_t RasterizerStorageGLES2::get_render_info(RS::RenderInfo p_info) {
	switch (p_info) {
		case RS::INFO_OBJECTS_IN_FRAME:
			return info.render_final.object_count;
		case RS::INFO_VERTICES_IN_FRAME:
			return info.render_final.vertices_count;
		case RS::INFO_MATERIAL_CHANGES_IN_FRAME:
			return info.render_final.material_switch_count;
		case RS::INFO_SHADER_CHANGES_IN_FRAME:
			return info.render_final.shader_rebind_count;
		case RS::INFO_SURFACE_CHANGES_IN_FRAME:
			return info.render_final.surface_switch_count;
		case RS::INFO_DRAW_CALLS_IN_FRAME:
			return info.render_final.draw_call_count;
		case RS::INFO_2D_ITEMS_IN_FRAME:
			return info.render_final._2d_item_count;
		case RS::INFO_2D_DRAW_CALLS_IN_FRAME:
			return info.render_final._2d_draw_call_count;
		case RS::INFO_USAGE_VIDEO_MEM_TOTAL:
			return 0; //no idea
		case RS::INFO_VIDEO_MEM_USED:
			return info.vertex_mem + info.texture_mem;
		case RS::INFO_TEXTURE_MEM_USED:
			return info.texture_mem;
		case RS::INFO_VERTEX_MEM_USED:
			return info.vertex_mem;
		default:
			return 0; //no idea either
	}
}

String RasterizerStorageGLES2::get_video_adapter_name() const {
	return (const char *)glGetString(GL_RENDERER);
}

String RasterizerStorageGLES2::get_video_adapter_vendor() const {
	return (const char *)glGetString(GL_VENDOR);
}

void RasterizerStorageGLES2::initialize() {
	RasterizerStorageGLES2::system_fbo = 0;

	{
		const GLubyte *extension_string = glGetString(GL_EXTENSIONS);

		Vector<String> extensions = String((const char *)extension_string).split(" ");

		for (int i = 0; i < extensions.size(); i++) {
			config.extensions.insert(extensions[i]);
		}
	}

	config.keep_original_textures = false;
	config.shrink_textures_x2 = false;
	config.depth_internalformat = GL_DEPTH_COMPONENT;
	config.depth_type = GL_UNSIGNED_INT;

	// Initialize GLWrapper early on, as required for any calls to glActiveTexture.
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &config.max_texture_image_units);
	gl_wrapper.initialize(config.max_texture_image_units);

#ifdef GLES_OVER_GL
	config.float_texture_supported = true;
	config.s3tc_supported = true;
	config.pvrtc_supported = false;
	config.etc1_supported = false;
	config.support_npot_repeat_mipmap = true;
	config.depth_buffer_internalformat = GL_DEPTH_COMPONENT24;
#else
	config.float_texture_supported = config.extensions.has("GL_ARB_texture_float") || config.extensions.has("GL_OES_texture_float");
	config.s3tc_supported = config.extensions.has("GL_EXT_texture_compression_s3tc") || config.extensions.has("WEBGL_compressed_texture_s3tc");
	config.etc1_supported = config.extensions.has("GL_OES_compressed_ETC1_RGB8_texture") || config.extensions.has("WEBGL_compressed_texture_etc1");
	config.pvrtc_supported = config.extensions.has("GL_IMG_texture_compression_pvrtc") || config.extensions.has("WEBGL_compressed_texture_pvrtc");
	config.support_npot_repeat_mipmap = config.extensions.has("GL_OES_texture_npot");

	// If the desktop build is using S3TC, and you export / run from the IDE for android, if the device supports
	// S3TC it will crash trying to load these textures, as they are not exported in the APK. This is a simple way
	// to prevent Android devices trying to load S3TC, by faking lack of hardware support.
#if defined(ANDROID_ENABLED) || defined(IPHONE_ENABLED)
	config.s3tc_supported = false;
#endif

#ifdef JAVASCRIPT_ENABLED
	// RenderBuffer internal format must be 16 bits in WebGL,
	// but depth_texture should default to 32 always
	// if the implementation doesn't support 32, it should just quietly use 16 instead
	// https://www.khronos.org/registry/webgl/extensions/WEBGL_depth_texture/
	config.depth_buffer_internalformat = GL_DEPTH_COMPONENT16;
	config.depth_type = GL_UNSIGNED_INT;
#else
	// on mobile check for 24 bit depth support for RenderBufferStorage
	if (config.extensions.has("GL_OES_depth24")) {
		config.depth_buffer_internalformat = _DEPTH_COMPONENT24_OES;
		config.depth_type = GL_UNSIGNED_INT;
	} else {
		config.depth_buffer_internalformat = GL_DEPTH_COMPONENT16;
		config.depth_type = GL_UNSIGNED_SHORT;
	}
#endif
#endif

#ifndef GLES_OVER_GL
	//Manually load extensions for android and ios

#ifdef IPHONE_ENABLED
	// appears that IPhone doesn't need to dlopen TODO: test this rigorously before removing
	//void *gles2_lib = dlopen(NULL, RTLD_LAZY);
	//glRenderbufferStorageMultisampleAPPLE = dlsym(gles2_lib, "glRenderbufferStorageMultisampleAPPLE");
	//glResolveMultisampleFramebufferAPPLE = dlsym(gles2_lib, "glResolveMultisampleFramebufferAPPLE");
#elif ANDROID_ENABLED

	void *gles2_lib = dlopen("libGLESv2.so", RTLD_LAZY);
	glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)dlsym(gles2_lib, "glRenderbufferStorageMultisampleEXT");
	glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)dlsym(gles2_lib, "glFramebufferTexture2DMultisampleEXT");
#endif
#endif

	// Check for multisample support
	config.multisample_supported = config.extensions.has("GL_EXT_framebuffer_multisample") || config.extensions.has("GL_EXT_multisampled_render_to_texture") || config.extensions.has("GL_APPLE_framebuffer_multisample");

#ifdef GLES_OVER_GL
	//TODO: causes huge problems with desktop video drivers. Making false for now, needs to be true to render SCREEN_TEXTURE mipmaps
	config.render_to_mipmap_supported = false;
#else
	//check if mipmaps can be used for SCREEN_TEXTURE and Glow on Mobile and web platforms
	config.render_to_mipmap_supported = config.extensions.has("GL_OES_fbo_render_mipmap") && config.extensions.has("GL_EXT_texture_lod");
#endif

#ifdef GLES_OVER_GL
	config.use_rgba_2d_shadows = false;
	config.support_depth_texture = true;
	config.use_rgba_3d_shadows = false;
	config.support_depth_cubemaps = true;
#else
	config.use_rgba_2d_shadows = !(config.float_texture_supported && config.extensions.has("GL_EXT_texture_rg"));
	config.support_depth_texture = config.extensions.has("GL_OES_depth_texture") || config.extensions.has("WEBGL_depth_texture");
	config.use_rgba_3d_shadows = !config.support_depth_texture;
	config.support_depth_cubemaps = config.extensions.has("GL_OES_depth_texture_cube_map");
#endif

#ifdef GLES_OVER_GL
	config.support_32_bits_indices = true;
#else
	config.support_32_bits_indices = config.extensions.has("GL_OES_element_index_uint");
#endif

#ifdef GLES_OVER_GL
	config.support_write_depth = true;
#elif defined(JAVASCRIPT_ENABLED)
	config.support_write_depth = false;
#else
	config.support_write_depth = config.extensions.has("GL_EXT_frag_depth");
#endif

	config.support_half_float_vertices = true;
//every platform should support this except web, iOS has issues with their support, so add option to disable
#ifdef JAVASCRIPT_ENABLED
	config.support_half_float_vertices = false;
#endif
	bool disable_half_float = GLOBAL_GET("rendering/gles2/compatibility/disable_half_float");
	if (disable_half_float) {
		config.support_half_float_vertices = false;
	}

	config.rgtc_supported = config.extensions.has("GL_EXT_texture_compression_rgtc") || config.extensions.has("GL_ARB_texture_compression_rgtc") || config.extensions.has("EXT_texture_compression_rgtc");
	config.bptc_supported = config.extensions.has("GL_ARB_texture_compression_bptc") || config.extensions.has("EXT_texture_compression_bptc");

	config.anisotropic_level = 1.0;
	config.use_anisotropic_filter = config.extensions.has("GL_EXT_texture_filter_anisotropic");
	if (config.use_anisotropic_filter) {
		glGetFloatv(_GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &config.anisotropic_level);
		config.anisotropic_level = MIN(int(ProjectSettings::get_singleton()->get("rendering/quality/filters/anisotropic_filter_level")), config.anisotropic_level);
	}

	//determine formats for depth textures (or renderbuffers)
	if (config.support_depth_texture) {
		// Will use texture for depth
		// have to manually see if we can create a valid framebuffer texture using UNSIGNED_INT,
		// as there is no extension to test for this.
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		GLuint depth;
		glGenTextures(1, &depth);
		glBindTexture(GL_TEXTURE_2D, depth);
		glTexImage2D(GL_TEXTURE_2D, 0, config.depth_internalformat, 32, 32, 0, GL_DEPTH_COMPONENT, config.depth_type, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		glBindFramebuffer(GL_FRAMEBUFFER, system_fbo);
		glDeleteFramebuffers(1, &fbo);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDeleteTextures(1, &depth);

		if (status != GL_FRAMEBUFFER_COMPLETE) {
			// If it fails, test to see if it supports a framebuffer texture using UNSIGNED_SHORT
			// This is needed because many OSX devices don't support either UNSIGNED_INT or UNSIGNED_SHORT
#ifdef GLES_OVER_GL
			config.depth_internalformat = GL_DEPTH_COMPONENT16;
#else
			// OES_depth_texture extension only specifies GL_DEPTH_COMPONENT.
			config.depth_internalformat = GL_DEPTH_COMPONENT;
#endif
			config.depth_type = GL_UNSIGNED_SHORT;

			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			glGenTextures(1, &depth);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, config.depth_internalformat, 32, 32, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);

			status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				//if it fails again depth textures aren't supported, use rgba shadows and renderbuffer for depth
				config.support_depth_texture = false;
				config.use_rgba_3d_shadows = true;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, system_fbo);
			glDeleteFramebuffers(1, &fbo);
			glBindTexture(GL_TEXTURE_2D, 0);
			glDeleteTextures(1, &depth);
		}
	}

	//picky requirements for these
	config.support_shadow_cubemaps = config.support_depth_texture && config.support_write_depth && config.support_depth_cubemaps;

	if (!config.support_shadow_cubemaps) {
		print_verbose("OmniLight cubemap shadows are not supported by this GPU. Falling back to dual paraboloid shadows for all omni lights (faster but less precise).");
	}

	frame.count = 0;
	frame.delta = 0;
	frame.current_rt = nullptr;
	frame.clear_request = false;

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &config.max_vertex_texture_image_units);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &config.max_texture_size);
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &config.max_cubemap_texture_size);
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, config.max_viewport_dimensions);

	shaders.copy.init();

	{
		// quad for copying stuff

		glGenBuffers(1, &resources.quadie);
		glBindBuffer(GL_ARRAY_BUFFER, resources.quadie);
		{
			const float qv[16] = {
				-1,
				-1,
				0,
				0,
				-1,
				1,
				0,
				1,
				1,
				1,
				1,
				1,
				1,
				-1,
				1,
				0,
			};

			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, qv, GL_STATIC_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	{
		// Generate default textures.

		// Opaque white color.
		glGenTextures(1, &resources.white_tex);
		unsigned char whitetexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i++) {
			whitetexdata[i] = 255;
		}
		gl_wrapper.gl_active_texture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.white_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, whitetexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Opaque black color.
		glGenTextures(1, &resources.black_tex);
		unsigned char blacktexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i++) {
			blacktexdata[i] = 0;
		}
		gl_wrapper.gl_active_texture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.black_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, blacktexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Transparent black color.
		glGenTextures(1, &resources.transparent_tex);
		unsigned char transparenttexdata[8 * 8 * 4];
		for (int i = 0; i < 8 * 8 * 4; i++) {
			transparenttexdata[i] = 0;
		}
		gl_wrapper.gl_active_texture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.transparent_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, transparenttexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Opaque "flat" normal map color.
		glGenTextures(1, &resources.normal_tex);
		unsigned char normaltexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i += 3) {
			normaltexdata[i + 0] = 128;
			normaltexdata[i + 1] = 128;
			normaltexdata[i + 2] = 255;
		}
		gl_wrapper.gl_active_texture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.normal_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, normaltexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Opaque "flat" flowmap color.
		glGenTextures(1, &resources.aniso_tex);
		unsigned char anisotexdata[8 * 8 * 3];
		for (int i = 0; i < 8 * 8 * 3; i += 3) {
			anisotexdata[i + 0] = 255;
			anisotexdata[i + 1] = 128;
			anisotexdata[i + 2] = 0;
		}
		gl_wrapper.gl_active_texture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.aniso_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, anisotexdata);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// blend buffer
	{
		resources.blend_shape_transform_cpu_buffer_size = 0;
	}

	// radical inverse vdc cache texture
	// used for cubemap filtering
	if (true /*||config.float_texture_supported*/) { //uint8 is similar and works everywhere
		glGenTextures(1, &resources.radical_inverse_vdc_cache_tex);

		gl_wrapper.gl_active_texture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, resources.radical_inverse_vdc_cache_tex);

		uint8_t radical_inverse[512];

		for (uint32_t i = 0; i < 512; i++) {
			uint32_t bits = i;

			bits = (bits << 16) | (bits >> 16);
			bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
			bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
			bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
			bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);

			float value = float(bits) * 2.3283064365386963e-10;
			radical_inverse[i] = uint8_t(CLAMP(value * 255.0, 0, 255));
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 512, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, radical_inverse);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //need this for proper sampling

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	{
		glGenFramebuffers(1, &resources.mipmap_blur_fbo);
		glGenTextures(1, &resources.mipmap_blur_color);
	}

#ifdef GLES_OVER_GL
	//this needs to be enabled manually in OpenGL 2.1

	if (config.extensions.has("GL_ARB_seamless_cube_map")) {
		glEnable(_EXT_TEXTURE_CUBE_MAP_SEAMLESS);
	}
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

	config.force_vertex_shading = GLOBAL_GET("rendering/quality/shading/force_vertex_shading");
	config.use_fast_texture_filter = GLOBAL_GET("rendering/quality/filters/use_nearest_mipmap_filter");

	config.use_physical_light_attenuation = GLOBAL_GET("rendering/quality/shading/use_physical_light_attenuation");

	int orphan_mode = GLOBAL_GET("rendering/2d/opengl/legacy_orphan_buffers");
	switch (orphan_mode) {
		default: {
			config.should_orphan = true;
		} break;
		case 1: {
			config.should_orphan = false;
		} break;
		case 2: {
			config.should_orphan = true;
		} break;
	}
}

void RasterizerStorageGLES2::finalize() {
}

void RasterizerStorageGLES2::_copy_screen() {
	bind_quad_array();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void RasterizerStorageGLES2::update_dirty_resources() {
	update_dirty_shaders();
	update_dirty_materials();
	update_dirty_blend_shapes();
	update_dirty_multimeshes();
}

RasterizerStorageGLES2::RasterizerStorageGLES2() {
	RasterizerStorageGLES2::system_fbo = 0;
	config.should_orphan = true;
}

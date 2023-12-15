/*************************************************************************/
/*  rasterizer_scene_gles2.cpp                                           */
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

#include "rasterizer_scene_gles2.h"

#include "core/config/project_settings.h"
#include "core/containers/vmap.h"
#include "core/math/math_funcs.h"
#include "core/math/transform.h"
#include "core/os/os.h"
#include "rasterizer_canvas_gles2.h"
#include "servers/rendering/rendering_server_raster.h"

#ifndef GLES_OVER_GL
#define glClearDepth glClearDepthf
#endif

#ifndef GLES_OVER_GL
#ifdef IPHONE_ENABLED
#include <OpenGLES/ES2/glext.h>
//void *glResolveMultisampleFramebufferAPPLE;

#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#endif

////////////////////////////

void RasterizerSceneGLES2::_add_geometry(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, int p_material, bool p_depth_pass) {
	RasterizerStorageGLES2::Material *material = nullptr;
	RID material_src;

	if (p_instance->material_override.is_valid()) {
		material_src = p_instance->material_override;
	} else if (p_material >= 0) {
		material_src = p_instance->materials[p_material];
	} else {
		material_src = p_geometry->material;
	}

	if (material_src.is_valid()) {
		material = storage->material_owner.getornull(material_src);

		if (!material->shader || !material->shader->valid) {
			material = nullptr;
		}
	}

	if (!material) {
		material = storage->material_owner.getptr(default_material);
	}

	ERR_FAIL_COND(!material);

	_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass);

	while (material->next_pass.is_valid()) {
		material = storage->material_owner.getornull(material->next_pass);

		if (!material || !material->shader || !material->shader->valid) {
			break;
		}

		_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass);
	}

	// Repeat the "nested chain" logic also for the overlay
	if (p_instance->material_overlay.is_valid()) {
		material = storage->material_owner.getornull(p_instance->material_overlay);

		if (!material || !material->shader || !material->shader->valid) {
			return;
		}

		_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass);

		while (material->next_pass.is_valid()) {
			material = storage->material_owner.getornull(material->next_pass);

			if (!material || !material->shader || !material->shader->valid) {
				break;
			}

			_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass);
		}
	}
}
void RasterizerSceneGLES2::_add_geometry_with_material(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, RasterizerStorageGLES2::Material *p_material, bool p_depth_pass) {
	bool has_base_alpha = (p_material->shader->spatial.uses_alpha && !p_material->shader->spatial.uses_alpha_scissor) || p_material->shader->spatial.uses_screen_texture || p_material->shader->spatial.uses_depth_texture;
	bool has_blend_alpha = p_material->shader->spatial.blend_mode != RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_MIX;
	bool has_alpha = has_base_alpha || has_blend_alpha;

	bool mirror = p_instance->mirror;

	if (p_material->shader->spatial.cull_mode == RasterizerStorageGLES2::Shader::Spatial::CULL_MODE_DISABLED) {
		mirror = false;
	} else if (p_material->shader->spatial.cull_mode == RasterizerStorageGLES2::Shader::Spatial::CULL_MODE_FRONT) {
		mirror = !mirror;
	}

	//if (p_material->shader->spatial.uses_sss) {
	//	state.used_sss = true;
	//}

	if (p_material->shader->spatial.uses_screen_texture) {
		state.used_screen_texture = true;
	}

	if (p_depth_pass) {
		if (has_blend_alpha || p_material->shader->spatial.uses_depth_texture || (has_base_alpha && p_material->shader->spatial.depth_draw_mode != RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS)) {
			return; //bye
		}

		if (!p_material->shader->spatial.uses_alpha_scissor && !p_material->shader->spatial.writes_modelview_or_projection && !p_material->shader->spatial.uses_vertex && !p_material->shader->spatial.uses_discard && p_material->shader->spatial.depth_draw_mode != RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS) {
			//shader does not use discard and does not write a vertex position, use generic material
			p_material = storage->material_owner.getptr(p_material->shader->spatial.uses_world_coordinates ? default_worldcoord_material : default_material);
		}

		has_alpha = false;
	}

	RenderList::Element *e = (has_alpha || p_material->shader->spatial.no_depth_test) ? render_list.add_alpha_element() : render_list.add_element();

	if (!e) {
		return;
	}

	e->geometry = p_geometry;
	e->material = p_material;
	e->instance = p_instance;
	e->owner = p_owner;
	e->sort_key = 0;
	e->depth_key = 0;
	e->use_accum = false;
	e->light_index = RenderList::MAX_LIGHTS;
	e->use_accum_ptr = &e->use_accum;
	e->instancing = (e->instance->base_type == RS::INSTANCE_MULTIMESH) ? 1 : 0;
	e->front_facing = false;

	if (e->geometry->last_pass != render_pass) {
		e->geometry->last_pass = render_pass;
		e->geometry->index = current_geometry_index++;
	}

	e->geometry_index = e->geometry->index;

	if (e->material->last_pass != render_pass) {
		e->material->last_pass = render_pass;
		e->material->index = current_material_index++;

		if (e->material->shader->last_pass != render_pass) {
			e->material->shader->index = current_shader_index++;
		}
	}

	e->material_index = e->material->index;

	if (mirror) {
		e->front_facing = true;
	}

	if (!p_depth_pass) {
		e->depth_layer = e->instance->depth_layer;
		e->priority = p_material->render_priority;

		if (has_alpha && p_material->shader->spatial.depth_draw_mode == RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS) {
			//add element to opaque
			RenderList::Element *eo = render_list.add_element();
			*eo = *e;
			eo->use_accum_ptr = &eo->use_accum;
		}

		//add directional lights

		e->light_mode = LIGHTMODE_UNSHADED;
	}

	// do not add anything here, as lights are duplicated elements..

	if (p_material->shader->spatial.uses_time) {
		RenderingServerRaster::redraw_request(false);
	}
}

void RasterizerSceneGLES2::_copy_texture_to_buffer(GLuint p_texture, GLuint p_buffer) {
	//copy to front buffer
	glBindFramebuffer(GL_FRAMEBUFFER, p_buffer);

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glColorMask(1, 1, 1, 1);

	WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_texture);

	glViewport(0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height);

	storage->shaders.copy.bind();

	storage->bind_quad_array();
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RasterizerSceneGLES2::_fill_render_list(InstanceBase **p_cull_result, int p_cull_count, bool p_depth_pass) {
	render_pass++;
	current_material_index = 0;
	current_geometry_index = 0;
	current_shader_index = 0;

	for (int i = 0; i < p_cull_count; i++) {
		InstanceBase *instance = p_cull_result[i];

		switch (instance->base_type) {
			case RS::INSTANCE_MESH: {
				RasterizerStorageGLES2::Mesh *mesh = storage->mesh_owner.getornull(instance->base);
				ERR_CONTINUE(!mesh);

				int num_surfaces = mesh->surfaces.size();

				for (int j = 0; j < num_surfaces; j++) {
					int material_index = instance->materials[j].is_valid() ? j : -1;

					RasterizerStorageGLES2::Surface *surface = mesh->surfaces[j];

					_add_geometry(surface, instance, nullptr, material_index, p_depth_pass);
				}

			} break;

			case RS::INSTANCE_MULTIMESH: {
				RasterizerStorageGLES2::MultiMesh *multi_mesh = storage->multimesh_owner.getptr(instance->base);
				ERR_CONTINUE(!multi_mesh);

				if (multi_mesh->size == 0 || multi_mesh->visible_instances == 0) {
					continue;
				}

				RasterizerStorageGLES2::Mesh *mesh = storage->mesh_owner.getptr(multi_mesh->mesh);
				if (!mesh) {
					continue;
				}

				int ssize = mesh->surfaces.size();

				for (int j = 0; j < ssize; j++) {
					RasterizerStorageGLES2::Surface *s = mesh->surfaces[j];
					_add_geometry(s, instance, multi_mesh, -1, p_depth_pass);
				}
			} break;

			default: {
			}
		}
	}
}

const GLenum RasterizerSceneGLES2::gl_primitive[] = {
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN
};

void RasterizerSceneGLES2::_set_cull(bool p_front, bool p_disabled, bool p_reverse_cull) {
	bool front = p_front;
	if (p_reverse_cull) {
		front = !front;
	}

	if (p_disabled != state.cull_disabled) {
		if (p_disabled) {
			glDisable(GL_CULL_FACE);
		} else {
			glEnable(GL_CULL_FACE);
		}

		state.cull_disabled = p_disabled;
	}

	if (front != state.cull_front) {
		glCullFace(front ? GL_FRONT : GL_BACK);
		state.cull_front = front;
	}
}

bool RasterizerSceneGLES2::_setup_material(RasterizerStorageGLES2::Material *p_material, bool p_alpha_pass, Size2i p_skeleton_tex_size) {
	// material parameters

	state.scene_shader.set_custom_shader(p_material->shader->custom_code_id);

	if (p_material->shader->spatial.uses_screen_texture && storage->frame.current_rt) {
		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
		glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->copy_screen_effect.color);
	}

	if (p_material->shader->spatial.uses_depth_texture && storage->frame.current_rt) {
		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0 + storage->config.max_texture_image_units - 4);
		glBindTexture(GL_TEXTURE_2D, storage->frame.current_rt->depth);
	}

	bool shader_rebind = state.scene_shader.bind();

	if (p_material->shader->spatial.no_depth_test || p_material->shader->spatial.uses_depth_texture) {
		glDisable(GL_DEPTH_TEST);
	} else {
		glEnable(GL_DEPTH_TEST);
	}

	switch (p_material->shader->spatial.depth_draw_mode) {
		case RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS:
		case RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_OPAQUE: {
			glDepthMask(!p_alpha_pass && !p_material->shader->spatial.uses_depth_texture);
		} break;
		case RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_ALWAYS: {
			glDepthMask(GL_TRUE && !p_material->shader->spatial.uses_depth_texture);
		} break;
		case RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_NEVER: {
			glDepthMask(GL_FALSE);
		} break;
	}

	int tc = p_material->textures.size();
	const Pair<StringName, RID> *textures = p_material->textures.ptr();

	const ShaderLanguage::ShaderNode::Uniform::Hint *texture_hints = p_material->shader->texture_hints.ptr();

	state.scene_shader.set_uniform(SceneShaderGLES2::SKELETON_TEXTURE_SIZE, p_skeleton_tex_size);

	state.current_main_tex = 0;

	for (int i = 0; i < tc; i++) {
		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0 + i);

		RasterizerStorageGLES2::Texture *t = storage->texture_owner.getornull(textures[i].second);

		if (!t) {
			switch (texture_hints[i]) {
				case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK_ALBEDO:
				case ShaderLanguage::ShaderNode::Uniform::HINT_BLACK: {
					glBindTexture(GL_TEXTURE_2D, storage->resources.black_tex);
				} break;
				case ShaderLanguage::ShaderNode::Uniform::HINT_TRANSPARENT: {
					glBindTexture(GL_TEXTURE_2D, storage->resources.transparent_tex);
				} break;
				case ShaderLanguage::ShaderNode::Uniform::HINT_ANISO: {
					glBindTexture(GL_TEXTURE_2D, storage->resources.aniso_tex);
				} break;
				case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
					glBindTexture(GL_TEXTURE_2D, storage->resources.normal_tex);
				} break;
				default: {
					glBindTexture(GL_TEXTURE_2D, storage->resources.white_tex);
				} break;
			}

			continue;
		}

		if (t->redraw_if_visible) { //must check before proxy because this is often used with proxies
			RenderingServerRaster::redraw_request(false);
		}

		t = t->get_ptr();

#ifdef TOOLS_ENABLED
		if (t->detect_3d) {
			t->detect_3d(t->detect_3d_ud);
		}
#endif

#ifdef TOOLS_ENABLED
		if (t->detect_normal && texture_hints[i] == ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL) {
			t->detect_normal(t->detect_normal_ud);
		}
#endif
		if (t->render_target) {
			t->render_target->used_in_frame = true;
		}

		glBindTexture(t->target, t->tex_id);
		if (i == 0) {
			state.current_main_tex = t->tex_id;
		}
	}
	state.scene_shader.use_material((void *)p_material);

	return shader_rebind;
}

void RasterizerSceneGLES2::_setup_geometry(RenderList::Element *p_element) {
	switch (p_element->instance->base_type) {
		case RS::INSTANCE_MESH: {
			RasterizerStorageGLES2::Surface *s = static_cast<RasterizerStorageGLES2::Surface *>(p_element->geometry);

			if (s->index_array_len > 0) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->index_id);
			}

			for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
				if (s->attribs[i].enabled) {
					glEnableVertexAttribArray(i);

					if (!s->blend_shape_data.empty() && i != RS::ARRAY_BONES && s->blend_shape_buffer_size > 0) {
						glBindBuffer(GL_ARRAY_BUFFER, s->blend_shape_buffer_id);
						// When using octahedral compression (2 component normal/tangent)
						// decompression changes the component count to 3/4
						int size;
						switch (i) {
							case RS::ARRAY_NORMAL: {
								size = 3;
							} break;
							case RS::ARRAY_TANGENT: {
								size = 4;
							} break;
							default:
								size = s->attribs[i].size;
						}

						glVertexAttribPointer(s->attribs[i].index, size, GL_FLOAT, GL_FALSE, 8 * 4 * sizeof(float), CAST_INT_TO_UCHAR_PTR(i * 4 * sizeof(float)));

					} else {
						glBindBuffer(GL_ARRAY_BUFFER, s->vertex_id);
						glVertexAttribPointer(s->attribs[i].index, s->attribs[i].size, s->attribs[i].type, s->attribs[i].normalized, s->attribs[i].stride, CAST_INT_TO_UCHAR_PTR(s->attribs[i].offset));
					}
				} else {
					glDisableVertexAttribArray(i);
					switch (i) {
						case RS::ARRAY_NORMAL: {
							glVertexAttrib4f(RS::ARRAY_NORMAL, 0.0, 0.0, 1, 1);
						} break;
						case RS::ARRAY_COLOR: {
							glVertexAttrib4f(RS::ARRAY_COLOR, 1, 1, 1, 1);

						} break;
						default: {
						}
					}
				}
			}

		} break;

		case RS::INSTANCE_MULTIMESH: {
			RasterizerStorageGLES2::Surface *s = static_cast<RasterizerStorageGLES2::Surface *>(p_element->geometry);

			glBindBuffer(GL_ARRAY_BUFFER, s->vertex_id);

			if (s->index_array_len > 0) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s->index_id);
			}

			for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
				if (s->attribs[i].enabled) {
					glEnableVertexAttribArray(i);
					glVertexAttribPointer(s->attribs[i].index, s->attribs[i].size, s->attribs[i].type, s->attribs[i].normalized, s->attribs[i].stride, CAST_INT_TO_UCHAR_PTR(s->attribs[i].offset));
				} else {
					glDisableVertexAttribArray(i);
					switch (i) {
						case RS::ARRAY_NORMAL: {
							glVertexAttrib4f(RS::ARRAY_NORMAL, 0.0, 0.0, 1, 1);
						} break;
						case RS::ARRAY_COLOR: {
							glVertexAttrib4f(RS::ARRAY_COLOR, 1, 1, 1, 1);

						} break;
						default: {
						}
					}
				}
			}

			// prepare multimesh (disable)
			glDisableVertexAttribArray(INSTANCE_ATTRIB_BASE + 0);
			glDisableVertexAttribArray(INSTANCE_ATTRIB_BASE + 1);
			glDisableVertexAttribArray(INSTANCE_ATTRIB_BASE + 2);
			glDisableVertexAttribArray(INSTANCE_ATTRIB_BASE + 3);
			glDisableVertexAttribArray(INSTANCE_ATTRIB_BASE + 4);
			glDisableVertexAttribArray(INSTANCE_BONE_BASE + 0);
			glDisableVertexAttribArray(INSTANCE_BONE_BASE + 1);
			glDisableVertexAttribArray(INSTANCE_BONE_BASE + 2);

		} break;

		default: {
		}
	}
}

void RasterizerSceneGLES2::_render_geometry(RenderList::Element *p_element) {
	switch (p_element->instance->base_type) {
		case RS::INSTANCE_MESH: {
			RasterizerStorageGLES2::Surface *s = static_cast<RasterizerStorageGLES2::Surface *>(p_element->geometry);

			// drawing

			if (s->index_array_len > 0) {
				glDrawElements(gl_primitive[s->primitive], s->index_array_len, (s->array_len >= (1 << 16)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, nullptr);
				storage->info.render.vertices_count += s->index_array_len;
			} else {
				glDrawArrays(gl_primitive[s->primitive], 0, s->array_len);
				storage->info.render.vertices_count += s->array_len;
			}
			/*
			if (p_element->instance->skeleton.is_valid() && s->attribs[RS::ARRAY_BONES].enabled && s->attribs[RS::ARRAY_WEIGHTS].enabled) {
				//clean up after skeleton
				glBindBuffer(GL_ARRAY_BUFFER, storage->resources.skeleton_transform_buffer);

				glDisableVertexAttribArray(RS::ARRAY_MAX + 0);
				glDisableVertexAttribArray(RS::ARRAY_MAX + 1);
				glDisableVertexAttribArray(RS::ARRAY_MAX + 2);

				glVertexAttrib4f(RS::ARRAY_MAX + 0, 1, 0, 0, 0);
				glVertexAttrib4f(RS::ARRAY_MAX + 1, 0, 1, 0, 0);
				glVertexAttrib4f(RS::ARRAY_MAX + 2, 0, 0, 1, 0);
			}
*/
		} break;

		case RS::INSTANCE_MULTIMESH: {
			RasterizerStorageGLES2::MultiMesh *multi_mesh = static_cast<RasterizerStorageGLES2::MultiMesh *>(p_element->owner);
			RasterizerStorageGLES2::Surface *s = static_cast<RasterizerStorageGLES2::Surface *>(p_element->geometry);

			int amount = MIN(multi_mesh->size, multi_mesh->visible_instances);

			if (amount == -1) {
				amount = multi_mesh->size;
			}

			if (!amount) {
				return;
			}

			int stride = multi_mesh->color_floats + multi_mesh->custom_data_floats + multi_mesh->xform_floats;

			int color_ofs = multi_mesh->xform_floats;
			int custom_data_ofs = color_ofs + multi_mesh->color_floats;

			// drawing

			const float *base_buffer = multi_mesh->data.ptr();

			for (int i = 0; i < amount; i++) {
				const float *buffer = base_buffer + i * stride;

				{
					glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 0, &buffer[0]);
					glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 1, &buffer[4]);
					glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 2, &buffer[8]);
				}

				if (multi_mesh->color_floats) {
					if (multi_mesh->color_format == RS::MULTIMESH_COLOR_8BIT) {
						uint8_t *color_data = (uint8_t *)(buffer + color_ofs);
						glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 3, color_data[0] / 255.0, color_data[1] / 255.0, color_data[2] / 255.0, color_data[3] / 255.0);
					} else {
						glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 3, buffer + color_ofs);
					}
				} else {
					glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 3, 1.0, 1.0, 1.0, 1.0);
				}

				if (multi_mesh->custom_data_floats) {
					if (multi_mesh->custom_data_format == RS::MULTIMESH_CUSTOM_DATA_8BIT) {
						uint8_t *custom_data = (uint8_t *)(buffer + custom_data_ofs);
						glVertexAttrib4f(INSTANCE_ATTRIB_BASE + 4, custom_data[0] / 255.0, custom_data[1] / 255.0, custom_data[2] / 255.0, custom_data[3] / 255.0);
					} else {
						glVertexAttrib4fv(INSTANCE_ATTRIB_BASE + 4, buffer + custom_data_ofs);
					}
				}

				if (s->index_array_len > 0) {
					glDrawElements(gl_primitive[s->primitive], s->index_array_len, (s->array_len >= (1 << 16)) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, nullptr);
					storage->info.render.vertices_count += s->index_array_len;
				} else {
					glDrawArrays(gl_primitive[s->primitive], 0, s->array_len);
					storage->info.render.vertices_count += s->array_len;
				}
			}

		} break;
		default: {
		}
	}
}

void RasterizerSceneGLES2::_render_render_list(RenderList::Element **p_elements, int p_element_count, const Transform &p_view_transform, const Projection &p_projection, const int p_eye, bool p_reverse_cull, bool p_alpha_pass) {
	Vector2 viewport_size = state.viewport_size;

	Vector2 screen_pixel_size = state.screen_pixel_size;

	bool use_radiance_map = false;

	bool prev_unshaded = false;
	bool prev_instancing = false;
	bool prev_depth_prepass = false;
	state.scene_shader.set_conditional(SceneShaderGLES2::SHADELESS, false);
	RasterizerStorageGLES2::Material *prev_material = nullptr;
	RasterizerStorageGLES2::Geometry *prev_geometry = nullptr;
	RasterizerStorageGLES2::GeometryOwner *prev_owner = nullptr;

	bool prev_octahedral_compression = false;

	Transform view_transform_inverse = p_view_transform.inverse();
	Projection projection_inverse = p_projection.inverse();

	bool prev_base_pass = false;
	bool prev_vertex_lit = false;

	int prev_blend_mode = -2; //will always catch the first go

	state.cull_front = false;
	state.cull_disabled = false;
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	if (p_alpha_pass) {
		glEnable(GL_BLEND);
	} else {
		glDisable(GL_BLEND);
	}

	bool using_fog = false;

	storage->info.render.draw_call_count += p_element_count;

	for (int i = 0; i < p_element_count; i++) {
		RenderList::Element *e = p_elements[i];

		RasterizerStorageGLES2::Material *material = e->material;

		bool rebind = false;
		bool accum_pass = *e->use_accum_ptr;
		*e->use_accum_ptr = true; //set to accum for next time this is found

		if (material->shader) {
			bool unshaded = material->shader->spatial.unshaded;

			if (unshaded != prev_unshaded) {
				rebind = true;
				if (unshaded) {
					state.scene_shader.set_conditional(SceneShaderGLES2::SHADELESS, true);
					state.scene_shader.set_conditional(SceneShaderGLES2::USE_RADIANCE_MAP, false);
					state.scene_shader.set_conditional(SceneShaderGLES2::USE_LIGHTING, false);
				} else {
					state.scene_shader.set_conditional(SceneShaderGLES2::SHADELESS, false);
					state.scene_shader.set_conditional(SceneShaderGLES2::USE_RADIANCE_MAP, use_radiance_map);
				}

				prev_unshaded = unshaded;
			}

			bool base_pass = !accum_pass && !unshaded; //conditions for a base pass

			if (base_pass != prev_base_pass) {
				state.scene_shader.set_conditional(SceneShaderGLES2::BASE_PASS, base_pass);
				rebind = true;
				prev_base_pass = base_pass;
			}

			int blend_mode = p_alpha_pass ? material->shader->spatial.blend_mode : -1; // -1 no blend, no mix

			if (prev_blend_mode != blend_mode) {
				if (prev_blend_mode == -1 && blend_mode != -1) {
					//does blend
					glEnable(GL_BLEND);
				} else if (blend_mode == -1 && prev_blend_mode != -1) {
					//do not blend
					glDisable(GL_BLEND);
				}

				switch (blend_mode) {
					//-1 not handled because not blend is enabled anyway
					case RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_MIX: {
						glBlendEquation(GL_FUNC_ADD);
						if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
							glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
						} else {
							glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
						}

					} break;
					case RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_ADD: {
						glBlendEquation(GL_FUNC_ADD);
						glBlendFunc(p_alpha_pass ? GL_SRC_ALPHA : GL_ONE, GL_ONE);

					} break;
					case RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_SUB: {
						glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
						glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					} break;
					case RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_MUL: {
						glBlendEquation(GL_FUNC_ADD);
						if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
							glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
						} else {
							glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_ZERO, GL_ONE);
						}

					} break;
				}

				prev_blend_mode = blend_mode;
			}

			//condition to enable vertex lighting on this object
			bool vertex_lit = (material->shader->spatial.uses_vertex_lighting || storage->config.force_vertex_shading) && ((!unshaded) || using_fog); //fog forces vertex lighting because it still applies even if unshaded or no fog

			if (vertex_lit != prev_vertex_lit) {
				state.scene_shader.set_conditional(SceneShaderGLES2::USE_VERTEX_LIGHTING, vertex_lit);
				prev_vertex_lit = vertex_lit;
				rebind = true;
			}
		}

		bool depth_prepass = false;

		if (!p_alpha_pass && material->shader->spatial.depth_draw_mode == RasterizerStorageGLES2::Shader::Spatial::DEPTH_DRAW_ALPHA_PREPASS) {
			depth_prepass = true;
		}

		if (depth_prepass != prev_depth_prepass) {
			state.scene_shader.set_conditional(SceneShaderGLES2::USE_DEPTH_PREPASS, depth_prepass);
			prev_depth_prepass = depth_prepass;
			rebind = true;
		}

		bool instancing = e->instance->base_type == RS::INSTANCE_MULTIMESH;

		if (instancing != prev_instancing) {
			state.scene_shader.set_conditional(SceneShaderGLES2::USE_INSTANCING, instancing);
			rebind = true;
		}

		state.scene_shader.set_conditional(SceneShaderGLES2::USE_SKELETON, false);
		state.scene_shader.set_conditional(SceneShaderGLES2::USE_SKELETON_SOFTWARE, false);

		if (e->owner != prev_owner || e->geometry != prev_geometry) {
			_setup_geometry(e);
			storage->info.render.surface_switch_count++;
		}

		state.scene_shader.set_conditional(SceneShaderGLES2::USE_PHYSICAL_LIGHT_ATTENUATION, storage->config.use_physical_light_attenuation);

		bool octahedral_compression = ((RasterizerStorageGLES2::Surface *)e->geometry)->format & RenderingServer::ArrayFormat::ARRAY_FLAG_USE_OCTAHEDRAL_COMPRESSION &&
				(((RasterizerStorageGLES2::Surface *)e->geometry)->blend_shape_data.empty() || ((RasterizerStorageGLES2::Surface *)e->geometry)->blend_shape_buffer_size == 0);
		if (octahedral_compression != prev_octahedral_compression) {
			state.scene_shader.set_conditional(SceneShaderGLES2::ENABLE_OCTAHEDRAL_COMPRESSION, octahedral_compression);
			rebind = true;
		}

		bool shader_rebind = false;
		if (rebind || material != prev_material) {
			storage->info.render.material_switch_count++;
			shader_rebind = _setup_material(material, p_alpha_pass, Size2i(0, 0));
			if (shader_rebind) {
				storage->info.render.shader_rebind_count++;
			}
		}

		_set_cull(e->front_facing, material->shader->spatial.cull_mode == RasterizerStorageGLES2::Shader::Spatial::CULL_MODE_DISABLED, p_reverse_cull);

		if (i == 0 || shader_rebind) { //first time must rebind
			if (use_radiance_map) {
				// would be a bit weird if we don't have this...
				state.scene_shader.set_uniform(SceneShaderGLES2::RADIANCE_INVERSE_XFORM, p_view_transform);
			}

			state.scene_shader.set_uniform(SceneShaderGLES2::BG_ENERGY, 1.0);
			state.scene_shader.set_uniform(SceneShaderGLES2::BG_COLOR, state.default_bg);
			state.scene_shader.set_uniform(SceneShaderGLES2::AMBIENT_SKY_CONTRIBUTION, 1.0);
			state.scene_shader.set_uniform(SceneShaderGLES2::AMBIENT_COLOR, state.default_ambient);
			state.scene_shader.set_uniform(SceneShaderGLES2::AMBIENT_ENERGY, 1.0);

			state.scene_shader.set_uniform(SceneShaderGLES2::CAMERA_MATRIX, p_view_transform);
			state.scene_shader.set_uniform(SceneShaderGLES2::CAMERA_INVERSE_MATRIX, view_transform_inverse);
			state.scene_shader.set_uniform(SceneShaderGLES2::PROJECTION_MATRIX, p_projection);
			state.scene_shader.set_uniform(SceneShaderGLES2::PROJECTION_INVERSE_MATRIX, projection_inverse);

			state.scene_shader.set_uniform(SceneShaderGLES2::TIME, storage->frame.time[0]);
			state.scene_shader.set_uniform(SceneShaderGLES2::VIEW_INDEX, p_eye == 2 ? 1 : 0);

			state.scene_shader.set_uniform(SceneShaderGLES2::VIEWPORT_SIZE, viewport_size);

			state.scene_shader.set_uniform(SceneShaderGLES2::SCREEN_PIXEL_SIZE, screen_pixel_size);
		}

		state.scene_shader.set_uniform(SceneShaderGLES2::WORLD_TRANSFORM, e->instance->transform);

		_render_geometry(e);

		prev_geometry = e->geometry;
		prev_owner = e->owner;
		prev_material = material;
		prev_instancing = instancing;
		prev_octahedral_compression = octahedral_compression;
	}

	state.scene_shader.set_conditional(SceneShaderGLES2::ENABLE_OCTAHEDRAL_COMPRESSION, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_SKELETON, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::SHADELESS, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::BASE_PASS, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_INSTANCING, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_RADIANCE_MAP, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM4, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM3, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM2, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM_BLEND, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_VERTEX_LIGHTING, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_REFLECTION_PROBE1, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_REFLECTION_PROBE2, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::FOG_DEPTH_ENABLED, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::FOG_HEIGHT_ENABLED, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_DEPTH_PREPASS, false);
}

void RasterizerSceneGLES2::_post_process(const Projection &p_cam_projection) {
	//copy to front buffer

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glColorMask(1, 1, 1, 1);

	//no post process on small or render targets without an env
	bool use_post_process = false;

	// If using multisample buffer, resolve to post_process_effect buffer or to front buffer
	if (storage->frame.current_rt && storage->frame.current_rt->multisample_active) {
		GLuint next_buffer;
		if (use_post_process) {
			next_buffer = storage->frame.current_rt->mip_maps[0].sizes[0].fbo;
		} else if (storage->frame.current_rt->external.fbo != 0) {
			next_buffer = storage->frame.current_rt->external.fbo;
		} else {
			// set next_buffer to front buffer so multisample blit can happen if needed
			next_buffer = storage->frame.current_rt->fbo;
		}

#ifdef GLES_OVER_GL
		glBindFramebuffer(GL_READ_FRAMEBUFFER, storage->frame.current_rt->multisample_fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, next_buffer);
		glBlitFramebuffer(0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height, 0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif IPHONE_ENABLED

		glBindFramebuffer(GL_READ_FRAMEBUFFER, storage->frame.current_rt->multisample_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, next_buffer);
		glResolveMultisampleFramebufferAPPLE();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif ANDROID_ENABLED

		// In GLES2 Android Blit is not available, so just copy color texture manually
		_copy_texture_to_buffer(storage->frame.current_rt->multisample_color, next_buffer);
#else
		// TODO: any other platform not supported? this will fail.. maybe we should just call _copy_texture_to_buffer here as well?
		(void)next_buffer; // Silence warning as it's unused.
#endif
	} else if (use_post_process) {
		if (storage->frame.current_rt->external.fbo != 0) {
			_copy_texture_to_buffer(storage->frame.current_rt->external.color, storage->frame.current_rt->mip_maps[0].sizes[0].fbo);
		} else {
			_copy_texture_to_buffer(storage->frame.current_rt->color, storage->frame.current_rt->mip_maps[0].sizes[0].fbo);
		}
	}

	if (!use_post_process) {
		return;
	}

	// Order of operation
	//1) DOF Blur (first blur, then copy to buffer applying the blur) //only on desktop
	//2) FXAA
	//3) Bloom (Glow) //only on desktop
	//4) Adjustments

	//Adjustments

	state.tonemap_shader.set_conditional(TonemapShaderGLES2::DISABLE_ALPHA, !storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]);
	state.tonemap_shader.bind();

	if (storage->frame.current_rt->use_fxaa) {
		state.tonemap_shader.set_uniform(TonemapShaderGLES2::PIXEL_SIZE, Vector2(1.0 / storage->frame.current_rt->width, 1.0 / storage->frame.current_rt->height));
	}

	storage->_copy_screen();

	//turn off everything used
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_FXAA, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL1, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL2, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL3, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL4, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL5, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL6, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_LEVEL7, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_REPLACE, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_SCREEN, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_SOFTLIGHT, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_GLOW_FILTER_BICUBIC, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_MULTI_TEXTURE_GLOW, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_BCS, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::USE_COLOR_CORRECTION, false);
	state.tonemap_shader.set_conditional(TonemapShaderGLES2::DISABLE_ALPHA, false);
}

void RasterizerSceneGLES2::render_scene(const Transform &p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count) {
	Transform cam_transform = p_cam_transform;

	storage->info.render.object_count += p_cull_count;

	GLuint current_fb = 0;

	int viewport_width, viewport_height;
	int viewport_x = 0;
	int viewport_y = 0;
	bool probe_interior = false;
	bool reverse_cull = false;

	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_VFLIP]) {
		cam_transform.basis.set_axis(1, -cam_transform.basis.get_axis(1));
		reverse_cull = true;
	}

	state.render_no_shadows = false;
	if (storage->frame.current_rt->multisample_active) {
		current_fb = storage->frame.current_rt->multisample_fbo;
	} else if (storage->frame.current_rt->external.fbo != 0) {
		current_fb = storage->frame.current_rt->external.fbo;
	} else {
		current_fb = storage->frame.current_rt->fbo;
	}

	viewport_width = storage->frame.current_rt->width;
	viewport_height = storage->frame.current_rt->height;
	viewport_x = storage->frame.current_rt->x;

	if (storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_DIRECT_TO_SCREEN]) {
		viewport_y = OS::get_singleton()->get_window_size().height - viewport_height - storage->frame.current_rt->y;
	} else {
		viewport_y = storage->frame.current_rt->y;
	}

	state.used_screen_texture = false;
	state.viewport_size.x = viewport_width;
	state.viewport_size.y = viewport_height;
	state.screen_pixel_size.x = 1.0 / viewport_width;
	state.screen_pixel_size.y = 1.0 / viewport_height;

	// render list stuff

	render_list.clear();
	_fill_render_list(p_cull_result, p_cull_count, false);

	// other stuff

	glBindFramebuffer(GL_FRAMEBUFFER, current_fb);
	glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_DIRECT_TO_SCREEN]) {
		glScissor(viewport_x, viewport_y, viewport_width, viewport_height);
		glEnable(GL_SCISSOR_TEST);
	}

	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);

	// clear color

	Color clear_color(0, 0, 0, 1);

	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_TRANSPARENT]) {
		clear_color = Color(0, 0, 0, 0);
		storage->frame.clear_request = false;
	} else {
		storage->frame.clear_request = false;
	}

	state.default_ambient = Color(clear_color.r, clear_color.g, clear_color.b, 1.0);
	state.default_bg = Color(clear_color.r, clear_color.g, clear_color.b, 1.0);

	if (storage->frame.current_rt && storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_DIRECT_TO_SCREEN]) {
		glDisable(GL_SCISSOR_TEST);
	}

	glVertexAttrib4f(RS::ARRAY_COLOR, 1, 1, 1, 1);

	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// render sky
	if (probe_interior) {
		state.default_ambient = Color(0, 0, 0, 1); //black as default ambient for interior
		state.default_bg = Color(0, 0, 0, 1); //black as default background for interior
	}

	// make sure we set our output mode correctly
	if (storage->frame.current_rt) {
		state.scene_shader.set_conditional(SceneShaderGLES2::OUTPUT_LINEAR, storage->frame.current_rt->flags[RasterizerStorage::RENDER_TARGET_KEEP_3D_LINEAR]);
	} else {
		state.scene_shader.set_conditional(SceneShaderGLES2::OUTPUT_LINEAR, false);
	}

	// render opaque things first
	render_list.sort_by_key(false);
	_render_render_list(render_list.elements, render_list.element_count, cam_transform, p_cam_projection, p_eye, reverse_cull, false);

	if (storage->frame.current_rt && state.used_screen_texture) {
		//copy screen texture

		if (storage->frame.current_rt->multisample_active) {
			// Resolve framebuffer to front buffer before copying
#ifdef GLES_OVER_GL

			glBindFramebuffer(GL_READ_FRAMEBUFFER, storage->frame.current_rt->multisample_fbo);
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, storage->frame.current_rt->fbo);
			glBlitFramebuffer(0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height, 0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif IPHONE_ENABLED

			glBindFramebuffer(GL_READ_FRAMEBUFFER, storage->frame.current_rt->multisample_fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, storage->frame.current_rt->fbo);
			glResolveMultisampleFramebufferAPPLE();

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#elif ANDROID_ENABLED

			// In GLES2 AndroidBlit is not available, so just copy color texture manually
			_copy_texture_to_buffer(storage->frame.current_rt->multisample_color, storage->frame.current_rt->fbo);
#endif
		}

		storage->canvas->_copy_screen(Rect2());

		if (storage->frame.current_rt && storage->frame.current_rt->multisample_active) {
			// Rebind the current framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, current_fb);
			glViewport(0, 0, viewport_width, viewport_height);
		}
	}
	// alpha pass

	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	render_list.sort_by_reverse_depth_and_priority(true);

	_render_render_list(&render_list.elements[render_list.max_elements - render_list.alpha_element_count], render_list.alpha_element_count, cam_transform, p_cam_projection, p_eye, reverse_cull, true);

	//post process
	_post_process(p_cam_projection);

	// return to default
	state.scene_shader.set_conditional(SceneShaderGLES2::OUTPUT_LINEAR, false);
}

void RasterizerSceneGLES2::set_scene_pass(uint64_t p_pass) {
	scene_pass = p_pass;
}

bool RasterizerSceneGLES2::free(RID p_rid) {
	return false;
}

void RasterizerSceneGLES2::set_debug_draw_mode(RS::ViewportDebugDraw p_debug_draw) {
}

void RasterizerSceneGLES2::initialize() {
	state.scene_shader.init();

	state.scene_shader.set_conditional(SceneShaderGLES2::USE_RGBA_SHADOWS, storage->config.use_rgba_3d_shadows);
	state.cube_to_dp_shader.init();
	state.effect_blur_shader.init();
	state.tonemap_shader.init();

	render_list.init();

	render_pass = 1;

	{
		//default material and shader

		default_shader = RID_PRIME(storage->shader_create());
		storage->shader_set_code(default_shader, "shader_type spatial;\n");
		default_material = RID_PRIME(storage->material_create());
		storage->material_set_shader(default_material, default_shader);

		default_shader_twosided = RID_PRIME(storage->shader_create());
		default_material_twosided = RID_PRIME(storage->material_create());
		storage->shader_set_code(default_shader_twosided, "shader_type spatial; render_mode cull_disabled;\n");
		storage->material_set_shader(default_material_twosided, default_shader_twosided);
	}

	{
		default_worldcoord_shader = RID_PRIME(storage->shader_create());
		storage->shader_set_code(default_worldcoord_shader, "shader_type spatial; render_mode world_vertex_coords;\n");
		default_worldcoord_material = RID_PRIME(storage->material_create());
		storage->material_set_shader(default_worldcoord_material, default_worldcoord_shader);

		default_worldcoord_shader_twosided = RID_PRIME(storage->shader_create());
		default_worldcoord_material_twosided = RID_PRIME(storage->material_create());
		storage->shader_set_code(default_worldcoord_shader_twosided, "shader_type spatial; render_mode cull_disabled,world_vertex_coords;\n");
		storage->material_set_shader(default_worldcoord_material_twosided, default_worldcoord_shader_twosided);
	}

	{
		//default material and shader

		default_overdraw_shader = RID_PRIME(storage->shader_create());
		// Use relatively low opacity so that more "layers" of overlapping objects can be distinguished.
		storage->shader_set_code(default_overdraw_shader, "shader_type spatial;\nrender_mode blend_add,unshaded;\n void fragment() { ALBEDO=vec3(0.4,0.8,0.8); ALPHA=0.1; }");
		default_overdraw_material = RID_PRIME(storage->material_create());
		storage->material_set_shader(default_overdraw_material, default_overdraw_shader);
	}

	{
		glGenBuffers(1, &state.sky_verts);
		glBindBuffer(GL_ARRAY_BUFFER, state.sky_verts);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * 8, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	{
		uint32_t immediate_buffer_size = GLOBAL_DEF("rendering/limits/buffers/immediate_buffer_size_kb", 2048);
		ProjectSettings::get_singleton()->set_custom_property_info("rendering/limits/buffers/immediate_buffer_size_kb", PropertyInfo(Variant::INT, "rendering/limits/buffers/immediate_buffer_size_kb", PROPERTY_HINT_RANGE, "0,8192,1,or_greater"));

		glGenBuffers(1, &state.immediate_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, state.immediate_buffer);
		glBufferData(GL_ARRAY_BUFFER, immediate_buffer_size * 1024, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	glFrontFace(GL_CW);
}

void RasterizerSceneGLES2::iteration() {
}

void RasterizerSceneGLES2::finalize() {
}

RasterizerSceneGLES2::RasterizerSceneGLES2() {
}

RasterizerSceneGLES2::~RasterizerSceneGLES2() {
	storage->free(default_material);
	default_material = RID();
	storage->free(default_material_twosided);
	default_material_twosided = RID();
	storage->free(default_shader);
	default_shader = RID();
	storage->free(default_shader_twosided);
	default_shader_twosided = RID();

	storage->free(default_worldcoord_material);
	default_worldcoord_material = RID();
	storage->free(default_worldcoord_material_twosided);
	default_worldcoord_material_twosided = RID();
	storage->free(default_worldcoord_shader);
	default_worldcoord_shader = RID();
	storage->free(default_worldcoord_shader_twosided);
	default_worldcoord_shader_twosided = RID();

	storage->free(default_overdraw_material);
	default_overdraw_material = RID();
	storage->free(default_overdraw_shader);
	default_overdraw_shader = RID();
}

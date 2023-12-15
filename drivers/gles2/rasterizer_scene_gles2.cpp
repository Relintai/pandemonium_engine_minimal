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

const GLenum RasterizerSceneGLES2::_cube_side_enum[6] = {

	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,

};

void RasterizerSceneGLES2::directional_shadow_create() {
	if (directional_shadow.fbo) {
		// Erase existing directional shadow texture to recreate it.
		glDeleteTextures(1, &directional_shadow.depth);
		glDeleteFramebuffers(1, &directional_shadow.fbo);

		directional_shadow.depth = 0;
		directional_shadow.fbo = 0;
	}

	directional_shadow.light_count = 0;
	directional_shadow.size = next_power_of_2(directional_shadow_size);

	if (directional_shadow.size > storage->config.max_viewport_dimensions[0] || directional_shadow.size > storage->config.max_viewport_dimensions[1]) {
		WARN_PRINT("Cannot set directional shadow size larger than maximum hardware supported size of (" + itos(storage->config.max_viewport_dimensions[0]) + ", " + itos(storage->config.max_viewport_dimensions[1]) + "). Setting size to maximum.");
		directional_shadow.size = MIN(directional_shadow.size, storage->config.max_viewport_dimensions[0]);
		directional_shadow.size = MIN(directional_shadow.size, storage->config.max_viewport_dimensions[1]);
	}

	glGenFramebuffers(1, &directional_shadow.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, directional_shadow.fbo);

	if (storage->config.use_rgba_3d_shadows) {
		//maximum compatibility, renderbuffer and RGBA shadow
		glGenRenderbuffers(1, &directional_shadow.depth);
		glBindRenderbuffer(GL_RENDERBUFFER, directional_shadow.depth);
		glRenderbufferStorage(GL_RENDERBUFFER, storage->config.depth_buffer_internalformat, directional_shadow.size, directional_shadow.size);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, directional_shadow.depth);

		glGenTextures(1, &directional_shadow.color);
		glBindTexture(GL_TEXTURE_2D, directional_shadow.color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, directional_shadow.size, directional_shadow.size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, directional_shadow.color, 0);
	} else {
		//just a depth buffer
		glGenTextures(1, &directional_shadow.depth);
		glBindTexture(GL_TEXTURE_2D, directional_shadow.depth);

		glTexImage2D(GL_TEXTURE_2D, 0, storage->config.depth_internalformat, directional_shadow.size, directional_shadow.size, 0, GL_DEPTH_COMPONENT, storage->config.depth_type, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, directional_shadow.depth, 0);
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		ERR_PRINT("Directional shadow framebuffer status invalid");
	}
}

/* SHADOW ATLAS API */

RID RasterizerSceneGLES2::shadow_atlas_create() {
	ShadowAtlas *shadow_atlas = memnew(ShadowAtlas);
	shadow_atlas->fbo = 0;
	shadow_atlas->depth = 0;
	shadow_atlas->color = 0;
	shadow_atlas->size = 0;
	shadow_atlas->smallest_subdiv = 0;

	for (int i = 0; i < 4; i++) {
		shadow_atlas->size_order[i] = i;
	}

	return shadow_atlas_owner.make_rid(shadow_atlas);
}

void RasterizerSceneGLES2::shadow_atlas_set_size(RID p_atlas, int p_size) {
	ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_atlas);
	ERR_FAIL_COND(!shadow_atlas);
	ERR_FAIL_COND(p_size < 0);

	p_size = next_power_of_2(p_size);

	if (p_size == shadow_atlas->size) {
		return;
	}

	// erase the old atlast
	if (shadow_atlas->fbo) {
		if (storage->config.use_rgba_3d_shadows) {
			glDeleteRenderbuffers(1, &shadow_atlas->depth);
		} else {
			glDeleteTextures(1, &shadow_atlas->depth);
		}
		glDeleteFramebuffers(1, &shadow_atlas->fbo);
		if (shadow_atlas->color) {
			glDeleteTextures(1, &shadow_atlas->color);
		}

		shadow_atlas->fbo = 0;
		shadow_atlas->depth = 0;
		shadow_atlas->color = 0;
	}

	// erase shadow atlast references from lights
	for (RBMap<RID, uint32_t>::Element *E = shadow_atlas->shadow_owners.front(); E; E = E->next()) {
		LightInstance *li = light_instance_owner.getornull(E->key());
		ERR_CONTINUE(!li);
		li->shadow_atlases.erase(p_atlas);
	}

	shadow_atlas->shadow_owners.clear();

	shadow_atlas->size = p_size;

	if (shadow_atlas->size) {
		glGenFramebuffers(1, &shadow_atlas->fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, shadow_atlas->fbo);

		if (shadow_atlas->size > storage->config.max_viewport_dimensions[0] || shadow_atlas->size > storage->config.max_viewport_dimensions[1]) {
			WARN_PRINT("Cannot set shadow atlas size larger than maximum hardware supported size of (" + itos(storage->config.max_viewport_dimensions[0]) + ", " + itos(storage->config.max_viewport_dimensions[1]) + "). Setting size to maximum.");
			shadow_atlas->size = MIN(shadow_atlas->size, storage->config.max_viewport_dimensions[0]);
			shadow_atlas->size = MIN(shadow_atlas->size, storage->config.max_viewport_dimensions[1]);
		}

		// create a depth texture
		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0);

		if (storage->config.use_rgba_3d_shadows) {
			//maximum compatibility, renderbuffer and RGBA shadow
			glGenRenderbuffers(1, &shadow_atlas->depth);
			glBindRenderbuffer(GL_RENDERBUFFER, shadow_atlas->depth);
			glRenderbufferStorage(GL_RENDERBUFFER, storage->config.depth_buffer_internalformat, shadow_atlas->size, shadow_atlas->size);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadow_atlas->depth);

			glGenTextures(1, &shadow_atlas->color);
			glBindTexture(GL_TEXTURE_2D, shadow_atlas->color);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, shadow_atlas->size, shadow_atlas->size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadow_atlas->color, 0);
		} else {
			//just depth texture
			glGenTextures(1, &shadow_atlas->depth);
			glBindTexture(GL_TEXTURE_2D, shadow_atlas->depth);
			glTexImage2D(GL_TEXTURE_2D, 0, storage->config.depth_internalformat, shadow_atlas->size, shadow_atlas->size, 0, GL_DEPTH_COMPONENT, storage->config.depth_type, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_atlas->depth, 0);
		}
		glViewport(0, 0, shadow_atlas->size, shadow_atlas->size);

		glDepthMask(GL_TRUE);

		glClearDepth(0.0f);
		glClear(GL_DEPTH_BUFFER_BIT);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void RasterizerSceneGLES2::shadow_atlas_set_quadrant_subdivision(RID p_atlas, int p_quadrant, int p_subdivision) {
	ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_atlas);
	ERR_FAIL_COND(!shadow_atlas);
	ERR_FAIL_INDEX(p_quadrant, 4);
	ERR_FAIL_INDEX(p_subdivision, 16384);

	uint32_t subdiv = next_power_of_2(p_subdivision);
	if (subdiv & 0xaaaaaaaa) { // sqrt(subdiv) must be integer
		subdiv <<= 1;
	}

	subdiv = int(Math::sqrt((float)subdiv));

	if (shadow_atlas->quadrants[p_quadrant].shadows.size() == (int)subdiv) {
		return;
	}

	// erase all data from the quadrant
	for (int i = 0; i < shadow_atlas->quadrants[p_quadrant].shadows.size(); i++) {
		if (shadow_atlas->quadrants[p_quadrant].shadows[i].owner.is_valid()) {
			shadow_atlas->shadow_owners.erase(shadow_atlas->quadrants[p_quadrant].shadows[i].owner);

			LightInstance *li = light_instance_owner.getornull(shadow_atlas->quadrants[p_quadrant].shadows[i].owner);
			ERR_CONTINUE(!li);
			li->shadow_atlases.erase(p_atlas);
		}
	}

	shadow_atlas->quadrants[p_quadrant].shadows.resize(0);
	shadow_atlas->quadrants[p_quadrant].shadows.resize(subdiv);
	shadow_atlas->quadrants[p_quadrant].subdivision = subdiv;

	// cache the smallest subdivision for faster allocations

	shadow_atlas->smallest_subdiv = 1 << 30;

	for (int i = 0; i < 4; i++) {
		if (shadow_atlas->quadrants[i].subdivision) {
			shadow_atlas->smallest_subdiv = MIN(shadow_atlas->smallest_subdiv, shadow_atlas->quadrants[i].subdivision);
		}
	}

	if (shadow_atlas->smallest_subdiv == 1 << 30) {
		shadow_atlas->smallest_subdiv = 0;
	}

	// re-sort the quadrants

	int swaps = 0;
	do {
		swaps = 0;

		for (int i = 0; i < 3; i++) {
			if (shadow_atlas->quadrants[shadow_atlas->size_order[i]].subdivision < shadow_atlas->quadrants[shadow_atlas->size_order[i + 1]].subdivision) {
				SWAP(shadow_atlas->size_order[i], shadow_atlas->size_order[i + 1]);
				swaps++;
			}
		}

	} while (swaps > 0);
}

bool RasterizerSceneGLES2::_shadow_atlas_find_shadow(ShadowAtlas *shadow_atlas, int *p_in_quadrants, int p_quadrant_count, int p_current_subdiv, uint64_t p_tick, int &r_quadrant, int &r_shadow) {
	for (int i = p_quadrant_count - 1; i >= 0; i--) {
		int qidx = p_in_quadrants[i];

		if (shadow_atlas->quadrants[qidx].subdivision == (uint32_t)p_current_subdiv) {
			return false;
		}

		// look for an empty space

		int sc = shadow_atlas->quadrants[qidx].shadows.size();

		ShadowAtlas::Quadrant::Shadow *sarr = shadow_atlas->quadrants[qidx].shadows.ptrw();

		int found_free_idx = -1; // found a free one
		int found_used_idx = -1; // found an existing one, must steal it
		uint64_t min_pass = 0; // pass of the existing one, try to use the least recently

		for (int j = 0; j < sc; j++) {
			if (!sarr[j].owner.is_valid()) {
				found_free_idx = j;
				break;
			}

			LightInstance *sli = light_instance_owner.getornull(sarr[j].owner);
			ERR_CONTINUE(!sli);

			if (sli->last_scene_pass != scene_pass) {
				// was just allocated, don't kill it so soon, wait a bit...

				if (p_tick - sarr[j].alloc_tick < shadow_atlas_realloc_tolerance_msec) {
					continue;
				}

				if (found_used_idx == -1 || sli->last_scene_pass < min_pass) {
					found_used_idx = j;
					min_pass = sli->last_scene_pass;
				}
			}
		}

		if (found_free_idx == -1 && found_used_idx == -1) {
			continue; // nothing found
		}

		if (found_free_idx == -1 && found_used_idx != -1) {
			found_free_idx = found_used_idx;
		}

		r_quadrant = qidx;
		r_shadow = found_free_idx;

		return true;
	}

	return false;
}

bool RasterizerSceneGLES2::shadow_atlas_update_light(RID p_atlas, RID p_light_intance, float p_coverage, uint64_t p_light_version) {
	ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_atlas);
	ERR_FAIL_COND_V(!shadow_atlas, false);

	LightInstance *li = light_instance_owner.getornull(p_light_intance);
	ERR_FAIL_COND_V(!li, false);

	if (shadow_atlas->size == 0 || shadow_atlas->smallest_subdiv == 0) {
		return false;
	}

	uint32_t quad_size = shadow_atlas->size >> 1;
	int desired_fit = MIN(quad_size / shadow_atlas->smallest_subdiv, next_power_of_2(quad_size * p_coverage));

	int valid_quadrants[4];
	int valid_quadrant_count = 0;
	int best_size = -1;
	int best_subdiv = -1;

	for (int i = 0; i < 4; i++) {
		int q = shadow_atlas->size_order[i];
		int sd = shadow_atlas->quadrants[q].subdivision;

		if (sd == 0) {
			continue;
		}

		int max_fit = quad_size / sd;

		if (best_size != -1 && max_fit > best_size) {
			break; // what we asked for is bigger than this.
		}

		valid_quadrants[valid_quadrant_count] = q;
		valid_quadrant_count++;

		best_subdiv = sd;

		if (max_fit >= desired_fit) {
			best_size = max_fit;
		}
	}

	ERR_FAIL_COND_V(valid_quadrant_count == 0, false); // no suitable block available

	uint64_t tick = OS::get_singleton()->get_ticks_msec();

	if (shadow_atlas->shadow_owners.has(p_light_intance)) {
		// light was already known!

		uint32_t key = shadow_atlas->shadow_owners[p_light_intance];
		uint32_t q = (key >> ShadowAtlas::QUADRANT_SHIFT) & 0x3;
		uint32_t s = key & ShadowAtlas::SHADOW_INDEX_MASK;

		bool should_realloc = shadow_atlas->quadrants[q].subdivision != (uint32_t)best_subdiv && (shadow_atlas->quadrants[q].shadows[s].alloc_tick - tick > shadow_atlas_realloc_tolerance_msec);

		bool should_redraw = shadow_atlas->quadrants[q].shadows[s].version != p_light_version;

		if (!should_realloc) {
			shadow_atlas->quadrants[q].shadows.write[s].version = p_light_version;
			return should_redraw;
		}

		int new_quadrant;
		int new_shadow;

		// find a better place

		if (_shadow_atlas_find_shadow(shadow_atlas, valid_quadrants, valid_quadrant_count, shadow_atlas->quadrants[q].subdivision, tick, new_quadrant, new_shadow)) {
			// found a better place

			ShadowAtlas::Quadrant::Shadow *sh = &shadow_atlas->quadrants[new_quadrant].shadows.write[new_shadow];
			if (sh->owner.is_valid()) {
				// it is take but invalid, so we can take it

				shadow_atlas->shadow_owners.erase(sh->owner);
				LightInstance *sli = light_instance_owner.get(sh->owner);
				sli->shadow_atlases.erase(p_atlas);
			}

			// erase previous
			shadow_atlas->quadrants[q].shadows.write[s].version = 0;
			shadow_atlas->quadrants[q].shadows.write[s].owner = RID();

			sh->owner = p_light_intance;
			sh->alloc_tick = tick;
			sh->version = p_light_version;
			li->shadow_atlases.insert(p_atlas);

			// make a new key
			key = new_quadrant << ShadowAtlas::QUADRANT_SHIFT;
			key |= new_shadow;

			// update it in the map
			shadow_atlas->shadow_owners[p_light_intance] = key;

			// make it dirty, so we redraw
			return true;
		}

		// no better place found, so we keep the current place

		shadow_atlas->quadrants[q].shadows.write[s].version = p_light_version;

		return should_redraw;
	}

	int new_quadrant;
	int new_shadow;

	if (_shadow_atlas_find_shadow(shadow_atlas, valid_quadrants, valid_quadrant_count, -1, tick, new_quadrant, new_shadow)) {
		// found a better place

		ShadowAtlas::Quadrant::Shadow *sh = &shadow_atlas->quadrants[new_quadrant].shadows.write[new_shadow];
		if (sh->owner.is_valid()) {
			// it is take but invalid, so we can take it

			shadow_atlas->shadow_owners.erase(sh->owner);
			LightInstance *sli = light_instance_owner.get(sh->owner);
			sli->shadow_atlases.erase(p_atlas);
		}

		sh->owner = p_light_intance;
		sh->alloc_tick = tick;
		sh->version = p_light_version;
		li->shadow_atlases.insert(p_atlas);

		// make a new key
		uint32_t key = new_quadrant << ShadowAtlas::QUADRANT_SHIFT;
		key |= new_shadow;

		// update it in the map
		shadow_atlas->shadow_owners[p_light_intance] = key;

		// make it dirty, so we redraw
		return true;
	}

	return false;
}

void RasterizerSceneGLES2::set_directional_shadow_count(int p_count) {
	directional_shadow.light_count = p_count;
	directional_shadow.current_light = 0;
}

int RasterizerSceneGLES2::get_directional_light_shadow_size(RID p_light_intance) {
	ERR_FAIL_COND_V(directional_shadow.light_count == 0, 0);

	int shadow_size;

	if (directional_shadow.light_count == 1) {
		shadow_size = directional_shadow.size;
	} else {
		shadow_size = directional_shadow.size / 2; //more than 4 not supported anyway
	}

	LightInstance *light_instance = light_instance_owner.getornull(p_light_intance);
	ERR_FAIL_COND_V(!light_instance, 0);

	switch (light_instance->light_ptr->directional_shadow_mode) {
		case RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL:
			break; //none
		case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS:
		case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_3_SPLITS:
		case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS:
			shadow_size /= 2;
			break;
	}

	return shadow_size;
}
//////////////////////////////////////////////////////

RID RasterizerSceneGLES2::light_instance_create(RID p_light) {
	LightInstance *light_instance = memnew(LightInstance);

	light_instance->last_scene_pass = 0;

	light_instance->light = p_light;
	light_instance->light_ptr = storage->light_owner.getornull(p_light);

	light_instance->light_index = 0xFFFF;

	// an ever increasing counter for each light added,
	// used for sorting lights for a consistent render
	light_instance->light_counter = _light_counter++;

	if (!light_instance->light_ptr) {
		memdelete(light_instance);
		ERR_FAIL_V_MSG(RID(), "Condition ' !light_instance->light_ptr ' is true.");
	}

	light_instance->self = light_instance_owner.make_rid(light_instance);

	return light_instance->self;
}

void RasterizerSceneGLES2::light_instance_set_transform(RID p_light_instance, const Transform &p_transform) {
	LightInstance *light_instance = light_instance_owner.getornull(p_light_instance);
	ERR_FAIL_COND(!light_instance);

	light_instance->transform = p_transform;
}

void RasterizerSceneGLES2::light_instance_set_shadow_transform(RID p_light_instance, const Projection &p_projection, const Transform &p_transform, float p_far, float p_split, int p_pass, float p_bias_scale) {
	LightInstance *light_instance = light_instance_owner.getornull(p_light_instance);
	ERR_FAIL_COND(!light_instance);

	if (light_instance->light_ptr->type != RS::LIGHT_DIRECTIONAL) {
		p_pass = 0;
	}

	ERR_FAIL_INDEX(p_pass, 4);

	light_instance->shadow_transform[p_pass].camera = p_projection;
	light_instance->shadow_transform[p_pass].transform = p_transform;
	light_instance->shadow_transform[p_pass].farplane = p_far;
	light_instance->shadow_transform[p_pass].split = p_split;
	light_instance->shadow_transform[p_pass].bias_scale = p_bias_scale;
}

void RasterizerSceneGLES2::light_instance_mark_visible(RID p_light_instance) {
	LightInstance *light_instance = light_instance_owner.getornull(p_light_instance);
	ERR_FAIL_COND(!light_instance);

	light_instance->last_scene_pass = scene_pass;
}

////////////////////////////
////////////////////////////
////////////////////////////

void RasterizerSceneGLES2::_add_geometry(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, int p_material, bool p_depth_pass, bool p_shadow_pass) {
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

	_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass, p_shadow_pass);

	while (material->next_pass.is_valid()) {
		material = storage->material_owner.getornull(material->next_pass);

		if (!material || !material->shader || !material->shader->valid) {
			break;
		}

		_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass, p_shadow_pass);
	}

	// Repeat the "nested chain" logic also for the overlay
	if (p_instance->material_overlay.is_valid()) {
		material = storage->material_owner.getornull(p_instance->material_overlay);

		if (!material || !material->shader || !material->shader->valid) {
			return;
		}

		_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass, p_shadow_pass);

		while (material->next_pass.is_valid()) {
			material = storage->material_owner.getornull(material->next_pass);

			if (!material || !material->shader || !material->shader->valid) {
				break;
			}

			_add_geometry_with_material(p_geometry, p_instance, p_owner, material, p_depth_pass, p_shadow_pass);
		}
	}
}
void RasterizerSceneGLES2::_add_geometry_with_material(RasterizerStorageGLES2::Geometry *p_geometry, InstanceBase *p_instance, RasterizerStorageGLES2::GeometryOwner *p_owner, RasterizerStorageGLES2::Material *p_material, bool p_depth_pass, bool p_shadow_pass) {
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
			if (p_instance->cast_shadows == RS::SHADOW_CASTING_SETTING_DOUBLE_SIDED) {
				p_material = storage->material_owner.getptr(!p_shadow_pass && p_material->shader->spatial.uses_world_coordinates ? default_worldcoord_material_twosided : default_material_twosided);
				mirror = false;
			} else {
				p_material = storage->material_owner.getptr(!p_shadow_pass && p_material->shader->spatial.uses_world_coordinates ? default_worldcoord_material : default_material);
			}
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

	e->refprobe_0_index = RenderList::MAX_REFLECTION_PROBES; //refprobe disabled by default
	e->refprobe_1_index = RenderList::MAX_REFLECTION_PROBES; //refprobe disabled by default

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

		if (p_material->shader->spatial.unshaded) {
			e->light_mode = LIGHTMODE_UNSHADED;
		} else {
			bool copy = false;

			for (int i = 0; i < render_directional_lights; i++) {
				if (copy) {
					RenderList::Element *e2 = has_alpha ? render_list.add_alpha_element() : render_list.add_element();
					if (!e2) {
						break;
					}
					*e2 = *e; //this includes accum ptr :)
					e = e2;
				}

				//directional sort key
				e->light_type1 = 0;
				e->light_type2 = 1;
				e->light_index = i;

				copy = true;
			}

			//add omni / spots

			for (int i = 0; i < e->instance->light_instances.size(); i++) {
				LightInstance *li = light_instance_owner.getornull(e->instance->light_instances[i]);

				if (!li || li->light_index >= render_light_instance_count || render_light_instances[li->light_index] != li) {
					continue; // too many or light_index did not correspond to the light instances to be rendered
				}

				if (copy) {
					RenderList::Element *e2 = has_alpha ? render_list.add_alpha_element() : render_list.add_element();
					if (!e2) {
						break;
					}
					*e2 = *e; //this includes accum ptr :)
					e = e2;
				}

				//directional sort key
				e->light_type1 = 1;
				e->light_type2 = li->light_ptr->type == RenderingServer::LIGHT_OMNI ? 0 : 1;
				e->light_index = li->light_index;

				copy = true;
			}

			e->light_mode = LIGHTMODE_NORMAL;
		}
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

void RasterizerSceneGLES2::_fill_render_list(InstanceBase **p_cull_result, int p_cull_count, bool p_depth_pass, bool p_shadow_pass) {
	render_pass++;
	current_material_index = 0;
	current_geometry_index = 0;
	current_light_index = 0;
	current_refprobe_index = 0;
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

					_add_geometry(surface, instance, nullptr, material_index, p_depth_pass, p_shadow_pass);
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
					_add_geometry(s, instance, multi_mesh, -1, p_depth_pass, p_shadow_pass);
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

void RasterizerSceneGLES2::_setup_light_type(LightInstance *p_light, ShadowAtlas *shadow_atlas) {
	//turn off all by default
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_LIGHTING, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_SHADOW, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_5, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_13, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_MODE_DIRECTIONAL, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_MODE_OMNI, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_MODE_SPOT, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM2, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM4, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM_BLEND, false);

	if (!p_light) { //no light, return off
		return;
	}

	//turn on lighting
	state.scene_shader.set_conditional(SceneShaderGLES2::USE_LIGHTING, true);

	switch (p_light->light_ptr->type) {
		case RS::LIGHT_DIRECTIONAL: {
			state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_MODE_DIRECTIONAL, true);
			switch (p_light->light_ptr->directional_shadow_mode) {
				case RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL: {
					//no need
				} break;
				case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS: {
					state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM2, true);
				} break;
				case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_3_SPLITS: {
					state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM3, true);
				} break;
				case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS: {
					state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM4, true);
				} break;
			}

			state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_USE_PSSM_BLEND, p_light->light_ptr->directional_blend_splits);
			if (!state.render_no_shadows && p_light->light_ptr->shadow) {
				state.scene_shader.set_conditional(SceneShaderGLES2::USE_SHADOW, true);
				WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
				if (storage->config.use_rgba_3d_shadows) {
					glBindTexture(GL_TEXTURE_2D, directional_shadow.color);
				} else {
					glBindTexture(GL_TEXTURE_2D, directional_shadow.depth);
				}
				state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_5, shadow_filter_mode == SHADOW_FILTER_PCF5);
				state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_13, shadow_filter_mode == SHADOW_FILTER_PCF13);
			}

		} break;
		case RS::LIGHT_OMNI: {
			state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_MODE_OMNI, true);
			if (!state.render_no_shadows && shadow_atlas && p_light->light_ptr->shadow) {
				state.scene_shader.set_conditional(SceneShaderGLES2::USE_SHADOW, true);
				WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
				if (storage->config.use_rgba_3d_shadows) {
					glBindTexture(GL_TEXTURE_2D, shadow_atlas->color);
				} else {
					glBindTexture(GL_TEXTURE_2D, shadow_atlas->depth);
				}
				state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_5, shadow_filter_mode == SHADOW_FILTER_PCF5);
				state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_13, shadow_filter_mode == SHADOW_FILTER_PCF13);
			}
		} break;
		case RS::LIGHT_SPOT: {
			state.scene_shader.set_conditional(SceneShaderGLES2::LIGHT_MODE_SPOT, true);
			if (!state.render_no_shadows && shadow_atlas && p_light->light_ptr->shadow) {
				state.scene_shader.set_conditional(SceneShaderGLES2::USE_SHADOW, true);
				WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0 + storage->config.max_texture_image_units - 3);
				if (storage->config.use_rgba_3d_shadows) {
					glBindTexture(GL_TEXTURE_2D, shadow_atlas->color);
				} else {
					glBindTexture(GL_TEXTURE_2D, shadow_atlas->depth);
				}
				state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_5, shadow_filter_mode == SHADOW_FILTER_PCF5);
				state.scene_shader.set_conditional(SceneShaderGLES2::SHADOW_MODE_PCF_13, shadow_filter_mode == SHADOW_FILTER_PCF13);
			}
		} break;
	}
}

void RasterizerSceneGLES2::_setup_light(LightInstance *light, ShadowAtlas *shadow_atlas, const Transform &p_view_transform, bool accum_pass) {
	RasterizerStorageGLES2::Light *light_ptr = light->light_ptr;

	//common parameters
	float energy = light_ptr->param[RS::LIGHT_PARAM_ENERGY];
	float specular = light_ptr->param[RS::LIGHT_PARAM_SPECULAR];
	float sign = (light_ptr->negative && !accum_pass) ? -1 : 1; //inverse color for base pass lights only

	state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SPECULAR, specular);
	Color color = light_ptr->color * sign * energy * Math_PI;
	state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_COLOR, color);

	state.scene_shader.set_uniform(SceneShaderGLES2::SHADOW_COLOR, light_ptr->shadow_color);

	//specific parameters

	switch (light_ptr->type) {
		case RS::LIGHT_DIRECTIONAL: {
			//not using inverse for performance, view should be normalized anyway
			Vector3 direction = p_view_transform.basis.xform_inv(light->transform.basis.xform(Vector3(0, 0, -1))).normalized();
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_DIRECTION, direction);

			Projection matrices[4];

			if (!state.render_no_shadows && light_ptr->shadow && directional_shadow.depth) {
				int shadow_count = 0;
				Color split_offsets;

				switch (light_ptr->directional_shadow_mode) {
					case RS::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL: {
						shadow_count = 1;
					} break;

					case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS: {
						shadow_count = 2;
					} break;

					case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_3_SPLITS: {
						shadow_count = 3;
					} break;

					case RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS: {
						shadow_count = 4;
					} break;
				}

				for (int k = 0; k < shadow_count; k++) {
					uint32_t x = light->directional_rect.position.x;
					uint32_t y = light->directional_rect.position.y;
					uint32_t width = light->directional_rect.size.x;
					uint32_t height = light->directional_rect.size.y;

					if (light_ptr->directional_shadow_mode == RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_3_SPLITS ||
							light_ptr->directional_shadow_mode == RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS) {
						width /= 2;
						height /= 2;

						if (k == 1) {
							x += width;
						} else if (k == 2) {
							y += height;
						} else if (k == 3) {
							x += width;
							y += height;
						}

					} else if (light_ptr->directional_shadow_mode == RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS) {
						height /= 2;

						if (k != 0) {
							y += height;
						}
					}

					split_offsets[k] = light->shadow_transform[k].split;

					Transform modelview = (p_view_transform.inverse() * light->shadow_transform[k].transform).affine_inverse();

					Projection bias;
					bias.set_light_bias();
					Projection rectm;
					Rect2 atlas_rect = Rect2(float(x) / directional_shadow.size, float(y) / directional_shadow.size, float(width) / directional_shadow.size, float(height) / directional_shadow.size);
					rectm.set_light_atlas_rect(atlas_rect);

					Projection shadow_mtx = rectm * bias * light->shadow_transform[k].camera * modelview;
					matrices[k] = shadow_mtx;

					/*Color light_clamp;
					light_clamp[0] = atlas_rect.position.x;
					light_clamp[1] = atlas_rect.position.y;
					light_clamp[2] = atlas_rect.size.x;
					light_clamp[3] = atlas_rect.size.y;*/
				}

				//	state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_CLAMP, light_clamp);
				state.scene_shader.set_uniform(SceneShaderGLES2::SHADOW_PIXEL_SIZE, Size2(1.0 / directional_shadow.size, 1.0 / directional_shadow.size));
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SPLIT_OFFSETS, split_offsets);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SHADOW_MATRIX, matrices[0]);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SHADOW_MATRIX2, matrices[1]);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SHADOW_MATRIX3, matrices[2]);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SHADOW_MATRIX4, matrices[3]);
			}
		} break;
		case RS::LIGHT_OMNI: {
			Vector3 position = p_view_transform.xform_inv(light->transform.origin);

			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_POSITION, position);

			float range = light_ptr->param[RS::LIGHT_PARAM_RANGE];
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_RANGE, range);

			float attenuation = light_ptr->param[RS::LIGHT_PARAM_ATTENUATION];
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_ATTENUATION, attenuation);

			if (!state.render_no_shadows && light_ptr->shadow && shadow_atlas && shadow_atlas->shadow_owners.has(light->self)) {
				uint32_t key = shadow_atlas->shadow_owners[light->self];

				uint32_t quadrant = (key >> ShadowAtlas::QUADRANT_SHIFT) & 0x03;
				uint32_t shadow = key & ShadowAtlas::SHADOW_INDEX_MASK;

				ERR_BREAK(shadow >= (uint32_t)shadow_atlas->quadrants[quadrant].shadows.size());

				uint32_t atlas_size = shadow_atlas->size;
				uint32_t quadrant_size = atlas_size >> 1;

				uint32_t x = (quadrant & 1) * quadrant_size;
				uint32_t y = (quadrant >> 1) * quadrant_size;

				uint32_t shadow_size = (quadrant_size / shadow_atlas->quadrants[quadrant].subdivision);
				x += (shadow % shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;
				y += (shadow / shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;

				uint32_t width = shadow_size;
				uint32_t height = shadow_size;

				if (light->light_ptr->omni_shadow_detail == RS::LIGHT_OMNI_SHADOW_DETAIL_HORIZONTAL) {
					height /= 2;
				} else {
					width /= 2;
				}

				Transform proj = (p_view_transform.inverse() * light->transform).inverse();

				Color light_clamp;
				light_clamp[0] = float(x) / atlas_size;
				light_clamp[1] = float(y) / atlas_size;
				light_clamp[2] = float(width) / atlas_size;
				light_clamp[3] = float(height) / atlas_size;

				state.scene_shader.set_uniform(SceneShaderGLES2::SHADOW_PIXEL_SIZE, Size2(1.0 / shadow_atlas->size, 1.0 / shadow_atlas->size));
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SHADOW_MATRIX, proj);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_CLAMP, light_clamp);
			}
		} break;

		case RS::LIGHT_SPOT: {
			Vector3 position = p_view_transform.xform_inv(light->transform.origin);

			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_POSITION, position);

			Vector3 direction = p_view_transform.inverse().basis.xform(light->transform.basis.xform(Vector3(0, 0, -1))).normalized();
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_DIRECTION, direction);
			float attenuation = light_ptr->param[RS::LIGHT_PARAM_ATTENUATION];
			float range = light_ptr->param[RS::LIGHT_PARAM_RANGE];
			float spot_attenuation = light_ptr->param[RS::LIGHT_PARAM_SPOT_ATTENUATION];
			float angle = light_ptr->param[RS::LIGHT_PARAM_SPOT_ANGLE];
			angle = Math::cos(Math::deg2rad(angle));
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_ATTENUATION, attenuation);
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SPOT_ATTENUATION, spot_attenuation);
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SPOT_RANGE, spot_attenuation);
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SPOT_ANGLE, angle);
			state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_RANGE, range);

			if (!state.render_no_shadows && light->light_ptr->shadow && shadow_atlas && shadow_atlas->shadow_owners.has(light->self)) {
				uint32_t key = shadow_atlas->shadow_owners[light->self];

				uint32_t quadrant = (key >> ShadowAtlas::QUADRANT_SHIFT) & 0x03;
				uint32_t shadow = key & ShadowAtlas::SHADOW_INDEX_MASK;

				ERR_BREAK(shadow >= (uint32_t)shadow_atlas->quadrants[quadrant].shadows.size());

				uint32_t atlas_size = shadow_atlas->size;
				uint32_t quadrant_size = atlas_size >> 1;

				uint32_t x = (quadrant & 1) * quadrant_size;
				uint32_t y = (quadrant >> 1) * quadrant_size;

				uint32_t shadow_size = (quadrant_size / shadow_atlas->quadrants[quadrant].subdivision);
				x += (shadow % shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;
				y += (shadow / shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;

				uint32_t width = shadow_size;
				uint32_t height = shadow_size;

				Rect2 rect(float(x) / atlas_size, float(y) / atlas_size, float(width) / atlas_size, float(height) / atlas_size);

				Color light_clamp;
				light_clamp[0] = rect.position.x;
				light_clamp[1] = rect.position.y;
				light_clamp[2] = rect.size.x;
				light_clamp[3] = rect.size.y;

				Transform modelview = (p_view_transform.inverse() * light->transform).inverse();

				Projection bias;
				bias.set_light_bias();

				Projection rectm;
				rectm.set_light_atlas_rect(rect);

				Projection shadow_matrix = rectm * bias * light->shadow_transform[0].camera * modelview;

				state.scene_shader.set_uniform(SceneShaderGLES2::SHADOW_PIXEL_SIZE, Size2(1.0 / shadow_atlas->size, 1.0 / shadow_atlas->size));
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_SHADOW_MATRIX, shadow_matrix);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_CLAMP, light_clamp);
			}

		} break;
		default: {
		}
	}
}

void RasterizerSceneGLES2::_render_render_list(RenderList::Element **p_elements, int p_element_count, const Transform &p_view_transform, const Projection &p_projection, const int p_eye, RID p_shadow_atlas, float p_shadow_bias, float p_shadow_normal_bias, bool p_reverse_cull, bool p_alpha_pass, bool p_shadow) {
	ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_shadow_atlas);

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
	LightInstance *prev_light = nullptr;
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
		LightInstance *light = nullptr;
		bool rebind_light = false;

		if (!p_shadow && material->shader) {
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

			if (!unshaded && e->light_index < RenderList::MAX_LIGHTS) {
				light = render_light_instances[e->light_index];
				if ((e->instance->layer_mask & light->light_ptr->cull_mask) == 0) {
					light = nullptr; // Don't use this light, it is culled
				}
			}

			if (light != prev_light) {
				_setup_light_type(light, shadow_atlas);
				rebind = true;
				rebind_light = true;
			}

			int blend_mode = p_alpha_pass ? material->shader->spatial.blend_mode : -1; // -1 no blend, no mix

			if (accum_pass) { //accum pass force pass
				blend_mode = RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_ADD;
				if (light && light->light_ptr->negative) {
					blend_mode = RasterizerStorageGLES2::Shader::Spatial::BLEND_MODE_SUB;
				}
			}

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
			bool vertex_lit = (material->shader->spatial.uses_vertex_lighting || storage->config.force_vertex_shading) && ((!unshaded && light) || using_fog); //fog forces vertex lighting because it still applies even if unshaded or no fog

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

			if (p_shadow) {
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_BIAS, p_shadow_bias);
				state.scene_shader.set_uniform(SceneShaderGLES2::LIGHT_NORMAL_BIAS, p_shadow_normal_bias);
				if (state.shadow_is_dual_parabolloid) {
					state.scene_shader.set_uniform(SceneShaderGLES2::SHADOW_DUAL_PARABOLOID_RENDER_SIDE, state.dual_parbolloid_direction);
					state.scene_shader.set_uniform(SceneShaderGLES2::SHADOW_DUAL_PARABOLOID_RENDER_ZFAR, state.dual_parbolloid_zfar);
				}
			} else {
				if (use_radiance_map) {
					// would be a bit weird if we don't have this...
					state.scene_shader.set_uniform(SceneShaderGLES2::RADIANCE_INVERSE_XFORM, p_view_transform);
				}

				state.scene_shader.set_uniform(SceneShaderGLES2::BG_ENERGY, 1.0);
				state.scene_shader.set_uniform(SceneShaderGLES2::BG_COLOR, state.default_bg);
				state.scene_shader.set_uniform(SceneShaderGLES2::AMBIENT_SKY_CONTRIBUTION, 1.0);
				state.scene_shader.set_uniform(SceneShaderGLES2::AMBIENT_COLOR, state.default_ambient);
				state.scene_shader.set_uniform(SceneShaderGLES2::AMBIENT_ENERGY, 1.0);

				//rebind all these
				rebind_light = true;
			}

			state.scene_shader.set_uniform(SceneShaderGLES2::CAMERA_MATRIX, p_view_transform);
			state.scene_shader.set_uniform(SceneShaderGLES2::CAMERA_INVERSE_MATRIX, view_transform_inverse);
			state.scene_shader.set_uniform(SceneShaderGLES2::PROJECTION_MATRIX, p_projection);
			state.scene_shader.set_uniform(SceneShaderGLES2::PROJECTION_INVERSE_MATRIX, projection_inverse);

			state.scene_shader.set_uniform(SceneShaderGLES2::TIME, storage->frame.time[0]);
			state.scene_shader.set_uniform(SceneShaderGLES2::VIEW_INDEX, p_eye == 2 ? 1 : 0);

			state.scene_shader.set_uniform(SceneShaderGLES2::VIEWPORT_SIZE, viewport_size);

			state.scene_shader.set_uniform(SceneShaderGLES2::SCREEN_PIXEL_SIZE, screen_pixel_size);
		}

		if (rebind_light && light) {
			_setup_light(light, shadow_atlas, p_view_transform, accum_pass);
		}

		state.scene_shader.set_uniform(SceneShaderGLES2::WORLD_TRANSFORM, e->instance->transform);

		_render_geometry(e);

		prev_geometry = e->geometry;
		prev_owner = e->owner;
		prev_material = material;
		prev_instancing = instancing;
		prev_octahedral_compression = octahedral_compression;
		prev_light = light;
	}

	_setup_light_type(nullptr, nullptr); //clear light stuff
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

void RasterizerSceneGLES2::render_scene(const Transform &p_cam_transform, const Projection &p_cam_projection, const int p_eye, bool p_cam_ortogonal, InstanceBase **p_cull_result, int p_cull_count, RID *p_light_cull_result, int p_light_cull_count, RID p_shadow_atlas) {
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

	//push back the directional lights

	if (p_light_cull_count) {
		//hardcoded limit of 256 lights
		render_light_instance_count = MIN(RenderList::MAX_LIGHTS, p_light_cull_count);
		render_light_instances = (LightInstance **)alloca(sizeof(LightInstance *) * render_light_instance_count);
		render_directional_lights = 0;

		//doing this because directional lights are at the end, put them at the beginning
		int index = 0;
		for (int i = render_light_instance_count - 1; i >= 0; i--) {
			RID light_rid = p_light_cull_result[i];

			LightInstance *light = light_instance_owner.getornull(light_rid);

			if (light->light_ptr->type == RS::LIGHT_DIRECTIONAL) {
				render_directional_lights++;
				//as going in reverse, directional lights are always first anyway
			}

			light->light_index = index;
			render_light_instances[index] = light;

			index++;
		}

	} else {
		render_light_instances = nullptr;
		render_directional_lights = 0;
		render_light_instance_count = 0;
	}

	// render list stuff

	render_list.clear();
	_fill_render_list(p_cull_result, p_cull_count, false, false);

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
	_render_render_list(render_list.elements, render_list.element_count, cam_transform, p_cam_projection, p_eye, p_shadow_atlas, 0.0, 0.0, reverse_cull, false, false);

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

	_render_render_list(&render_list.elements[render_list.max_elements - render_list.alpha_element_count], render_list.alpha_element_count, cam_transform, p_cam_projection, p_eye, p_shadow_atlas, 0.0, 0.0, reverse_cull, true, false);

	//post process
	_post_process(p_cam_projection);

	//#define GLES2_SHADOW_ATLAS_DEBUG_VIEW

#ifdef GLES2_SHADOW_ATLAS_DEBUG_VIEW
	ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_shadow_atlas);
	if (shadow_atlas) {
		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadow_atlas->depth);

		glViewport(0, 0, storage->frame.current_rt->width / 4, storage->frame.current_rt->height / 4);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_CUBEMAP, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_COPY_SECTION, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_CUSTOM_ALPHA, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_MULTIPLIER, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_PANORAMA, false);
		storage->shaders.copy.bind();

		storage->_copy_screen();
	}
#endif

	//#define GLES2_SHADOW_DIRECTIONAL_DEBUG_VIEW

#ifdef GLES2_SHADOW_DIRECTIONAL_DEBUG_VIEW
	if (true) {
		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, directional_shadow.depth);

		glViewport(0, 0, storage->frame.current_rt->width / 4, storage->frame.current_rt->height / 4);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_CUBEMAP, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_COPY_SECTION, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_CUSTOM_ALPHA, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_MULTIPLIER, false);
		storage->shaders.copy.set_conditional(CopyShaderGLES2::USE_PANORAMA, false);
		storage->shaders.copy.bind();

		storage->_copy_screen();
	}
#endif

	// return to default
	state.scene_shader.set_conditional(SceneShaderGLES2::OUTPUT_LINEAR, false);
}

void RasterizerSceneGLES2::render_shadow(RID p_light, RID p_shadow_atlas, int p_pass, InstanceBase **p_cull_result, int p_cull_count) {
	state.render_no_shadows = false;

	LightInstance *light_instance = light_instance_owner.getornull(p_light);
	ERR_FAIL_COND(!light_instance);

	RasterizerStorageGLES2::Light *light = light_instance->light_ptr;
	ERR_FAIL_COND(!light);

	uint32_t x;
	uint32_t y;
	uint32_t width;
	uint32_t height;

	float zfar = 0;
	bool flip_facing = false;
	int custom_vp_size = 0;
	GLuint fbo = 0;
	state.shadow_is_dual_parabolloid = false;
	state.dual_parbolloid_direction = 0.0;

	int current_cubemap = -1;
	float bias = 0;
	float normal_bias = 0;

	Projection light_projection;
	Transform light_transform;

	// TODO directional light

	if (light->type == RS::LIGHT_DIRECTIONAL) {
		// set pssm stuff

		// TODO set this only when changed

		light_instance->light_directional_index = directional_shadow.current_light;
		light_instance->last_scene_shadow_pass = scene_pass;

		directional_shadow.current_light++;

		if (directional_shadow.light_count == 1) {
			light_instance->directional_rect = Rect2(0, 0, directional_shadow.size, directional_shadow.size);
		} else if (directional_shadow.light_count == 2) {
			light_instance->directional_rect = Rect2(0, 0, directional_shadow.size, directional_shadow.size / 2);
			if (light_instance->light_directional_index == 1) {
				light_instance->directional_rect.position.y += light_instance->directional_rect.size.y;
			}
		} else { //3 and 4
			light_instance->directional_rect = Rect2(0, 0, directional_shadow.size / 2, directional_shadow.size / 2);
			if (light_instance->light_directional_index & 1) {
				light_instance->directional_rect.position.x += light_instance->directional_rect.size.x;
			}
			if (light_instance->light_directional_index / 2) {
				light_instance->directional_rect.position.y += light_instance->directional_rect.size.y;
			}
		}

		light_projection = light_instance->shadow_transform[p_pass].camera;
		light_transform = light_instance->shadow_transform[p_pass].transform;

		x = light_instance->directional_rect.position.x;
		y = light_instance->directional_rect.position.y;
		width = light_instance->directional_rect.size.width;
		height = light_instance->directional_rect.size.height;

		if (light->directional_shadow_mode == RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_3_SPLITS || light->directional_shadow_mode == RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS) {
			width /= 2;
			height /= 2;

			if (p_pass == 1) {
				x += width;
			} else if (p_pass == 2) {
				y += height;
			} else if (p_pass == 3) {
				x += width;
				y += height;
			}

		} else if (light->directional_shadow_mode == RS::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS) {
			height /= 2;

			if (p_pass == 0) {
			} else {
				y += height;
			}
		}

		float bias_mult = Math::lerp(1.0f, light_instance->shadow_transform[p_pass].bias_scale, light->param[RS::LIGHT_PARAM_SHADOW_BIAS_SPLIT_SCALE]);
		zfar = light->param[RS::LIGHT_PARAM_RANGE];
		bias = light->param[RS::LIGHT_PARAM_SHADOW_BIAS] * bias_mult;
		normal_bias = light->param[RS::LIGHT_PARAM_SHADOW_NORMAL_BIAS] * bias_mult;

		fbo = directional_shadow.fbo;
	} else {
		ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_shadow_atlas);
		ERR_FAIL_COND(!shadow_atlas);
		ERR_FAIL_COND(!shadow_atlas->shadow_owners.has(p_light));

		fbo = shadow_atlas->fbo;

		uint32_t key = shadow_atlas->shadow_owners[p_light];

		uint32_t quadrant = (key >> ShadowAtlas::QUADRANT_SHIFT) & 0x03;
		uint32_t shadow = key & ShadowAtlas::SHADOW_INDEX_MASK;

		ERR_FAIL_INDEX((int)shadow, shadow_atlas->quadrants[quadrant].shadows.size());

		uint32_t quadrant_size = shadow_atlas->size >> 1;

		x = (quadrant & 1) * quadrant_size;
		y = (quadrant >> 1) * quadrant_size;

		uint32_t shadow_size = (quadrant_size / shadow_atlas->quadrants[quadrant].subdivision);
		x += (shadow % shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;
		y += (shadow / shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;

		width = shadow_size;
		height = shadow_size;

		if (light->type == RS::LIGHT_OMNI) {
			// cubemap only
			if (light->omni_shadow_mode == RS::LIGHT_OMNI_SHADOW_CUBE && storage->config.support_shadow_cubemaps) {
				int cubemap_index = shadow_cubemaps.size() - 1;

				// find an appropriate cubemap to render to
				for (int i = shadow_cubemaps.size() - 1; i >= 0; i--) {
					if (shadow_cubemaps[i].size > shadow_size) {
						break;
					}

					cubemap_index = i;
				}

				fbo = shadow_cubemaps[cubemap_index].fbo[p_pass];
				light_projection = light_instance->shadow_transform[0].camera;
				light_transform = light_instance->shadow_transform[0].transform;

				custom_vp_size = shadow_cubemaps[cubemap_index].size;
				zfar = light->param[RS::LIGHT_PARAM_RANGE];

				current_cubemap = cubemap_index;
			} else {
				//dual parabolloid
				state.shadow_is_dual_parabolloid = true;
				light_projection = light_instance->shadow_transform[0].camera;
				light_transform = light_instance->shadow_transform[0].transform;

				if (light->omni_shadow_detail == RS::LIGHT_OMNI_SHADOW_DETAIL_HORIZONTAL) {
					height /= 2;
					y += p_pass * height;
				} else {
					width /= 2;
					x += p_pass * width;
				}

				state.dual_parbolloid_direction = p_pass == 0 ? 1.0 : -1.0;
				flip_facing = (p_pass == 1);
				zfar = light->param[RS::LIGHT_PARAM_RANGE];
				bias = light->param[RS::LIGHT_PARAM_SHADOW_BIAS];

				state.dual_parbolloid_zfar = zfar;

				state.scene_shader.set_conditional(SceneShaderGLES2::RENDER_DEPTH_DUAL_PARABOLOID, true);
			}

		} else if (light->type == RS::LIGHT_SPOT) {
			light_projection = light_instance->shadow_transform[0].camera;
			light_transform = light_instance->shadow_transform[0].transform;

			flip_facing = false;
			zfar = light->param[RS::LIGHT_PARAM_RANGE];
			bias = light->param[RS::LIGHT_PARAM_SHADOW_BIAS];
			normal_bias = light->param[RS::LIGHT_PARAM_SHADOW_NORMAL_BIAS];
		}
	}

	render_list.clear();

	_fill_render_list(p_cull_result, p_cull_count, true, true);

	render_list.sort_by_depth(false);

	glDisable(GL_BLEND);
	glDisable(GL_DITHER);
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glDepthMask(GL_TRUE);
	if (!storage->config.use_rgba_3d_shadows) {
		glColorMask(0, 0, 0, 0);
	}

	if (custom_vp_size) {
		glViewport(0, 0, custom_vp_size, custom_vp_size);
		glScissor(0, 0, custom_vp_size, custom_vp_size);
	} else {
		glViewport(x, y, width, height);
		glScissor(x, y, width, height);
	}

	glEnable(GL_SCISSOR_TEST);
	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	if (storage->config.use_rgba_3d_shadows) {
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glDisable(GL_SCISSOR_TEST);

	if (light->reverse_cull) {
		flip_facing = !flip_facing;
	}

	state.scene_shader.set_conditional(SceneShaderGLES2::RENDER_DEPTH, true);
	state.scene_shader.set_conditional(SceneShaderGLES2::OUTPUT_LINEAR, false); // just in case, should be false already

	_render_render_list(render_list.elements, render_list.element_count, light_transform, light_projection, 0, RID(), bias, normal_bias, flip_facing, false, true);

	state.scene_shader.set_conditional(SceneShaderGLES2::RENDER_DEPTH, false);
	state.scene_shader.set_conditional(SceneShaderGLES2::RENDER_DEPTH_DUAL_PARABOLOID, false);

	// convert cubemap to dual paraboloid if needed
	if (light->type == RS::LIGHT_OMNI && (light->omni_shadow_mode == RS::LIGHT_OMNI_SHADOW_CUBE && storage->config.support_shadow_cubemaps) && p_pass == 5) {
		ShadowAtlas *shadow_atlas = shadow_atlas_owner.getornull(p_shadow_atlas);

		glBindFramebuffer(GL_FRAMEBUFFER, shadow_atlas->fbo);
		state.cube_to_dp_shader.bind();

		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, shadow_cubemaps[current_cubemap].cubemap);

		glDisable(GL_CULL_FACE);

		for (int i = 0; i < 2; i++) {
			state.cube_to_dp_shader.set_uniform(CubeToDpShaderGLES2::Z_FLIP, i == 1);
			state.cube_to_dp_shader.set_uniform(CubeToDpShaderGLES2::Z_NEAR, light_projection.get_z_near());
			state.cube_to_dp_shader.set_uniform(CubeToDpShaderGLES2::Z_FAR, light_projection.get_z_far());
			state.cube_to_dp_shader.set_uniform(CubeToDpShaderGLES2::BIAS, light->param[RS::LIGHT_PARAM_SHADOW_BIAS]);

			uint32_t local_width = width;
			uint32_t local_height = height;
			uint32_t local_x = x;
			uint32_t local_y = y;

			if (light->omni_shadow_detail == RS::LIGHT_OMNI_SHADOW_DETAIL_HORIZONTAL) {
				local_height /= 2;
				local_y += i * local_height;
			} else {
				local_width /= 2;
				local_x += i * local_width;
			}

			glViewport(local_x, local_y, local_width, local_height);
			glScissor(local_x, local_y, local_width, local_height);

			glEnable(GL_SCISSOR_TEST);

			glClearDepth(1.0f);

			glClear(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_SCISSOR_TEST);

			glDisable(GL_BLEND);

			storage->_copy_screen();
		}
	}

	if (storage->frame.current_rt) {
		glViewport(0, 0, storage->frame.current_rt->width, storage->frame.current_rt->height);
	}
	if (!storage->config.use_rgba_3d_shadows) {
		glColorMask(1, 1, 1, 1);
	}
}

void RasterizerSceneGLES2::set_scene_pass(uint64_t p_pass) {
	scene_pass = p_pass;
}

bool RasterizerSceneGLES2::free(RID p_rid) {
	if (light_instance_owner.owns(p_rid)) {
		LightInstance *light_instance = light_instance_owner.getptr(p_rid);

		//remove from shadow atlases..
		for (RBSet<RID>::Element *E = light_instance->shadow_atlases.front(); E; E = E->next()) {
			ShadowAtlas *shadow_atlas = shadow_atlas_owner.get(E->get());
			ERR_CONTINUE(!shadow_atlas->shadow_owners.has(p_rid));
			uint32_t key = shadow_atlas->shadow_owners[p_rid];
			uint32_t q = (key >> ShadowAtlas::QUADRANT_SHIFT) & 0x3;
			uint32_t s = key & ShadowAtlas::SHADOW_INDEX_MASK;

			shadow_atlas->quadrants[q].shadows.write[s].owner = RID();
			shadow_atlas->shadow_owners.erase(p_rid);
		}

		light_instance_owner.free(p_rid);
		memdelete(light_instance);

	} else if (shadow_atlas_owner.owns(p_rid)) {
		ShadowAtlas *shadow_atlas = shadow_atlas_owner.get(p_rid);
		shadow_atlas_set_size(p_rid, 0);
		shadow_atlas_owner.free(p_rid);
		memdelete(shadow_atlas);

	} else {
		return false;
	}

	return true;
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

	shadow_atlas_realloc_tolerance_msec = 500;

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

	// cubemaps for shadows
	if (storage->config.support_shadow_cubemaps) { //not going to be used
		int max_shadow_cubemap_sampler_size = MIN(int(GLOBAL_GET("rendering/quality/shadow_atlas/cubemap_size")), storage->config.max_cubemap_texture_size);

		int cube_size = max_shadow_cubemap_sampler_size;

		WRAPPED_GL_ACTIVE_TEXTURE(GL_TEXTURE0);

		while (cube_size >= 32) {
			ShadowCubeMap cube;

			cube.size = cube_size;

			glGenTextures(1, &cube.cubemap);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cube.cubemap);

			for (int i = 0; i < 6; i++) {
				glTexImage2D(_cube_side_enum[i], 0, storage->config.depth_internalformat, cube_size, cube_size, 0, GL_DEPTH_COMPONENT, storage->config.depth_type, nullptr);
			}

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glGenFramebuffers(6, cube.fbo);
			for (int i = 0; i < 6; i++) {
				glBindFramebuffer(GL_FRAMEBUFFER, cube.fbo[i]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _cube_side_enum[i], cube.cubemap, 0);
			}

			shadow_cubemaps.push_back(cube);

			cube_size >>= 1;
		}
	}

	directional_shadow_create();

	shadow_filter_mode = SHADOW_FILTER_NEAREST;

	glFrontFace(GL_CW);
}

void RasterizerSceneGLES2::iteration() {
	shadow_filter_mode = ShadowFilterMode(int(GLOBAL_GET("rendering/quality/shadows/filter_mode")));

	const int directional_shadow_size_new = next_power_of_2(int(GLOBAL_GET("rendering/quality/directional_shadow/size")));
	if (directional_shadow_size != directional_shadow_size_new) {
		directional_shadow_size = directional_shadow_size_new;
		directional_shadow_create();
	}
}

void RasterizerSceneGLES2::finalize() {
}

RasterizerSceneGLES2::RasterizerSceneGLES2() {
	_light_counter = 0;
	directional_shadow_size = next_power_of_2(int(GLOBAL_GET("rendering/quality/directional_shadow/size")));
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

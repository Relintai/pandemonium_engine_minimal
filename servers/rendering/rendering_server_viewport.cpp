
/*  rendering_server_viewport.cpp                                           */


#include "rendering_server_viewport.h"

#include "core/config/project_settings.h"
#include "core/config/engine.h"
#include "rendering_server_canvas.h"
#include "rendering_server_globals.h"

static Transform2D _canvas_get_transform(RenderingServerViewport::Viewport *p_viewport, RenderingServerCanvas::Canvas *p_canvas, RenderingServerViewport::Viewport::CanvasData *p_canvas_data, const Vector2 &p_vp_size) {
	Transform2D xf = p_viewport->global_transform;

	float scale = 1.0;

	if (p_viewport->canvas_map.has(p_canvas->parent)) {
		Transform2D c_xform = p_viewport->canvas_map[p_canvas->parent].transform;
		xf = xf * c_xform;
		scale = p_canvas->parent_scale;
	}

	Transform2D c_xform = p_canvas_data->transform;
	xf = xf * c_xform;

	if (scale != 1.0 && !RSG::canvas->disable_scale) {
		Vector2 pivot = p_vp_size * 0.5;
		Transform2D xfpivot;
		xfpivot.set_origin(pivot);
		Transform2D xfscale;
		xfscale.scale(Vector2(scale, scale));

		xf = xfpivot.affine_inverse() * xf;
		xf = xfscale * xf;
		xf = xfpivot * xf;
	}

	return xf;
}

void RenderingServerViewport::_draw_viewport(Viewport *p_viewport) {
	/* Camera should always be BEFORE any other 3D */

	if (p_viewport->clear_mode != RS::VIEWPORT_CLEAR_NEVER) {
		RSG::rasterizer->clear_render_target(p_viewport->transparent_bg ? Color(0, 0, 0, 0) : clear_color);
		if (p_viewport->clear_mode == RS::VIEWPORT_CLEAR_ONLY_NEXT_FRAME) {
			p_viewport->clear_mode = RS::VIEWPORT_CLEAR_NEVER;
		}
	}

	if (!p_viewport->hide_canvas) {
		RBMap<Viewport::CanvasKey, Viewport::CanvasData *> canvas_map;

		Rect2 clip_rect(0, 0, p_viewport->size.x, p_viewport->size.y);
		Rect2 shadow_rect;

		for (RBMap<RID, Viewport::CanvasData>::Element *E = p_viewport->canvas_map.front(); E; E = E->next()) {
			canvas_map[Viewport::CanvasKey(E->key(), E->get().layer, E->get().sublayer)] = &E->get();
		}

		RSG::rasterizer->restore_render_target(false);

		for (RBMap<Viewport::CanvasKey, Viewport::CanvasData *>::Element *E = canvas_map.front(); E; E = E->next()) {
			RenderingServerCanvas::Canvas *canvas = static_cast<RenderingServerCanvas::Canvas *>(E->get()->canvas);

			Transform2D xform = _canvas_get_transform(p_viewport, canvas, E->get(), clip_rect.size);

			int canvas_layer_id = E->get()->layer;

			RSG::canvas->render_canvas(canvas, xform, clip_rect, canvas_layer_id);
		}
	}
}

void RenderingServerViewport::draw_viewports() {
	if (Engine::get_singleton()->is_editor_hint()) {
		clear_color = GLOBAL_GET("rendering/environment/default_clear_color");
	}

	//sort viewports
	active_viewports.sort_custom<ViewportSort>();

	//draw viewports
	for (int i = 0; i < active_viewports.size(); i++) {
		Viewport *vp = active_viewports[i];

		if (vp->update_mode == RS::VIEWPORT_UPDATE_DISABLED) {
			continue;
		}

		ERR_CONTINUE(!vp->render_target.is_valid());

		bool visible = vp->viewport_to_screen_rect != Rect2() || vp->update_mode == RS::VIEWPORT_UPDATE_ALWAYS || vp->update_mode == RS::VIEWPORT_UPDATE_ONCE || (vp->update_mode == RS::VIEWPORT_UPDATE_WHEN_VISIBLE && RSG::storage->render_target_was_used(vp->render_target));
		visible = visible && vp->size.x > 1 && vp->size.y > 1;

		if (!visible) {
			continue;
		}

		RSG::storage->render_target_clear_used(vp->render_target);

		RSG::storage->render_target_set_external_texture(vp->render_target, 0, 0);
		RSG::rasterizer->set_current_render_target(vp->render_target);

		RSG::storage->render_info_begin_capture();

		// render standard mono camera
		_draw_viewport(vp);

		RSG::storage->render_info_end_capture();
		vp->render_info[RS::VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_OBJECTS_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_VERTICES_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_VERTICES_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_MATERIAL_CHANGES_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_MATERIAL_CHANGES_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_SHADER_CHANGES_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_SHADER_CHANGES_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_SURFACE_CHANGES_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_SURFACE_CHANGES_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_DRAW_CALLS_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_2D_ITEMS_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_2D_ITEMS_IN_FRAME);
		vp->render_info[RS::VIEWPORT_RENDER_INFO_2D_DRAW_CALLS_IN_FRAME] = RSG::storage->get_captured_render_info(RS::INFO_2D_DRAW_CALLS_IN_FRAME);

		if (vp->viewport_to_screen_rect != Rect2() && (!vp->viewport_render_direct_to_screen || !RSG::rasterizer->is_low_end())) {
			//copy to screen if set as such
			RSG::rasterizer->set_current_render_target(RID());
			RSG::rasterizer->blit_render_target_to_screen(vp->render_target, vp->viewport_to_screen_rect, vp->viewport_to_screen);
		}

		if (vp->update_mode == RS::VIEWPORT_UPDATE_ONCE) {
			vp->update_mode = RS::VIEWPORT_UPDATE_DISABLED;
		}
	}
}

RID RenderingServerViewport::viewport_create() {
	Viewport *viewport = memnew(Viewport);

	RID rid = viewport_owner.make_rid(viewport);

	viewport->self = rid;
	viewport->hide_scenario = false;
	viewport->hide_canvas = false;
	viewport->render_target = RSG::storage->render_target_create();
	viewport->viewport_render_direct_to_screen = false;

	return rid;
}

void RenderingServerViewport::viewport_set_size(RID p_viewport, int p_width, int p_height) {
	ERR_FAIL_COND(p_width < 0 && p_height < 0);

	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->size = Size2(p_width, p_height);

	RSG::storage->render_target_set_size(viewport->render_target, p_width, p_height);
}

void RenderingServerViewport::viewport_set_active(RID p_viewport, bool p_active) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	if (p_active) {
		ERR_FAIL_COND_MSG(active_viewports.find(viewport) != -1, "Can't make active a Viewport that is already active.");
		active_viewports.push_back(viewport);
	} else {
		active_viewports.erase(viewport);
	}
}

void RenderingServerViewport::viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->parent = p_parent_viewport;
}

void RenderingServerViewport::viewport_set_clear_mode(RID p_viewport, RS::ViewportClearMode p_clear_mode) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->clear_mode = p_clear_mode;
}

void RenderingServerViewport::viewport_attach_to_screen(RID p_viewport, const Rect2 &p_rect, int p_screen) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	// If using GLES2 we can optimize this operation by rendering directly to system_fbo
	// instead of rendering to fbo and copying to system_fbo after
	if (RSG::rasterizer->is_low_end() && viewport->viewport_render_direct_to_screen) {
		RSG::storage->render_target_set_size(viewport->render_target, p_rect.size.x, p_rect.size.y);
		RSG::storage->render_target_set_position(viewport->render_target, p_rect.position.x, p_rect.position.y);
	}

	viewport->viewport_to_screen_rect = p_rect;
	viewport->viewport_to_screen = p_screen;
}

void RenderingServerViewport::viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	if (p_enable == viewport->viewport_render_direct_to_screen) {
		return;
	}

	// if disabled, reset render_target size and position
	if (!p_enable) {
		RSG::storage->render_target_set_position(viewport->render_target, 0, 0);
		RSG::storage->render_target_set_size(viewport->render_target, viewport->size.x, viewport->size.y);
	}

	RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_DIRECT_TO_SCREEN, p_enable);
	viewport->viewport_render_direct_to_screen = p_enable;

	// if attached to screen already, setup screen size and position, this needs to happen after setting flag to avoid an unnecessary buffer allocation
	if (RSG::rasterizer->is_low_end() && viewport->viewport_to_screen_rect != Rect2() && p_enable) {
		RSG::storage->render_target_set_size(viewport->render_target, viewport->viewport_to_screen_rect.size.x, viewport->viewport_to_screen_rect.size.y);
		RSG::storage->render_target_set_position(viewport->render_target, viewport->viewport_to_screen_rect.position.x, viewport->viewport_to_screen_rect.position.y);
	}
}

void RenderingServerViewport::viewport_detach(RID p_viewport) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	// if render_direct_to_screen was used, reset size and position
	if (RSG::rasterizer->is_low_end() && viewport->viewport_render_direct_to_screen) {
		RSG::storage->render_target_set_position(viewport->render_target, 0, 0);
		RSG::storage->render_target_set_size(viewport->render_target, viewport->size.x, viewport->size.y);
	}

	viewport->viewport_to_screen_rect = Rect2();
	viewport->viewport_to_screen = 0;
}

void RenderingServerViewport::viewport_set_update_mode(RID p_viewport, RS::ViewportUpdateMode p_mode) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->update_mode = p_mode;
}
void RenderingServerViewport::viewport_set_vflip(RID p_viewport, bool p_enable) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_VFLIP, p_enable);
}

RID RenderingServerViewport::viewport_get_texture(RID p_viewport) const {
	const Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND_V(!viewport, RID());

	return RSG::storage->render_target_get_texture(viewport->render_target);
}

void RenderingServerViewport::viewport_set_hide_canvas(RID p_viewport, bool p_hide) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->hide_canvas = p_hide;
}

void RenderingServerViewport::viewport_attach_canvas(RID p_viewport, RID p_canvas) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	ERR_FAIL_COND(viewport->canvas_map.has(p_canvas));
	RenderingServerCanvas::Canvas *canvas = RSG::canvas->canvas_owner.getornull(p_canvas);
	ERR_FAIL_COND(!canvas);

	canvas->viewports.insert(p_viewport);
	viewport->canvas_map[p_canvas] = Viewport::CanvasData();
	viewport->canvas_map[p_canvas].layer = 0;
	viewport->canvas_map[p_canvas].sublayer = 0;
	viewport->canvas_map[p_canvas].canvas = canvas;
}

void RenderingServerViewport::viewport_remove_canvas(RID p_viewport, RID p_canvas) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RenderingServerCanvas::Canvas *canvas = RSG::canvas->canvas_owner.getornull(p_canvas);
	ERR_FAIL_COND(!canvas);

	viewport->canvas_map.erase(p_canvas);
	canvas->viewports.erase(p_viewport);
}
void RenderingServerViewport::viewport_set_canvas_transform(RID p_viewport, RID p_canvas, const Transform2D &p_offset) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	ERR_FAIL_COND(!viewport->canvas_map.has(p_canvas));
	viewport->canvas_map[p_canvas].transform = p_offset;
}
void RenderingServerViewport::viewport_set_transparent_background(RID p_viewport, bool p_enabled) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_TRANSPARENT, p_enabled);
	viewport->transparent_bg = p_enabled;
}

void RenderingServerViewport::viewport_set_global_canvas_transform(RID p_viewport, const Transform2D &p_transform) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->global_transform = p_transform;
}
void RenderingServerViewport::viewport_set_canvas_stacking(RID p_viewport, RID p_canvas, int p_layer, int p_sublayer) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	ERR_FAIL_COND(!viewport->canvas_map.has(p_canvas));
	viewport->canvas_map[p_canvas].layer = p_layer;
	viewport->canvas_map[p_canvas].sublayer = p_sublayer;
}

void RenderingServerViewport::viewport_set_msaa(RID p_viewport, RS::ViewportMSAA p_msaa) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_msaa(viewport->render_target, p_msaa);
}

void RenderingServerViewport::viewport_set_use_fxaa(RID p_viewport, bool p_fxaa) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_use_fxaa(viewport->render_target, p_fxaa);
}

void RenderingServerViewport::viewport_set_use_debanding(RID p_viewport, bool p_debanding) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_use_debanding(viewport->render_target, p_debanding);
}

void RenderingServerViewport::viewport_set_sharpen_intensity(RID p_viewport, float p_intensity) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_sharpen_intensity(viewport->render_target, p_intensity);
}

void RenderingServerViewport::viewport_set_hdr(RID p_viewport, bool p_enabled) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_HDR, p_enabled);
}

void RenderingServerViewport::viewport_set_use_32_bpc_depth(RID p_viewport, bool p_enabled) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_USE_32_BPC_DEPTH, p_enabled);
}

void RenderingServerViewport::viewport_set_usage(RID p_viewport, RS::ViewportUsage p_usage) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	switch (p_usage) {
		case RS::VIEWPORT_USAGE_2D: {
			RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_NO_3D, true);
			RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_NO_3D_EFFECTS, true);
			RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_NO_SAMPLING, false);
		} break;
		case RS::VIEWPORT_USAGE_2D_NO_SAMPLING: {
			RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_NO_3D, true);
			RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_NO_3D_EFFECTS, true);
			RSG::storage->render_target_set_flag(viewport->render_target, RasterizerStorage::RENDER_TARGET_NO_SAMPLING, true);
		} break;
	}
}

int RenderingServerViewport::viewport_get_render_info(RID p_viewport, RS::ViewportRenderInfo p_info) {
	ERR_FAIL_INDEX_V(p_info, RS::VIEWPORT_RENDER_INFO_MAX, -1);

	Viewport *viewport = viewport_owner.getornull(p_viewport);
	if (!viewport) {
		return 0; //there should be a lock here..
	}

	return viewport->render_info[p_info];
}

void RenderingServerViewport::viewport_set_debug_draw(RID p_viewport, RS::ViewportDebugDraw p_draw) {
	Viewport *viewport = viewport_owner.getornull(p_viewport);
	ERR_FAIL_COND(!viewport);

	viewport->debug_draw = p_draw;
}

bool RenderingServerViewport::free(RID p_rid) {
	if (viewport_owner.owns(p_rid)) {
		Viewport *viewport = viewport_owner.getornull(p_rid);

		RSG::storage->free(viewport->render_target);

		while (viewport->canvas_map.front()) {
			viewport_remove_canvas(p_rid, viewport->canvas_map.front()->key());
		}

		active_viewports.erase(viewport);

		viewport_owner.free(p_rid);
		memdelete(viewport);

		return true;
	}

	return false;
}

void RenderingServerViewport::set_default_clear_color(const Color &p_color) {
	clear_color = p_color;
}

RenderingServerViewport::RenderingServerViewport() {
}

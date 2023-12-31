#ifndef RENDERINGSERVERVIEWPORT_H
#define RENDERINGSERVERVIEWPORT_H

/*  rendering_server_viewport.h                                             */


#include "core/containers/self_list.h"
#include "rasterizer.h"
#include "servers/rendering_server.h"

class RenderingServerViewport {
public:
	struct CanvasBase : public RID_Data {
	};

	struct Viewport : public RID_Data {
		RID self;
		RID parent;

		Size2i size;

		RS::ViewportUpdateMode update_mode;
		RID render_target;
		RID render_target_texture;

		int viewport_to_screen;
		Rect2 viewport_to_screen_rect;
		bool viewport_render_direct_to_screen;

		bool hide_scenario;
		bool hide_canvas;

		int render_info[RS::VIEWPORT_RENDER_INFO_MAX];
		RS::ViewportDebugDraw debug_draw;

		RS::ViewportClearMode clear_mode;

		bool transparent_bg;

		struct CanvasKey {
			int64_t stacking;
			RID canvas;
			bool operator<(const CanvasKey &p_canvas) const {
				if (stacking == p_canvas.stacking) {
					return canvas < p_canvas.canvas;
				}
				return stacking < p_canvas.stacking;
			}
			CanvasKey() {
				stacking = 0;
			}
			CanvasKey(const RID &p_canvas, int p_layer, int p_sublayer) {
				canvas = p_canvas;
				int64_t sign = p_layer < 0 ? -1 : 1;
				stacking = sign * (((int64_t)ABS(p_layer)) << 32) + p_sublayer;
			}
			int get_layer() const { return stacking >> 32; }
		};

		struct CanvasData {
			CanvasBase *canvas;
			Transform2D transform;
			int layer;
			int sublayer;
		};

		Transform2D global_transform;

		RBMap<RID, CanvasData> canvas_map;

		Viewport() {
			update_mode = RS::VIEWPORT_UPDATE_WHEN_VISIBLE;
			clear_mode = RS::VIEWPORT_CLEAR_ALWAYS;
			transparent_bg = false;
			viewport_to_screen = 0;
			debug_draw = RS::VIEWPORT_DEBUG_DRAW_DISABLED;
			for (int i = 0; i < RS::VIEWPORT_RENDER_INFO_MAX; i++) {
				render_info[i] = 0;
			}
		}
	};

	mutable RID_Owner<Viewport> viewport_owner;

	struct ViewportSort {
		_FORCE_INLINE_ bool operator()(const Viewport *p_left, const Viewport *p_right) const {
			bool left_to_screen = p_left->viewport_to_screen_rect.size != Size2();
			bool right_to_screen = p_right->viewport_to_screen_rect.size != Size2();

			if (left_to_screen == right_to_screen) {
				return p_left->parent == p_right->self;
			}
			return right_to_screen;
		}
	};

	Vector<Viewport *> active_viewports;

private:
	Color clear_color;
	void _draw_viewport(Viewport *p_viewport);

public:
	RID viewport_create();

	void viewport_set_size(RID p_viewport, int p_width, int p_height);

	void viewport_attach_to_screen(RID p_viewport, const Rect2 &p_rect = Rect2(), int p_screen = 0);
	void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);
	void viewport_detach(RID p_viewport);

	void viewport_set_active(RID p_viewport, bool p_active);
	void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);
	void viewport_set_update_mode(RID p_viewport, RS::ViewportUpdateMode p_mode);
	void viewport_set_vflip(RID p_viewport, bool p_enable);

	void viewport_set_clear_mode(RID p_viewport, RS::ViewportClearMode p_clear_mode);

	RID viewport_get_texture(RID p_viewport) const;

	void viewport_set_hide_canvas(RID p_viewport, bool p_hide);

	void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	void viewport_set_canvas_transform(RID p_viewport, RID p_canvas, const Transform2D &p_offset);
	void viewport_set_transparent_background(RID p_viewport, bool p_enabled);

	void viewport_set_global_canvas_transform(RID p_viewport, const Transform2D &p_transform);
	void viewport_set_canvas_stacking(RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	void viewport_set_msaa(RID p_viewport, RS::ViewportMSAA p_msaa);
	void viewport_set_use_fxaa(RID p_viewport, bool p_fxaa);
	void viewport_set_use_debanding(RID p_viewport, bool p_debanding);
	void viewport_set_sharpen_intensity(RID p_viewport, float p_intensity);
	void viewport_set_hdr(RID p_viewport, bool p_enabled);
	void viewport_set_use_32_bpc_depth(RID p_viewport, bool p_enabled);
	void viewport_set_usage(RID p_viewport, RS::ViewportUsage p_usage);

	virtual int viewport_get_render_info(RID p_viewport, RS::ViewportRenderInfo p_info);
	virtual void viewport_set_debug_draw(RID p_viewport, RS::ViewportDebugDraw p_draw);

	void set_default_clear_color(const Color &p_color);
	void draw_viewports();

	bool free(RID p_rid);

	RenderingServerViewport();
	virtual ~RenderingServerViewport() {}
};

#endif // RENDERINGSERVERVIEWPORT_H

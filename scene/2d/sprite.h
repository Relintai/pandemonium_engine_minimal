#ifndef SPRITE_H
#define SPRITE_H

/*  sprite.h                                                             */


#include "core/object/reference.h"
#include "scene/main/node_2d.h"

class Sprite : public Node2D {
	GDCLASS(Sprite, Node2D);

	Ref<Texture> texture;
	Ref<Texture> normal_map;

	bool centered;
	Point2 offset;

	bool hflip;
	bool vflip;
	bool region;
	Rect2 region_rect;
	bool region_filter_clip;

	int frame;

	int vframes;
	int hframes;

	void _get_rects(Rect2 &r_src_rect, Rect2 &r_dst_rect, bool &r_filter_clip) const;

	void _texture_changed();

protected:
	void _notification(int p_what);

	static void _bind_methods();

	virtual void _validate_property(PropertyInfo &property) const;

public:
	bool is_pixel_opaque(const Point2 &p_point) const;

	void set_texture(const Ref<Texture> &p_texture);
	Ref<Texture> get_texture() const;

	void set_normal_map(const Ref<Texture> &p_texture);
	Ref<Texture> get_normal_map() const;

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2 &p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_region(bool p_region);
	bool is_region() const;

	void set_region_filter_clip(bool p_enable);
	bool is_region_filter_clip_enabled() const;

	void set_region_rect(const Rect2 &p_region_rect);
	Rect2 get_region_rect() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_coords(const Vector2 &p_coord);
	Vector2 get_frame_coords() const;

	void set_vframes(int p_amount);
	int get_vframes() const;

	void set_hframes(int p_amount);
	int get_hframes() const;

	Rect2 get_rect() const;
	virtual Rect2 get_anchorable_rect() const;

	Sprite();
	~Sprite();
};

#endif // SPRITE_H

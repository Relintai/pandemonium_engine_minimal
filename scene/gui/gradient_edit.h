#ifndef GRADIENT_EDIT_H
#define GRADIENT_EDIT_H

/*  gradient_edit.h                                                      */


#include "scene/main/control.h"

#include "scene/resources/gradient.h"

class PopupPanel;
class ColorPicker;
class ImageTexture;

class GradientEdit : public Control {
	GDCLASS(GradientEdit, Control);

	PopupPanel *popup;
	ColorPicker *picker;

	Ref<ImageTexture> checker;

	bool grabbing;
	int grabbed;
	Vector<Gradient::Point> points;
	Gradient::InterpolationMode interpolation_mode = Gradient::GRADIENT_INTERPOLATE_LINEAR;

	Ref<Gradient> gradient_cache;
	Ref<GradientTexture> preview_texture;

	void _draw_checker(int x, int y, int w, int h);
	void _color_changed(const Color &p_color);
	int _get_point_from_pos(int x);
	void _show_color_picker();

protected:
	void _gui_input(const Ref<InputEvent> &p_event);
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_ramp(const Vector<float> &p_offsets, const Vector<Color> &p_colors);
	Vector<float> get_offsets() const;
	Vector<Color> get_colors() const;
	void set_points(Vector<Gradient::Point> &p_points);
	Vector<Gradient::Point> &get_points();
	void set_interpolation_mode(Gradient::InterpolationMode p_interp_mode);
	Gradient::InterpolationMode get_interpolation_mode();

	virtual Size2 get_minimum_size() const;

	GradientEdit();
	virtual ~GradientEdit();
};

#endif // GRADIENT_EDIT_H

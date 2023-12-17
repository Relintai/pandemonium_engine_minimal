#ifndef REFERENCE_RECT_H
#define REFERENCE_RECT_H

/*  reference_rect.h                                                     */


#include "scene/main/control.h"

class ReferenceRect : public Control {
	GDCLASS(ReferenceRect, Control);

	Color border_color = Color(1, 0, 0);
	float border_width = 1.0;
	bool editor_only = true;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_border_color(const Color &p_color);
	Color get_border_color() const;

	void set_border_width(float p_width);
	float get_border_width() const;

	void set_editor_only(const bool &p_enabled);
	bool get_editor_only() const;
};

#endif // REFERENCE_RECT_H

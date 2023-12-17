
/*  panel.cpp                                                            */


#include "panel.h"
#include "core/string/print_string.h"

void Panel::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		RID ci = get_canvas_item();
		Ref<StyleBox> style = get_theme_stylebox("panel");
		style->draw(ci, Rect2(Point2(), get_size()));
	}
}

Panel::Panel() {
	set_mouse_filter(MOUSE_FILTER_STOP);
}

Panel::~Panel() {
}

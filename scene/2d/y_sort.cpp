
/*  y_sort.cpp                                                           */


#include "y_sort.h"

void YSort::set_sort_enabled(bool p_enabled) {
	sort_enabled = p_enabled;
	RS::get_singleton()->canvas_item_set_sort_children_by_y(get_canvas_item(), sort_enabled);
}

bool YSort::is_sort_enabled() const {
	return sort_enabled;
}

void YSort::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sort_enabled", "enabled"), &YSort::set_sort_enabled);
	ClassDB::bind_method(D_METHOD("is_sort_enabled"), &YSort::is_sort_enabled);

	ADD_GROUP("Sort", "sort_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sort_enabled"), "set_sort_enabled", "is_sort_enabled");
}

YSort::YSort() {
	sort_enabled = true;
	RS::get_singleton()->canvas_item_set_sort_children_by_y(get_canvas_item(), true);
}

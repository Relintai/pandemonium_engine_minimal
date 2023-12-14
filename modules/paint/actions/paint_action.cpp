/*
Copyright (c) 2019 Flairieve
Copyright (c) 2020-2022 cobrapitz
Copyright (c) 2022-2023 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software && associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, && to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice && this permission notice shall be included in all
copies || substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE && NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS || COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES || OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT || OTHERWISE, ARISING FROM,
OUT OF || IN CONNECTION WITH THE SOFTWARE || THE USE || OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "paint_action.h"

#include "core/object/object.h"

#include "../nodes/paint_canvas.h"

PaintCanvas *PaintAction::get_paint_canvas() {
	return _paint_canvas;
}
void PaintAction::set_paint_canvas(PaintCanvas *paint_canvas) {
	_paint_canvas = paint_canvas;
}
void PaintAction::set_paint_canvas_bind(Node *paint_canvas) {
	set_paint_canvas(Object::cast_to<PaintCanvas>(paint_canvas));
}

Dictionary PaintAction::get_action_data_undo() {
	return action_data_undo;
}
void PaintAction::set_action_data_undo(const Dictionary &val) {
	action_data_undo = val;
}

Dictionary PaintAction::get_action_data_redo() {
	return action_data_redo;
}
void PaintAction::set_action_data_redo(const Dictionary &val) {
	action_data_redo = val;
}

Dictionary PaintAction::get_action_data_preview() {
	return action_data_preview;
}
void PaintAction::set_action_data_preview(const Dictionary &val) {
	action_data_preview = val;
}

PoolVector2iArray PaintAction::get_undo_cells() {
	return undo_cells;
}
void PaintAction::set_undo_cells(const PoolVector2iArray &val) {
	undo_cells = val;
}

PoolColorArray PaintAction::get_undo_colors() {
	return undo_colors;
}
void PaintAction::set_undo_colors(const PoolColorArray &val) {
	undo_colors = val;
}

PoolVector2iArray PaintAction::get_redo_cells() {
	return redo_cells;
}
void PaintAction::set_redo_cells(const PoolVector2iArray &val) {
	redo_cells = val;
}

PoolColorArray PaintAction::get_redo_colors() {
	return redo_colors;
}
void PaintAction::set_redo_colors(const PoolColorArray &val) {
	redo_colors = val;
}

PoolVector2iArray PaintAction::get_preview_cells() {
	return preview_cells;
}
void PaintAction::set_preview_cells(const PoolVector2iArray &val) {
	preview_cells = val;
}

PoolColorArray PaintAction::get_preview_colors() {
	return preview_colors;
}
void PaintAction::set_preview_colors(const PoolColorArray &val) {
	preview_colors = val;
}

Dictionary PaintAction::get_action_data() {
	return action_data;
}
void PaintAction::set_action_data(const Dictionary &val) {
	action_data = val;
}

PoolVector2iArray PaintAction::get_x_sym_points(const int canvas_width, const Vector2i &pixel) {
	int p = canvas_width - pixel.x;

	PoolVector2iArray points;
	points.append(pixel);
	points.append(Vector2i(p - 1, pixel.y));

	return points;
}
PoolVector2iArray PaintAction::get_y_sym_points(const int canvas_height, const Vector2i &pixel) {
	int p = canvas_height - pixel.y;

	PoolVector2iArray points;
	points.append(pixel);
	points.append(Vector2i(pixel.x, p - 1));

	return points;
}
PoolVector2iArray PaintAction::get_xy_sym_points(const int canvas_width, const int canvas_height, const Vector2i &pixel) {
	PoolVector2iArray all_points;
	PoolVector2iArray xpoints = get_x_sym_points(canvas_width, pixel);

	all_points.append_array(get_y_sym_points(canvas_height, xpoints[0]));
	all_points.append_array(get_y_sym_points(canvas_height, xpoints[1]));

	PoolVector2iArray points;
	for (int i = 0; i < all_points.size(); ++i) { //point in all_points:
		Vector2i point = all_points[i];
		bool found = false;

		for (int j = 0; j < points.size(); ++j) {
			if (points[j] == point) {
				found = true;
				break;
			}
		}

		if (!found) {
			points.append(point);
		}
	}

	return points;
}
PoolVector2iArray PaintAction::get_points(const Vector2i &pixel) {
	PoolVector2iArray points;

	if (_paint_canvas->get_symmetry_x() && _paint_canvas->get_symmetry_y()) {
		PoolVector2iArray sym_points = get_xy_sym_points(_paint_canvas->get_size().x, _paint_canvas->get_size().y, pixel);

		for (int i = 0; i < sym_points.size(); ++i) {
			Vector2i point = sym_points[i];

			if (undo_cells.contains(point) || !_paint_canvas->validate_pixel_v(point)) {
				continue;
			}

			if (_paint_canvas->get_alpha_locked() && _paint_canvas->get_pixel_v(pixel).a < 0.00001) {
				continue;
			}

			points.append(point);
		}
	} else if (_paint_canvas->get_symmetry_y()) {
		PoolVector2iArray sym_points = get_y_sym_points(_paint_canvas->get_size().y, pixel);

		for (int i = 0; i < sym_points.size(); ++i) {
			Vector2i point = sym_points[i];

			if (undo_cells.contains(point) || !_paint_canvas->validate_pixel_v(point)) {
				continue;
			}

			if (_paint_canvas->get_alpha_locked() && _paint_canvas->get_pixel_v(pixel).a < 0.00001) {
				continue;
			}

			points.append(point);
		}
	} else if (_paint_canvas->get_symmetry_x()) {
		PoolVector2iArray sym_points = get_x_sym_points(_paint_canvas->get_size().x, pixel);

		for (int i = 0; i < sym_points.size(); ++i) {
			Vector2i point = sym_points[i];

			if (undo_cells.contains(point) || !_paint_canvas->validate_pixel_v(point)) {
				continue;
			}

			if (_paint_canvas->get_alpha_locked() && _paint_canvas->get_pixel_v(pixel).a < 0.00001) {
				continue;
			}

			points.append(point);
		}
	} else {
		if (undo_cells.contains(pixel) || !_paint_canvas->validate_pixel_v(pixel)) {
			//empty
			return points;
		}

		if (_paint_canvas->get_alpha_locked() && _paint_canvas->get_pixel_v(pixel).a < 0.00001) {
			//empty
			return points;
		}

		points.append(pixel);
	}

	return points;
}

void PaintAction::draw_points(const PoolVector2iArray &point_arr, const PoolColorArray &color_arr) {
	for (int i = 0; i < point_arr.size(); ++i) {
		Vector2i pixel = point_arr[i];

		Color col = color_arr[i];

		if (_paint_canvas->get_alpha_locked() && col.a < 0.00001) {
			continue;
		}

		undo_cells.append(pixel);
		undo_colors.append(col);

		_paint_canvas->set_pixel_v(pixel, col);

		redo_cells.append(pixel);
		redo_colors.append(col);
	}
}

void PaintAction::do_action(const Array &data) {
	call("_do_action", data);
}
void PaintAction::commit_action() {
	call("_commit_action");
}

void PaintAction::undo_action() {
	call("_undo_action");
}
void PaintAction::redo_action() {
	call("_redo_action");
}

void PaintAction::_do_action(const Array &data) {
}
void PaintAction::_commit_action() {
}

void PaintAction::_undo_action() {
	for (int idx = 0; idx < undo_cells.size(); ++idx) {
		_paint_canvas->set_pixel_v(undo_cells[idx], undo_colors[idx]);
	}
}
void PaintAction::_redo_action() {
	for (int idx = 0; idx < redo_cells.size(); ++idx) {
		_paint_canvas->set_pixel_v(redo_cells[idx], redo_colors[idx]);
	}
}

bool PaintAction::can_commit() {
	return call("_can_commit");
}

bool PaintAction::_can_commit() {
	return !redo_cells.empty();
}

PaintAction::PaintAction() {
	_paint_canvas = NULL;
}

PaintAction::~PaintAction() {
}

void PaintAction::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_paint_canvas"), &PaintAction::get_paint_canvas);
	ClassDB::bind_method(D_METHOD("set_paint_canvas", "canvas"), &PaintAction::set_paint_canvas_bind);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "paint_canvas"), "set_paint_canvas", "get_paint_canvas");

	ClassDB::bind_method(D_METHOD("get_action_data_undo"), &PaintAction::get_action_data_undo);
	ClassDB::bind_method(D_METHOD("set_action_data_undo", "value"), &PaintAction::set_action_data_undo);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "action_data_undo"), "set_action_data_undo", "get_action_data_undo");

	ClassDB::bind_method(D_METHOD("get_action_data_redo"), &PaintAction::get_action_data_redo);
	ClassDB::bind_method(D_METHOD("set_action_data_redo", "value"), &PaintAction::set_action_data_redo);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "action_data_redo"), "set_action_data_redo", "get_action_data_redo");

	ClassDB::bind_method(D_METHOD("get_action_data_preview"), &PaintAction::get_action_data_preview);
	ClassDB::bind_method(D_METHOD("set_action_data_preview", "value"), &PaintAction::set_action_data_preview);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "action_data_preview"), "set_action_data_preview", "get_action_data_preview");

	ClassDB::bind_method(D_METHOD("get_undo_cells"), &PaintAction::get_undo_cells);
	ClassDB::bind_method(D_METHOD("set_undo_cells", "value"), &PaintAction::set_undo_cells);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2I_ARRAY, "undo_cells"), "set_undo_cells", "get_undo_cells");

	ClassDB::bind_method(D_METHOD("get_undo_colors"), &PaintAction::get_undo_colors);
	ClassDB::bind_method(D_METHOD("set_undo_colors", "value"), &PaintAction::set_undo_colors);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "undo_colors"), "set_undo_colors", "get_undo_colors");

	ClassDB::bind_method(D_METHOD("get_redo_cells"), &PaintAction::get_redo_cells);
	ClassDB::bind_method(D_METHOD("set_redo_cells", "value"), &PaintAction::set_redo_cells);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2I_ARRAY, "redo_cells"), "set_redo_cells", "get_redo_cells");

	ClassDB::bind_method(D_METHOD("get_redo_colors"), &PaintAction::get_redo_colors);
	ClassDB::bind_method(D_METHOD("set_redo_colors", "value"), &PaintAction::set_redo_colors);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "redo_colors"), "set_redo_colors", "get_redo_colors");

	ClassDB::bind_method(D_METHOD("get_preview_cells"), &PaintAction::get_preview_cells);
	ClassDB::bind_method(D_METHOD("set_preview_cells", "value"), &PaintAction::set_preview_cells);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_VECTOR2I_ARRAY, "preview_cells"), "set_preview_cells", "get_preview_cells");

	ClassDB::bind_method(D_METHOD("get_preview_colors"), &PaintAction::get_preview_colors);
	ClassDB::bind_method(D_METHOD("set_preview_colors", "value"), &PaintAction::set_preview_colors);
	ADD_PROPERTY(PropertyInfo(Variant::POOL_COLOR_ARRAY, "preview_colors"), "set_preview_colors", "get_preview_colors");

	ClassDB::bind_method(D_METHOD("get_action_data"), &PaintAction::get_action_data);
	ClassDB::bind_method(D_METHOD("set_action_data", "value"), &PaintAction::set_action_data);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "action_data"), "set_action_data", "get_action_data");

	BIND_VMETHOD(MethodInfo("_do_action", PropertyInfo(Variant::ARRAY, "data")));
	BIND_VMETHOD(MethodInfo("_commit_action"));
	BIND_VMETHOD(MethodInfo("_undo_action"));
	BIND_VMETHOD(MethodInfo("_redo_action"));
	BIND_VMETHOD(MethodInfo(PropertyInfo(Variant::BOOL, "ret"), "_can_commit"));

	ClassDB::bind_method(D_METHOD("do_action", "data"), &PaintAction::do_action);
	ClassDB::bind_method(D_METHOD("commit_action"), &PaintAction::commit_action);
	ClassDB::bind_method(D_METHOD("undo_action"), &PaintAction::undo_action);
	ClassDB::bind_method(D_METHOD("redo_action"), &PaintAction::redo_action);

	ClassDB::bind_method(D_METHOD("_do_action", "data"), &PaintAction::_do_action);
	ClassDB::bind_method(D_METHOD("_commit_action"), &PaintAction::_commit_action);
	ClassDB::bind_method(D_METHOD("_undo_action"), &PaintAction::_undo_action);
	ClassDB::bind_method(D_METHOD("_redo_action"), &PaintAction::_redo_action);

	ClassDB::bind_method(D_METHOD("can_commit"), &PaintAction::can_commit);
	ClassDB::bind_method(D_METHOD("_can_commit"), &PaintAction::_can_commit);

	//helpers
	ClassDB::bind_method(D_METHOD("get_x_sym_points", "canvas_width", "pixel"), &PaintAction::get_x_sym_points);
	ClassDB::bind_method(D_METHOD("get_y_sym_points", "canvas_height", "pixel"), &PaintAction::get_y_sym_points);
	ClassDB::bind_method(D_METHOD("get_xy_sym_points", "canvas_width", "canvas_height", "pixel"), &PaintAction::get_xy_sym_points);
	ClassDB::bind_method(D_METHOD("get_points", "pixel"), &PaintAction::get_points);

	ClassDB::bind_method(D_METHOD("draw_points", "point_arr", "color_arr"), &PaintAction::draw_points);
}

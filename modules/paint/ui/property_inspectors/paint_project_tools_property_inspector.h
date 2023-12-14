#ifndef PAINT_PROJECT_TOOLS_PROPERTY_INSPECTOR_H
#define PAINT_PROJECT_TOOLS_PROPERTY_INSPECTOR_H

/*
Copyright (c) 2022-2023 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "core/object/object_id.h"
#include "core/object/reference.h"
#include "paint_custom_property_inspector.h"

class GridContainer;
class PaintNode;
class PaintProject;
class HFlowContainer;
class ColorSelectorButton;
class Button;
class FileDialog;

class PaintProjectToolsPropertyInspector : public PaintCustomPropertyInspector {
	GDCLASS(PaintProjectToolsPropertyInspector, PaintCustomPropertyInspector);

public:
	void add_action_button(const String &callback, const String &hint, const String &icon, const String &theme_type);

	void _set_paint_node(Node *paint_node);

	PaintProjectToolsPropertyInspector();
	~PaintProjectToolsPropertyInspector();

protected:
	void _on_export_pressed();
	void _on_export_as_pressed();
	void _on_export_as_dialog_file_selected(const String &f);
	void _on_set_colors_as_default_pressed();
	void _on_add_paint_visual_grid_pressed();
	void _on_add_paint_canvas_background_pressed();
	void _on_add_paint_canvas_pressed();
	void _on_add_paint_curve_2d_pressed();
	void _on_add_paint_polygon_2d_pressed();

	//void _notification(int p_what);

	static void _bind_methods();

	FileDialog *_export_as_file_dialog;

	HFlowContainer *_button_contianer;

	ObjectID _current_paint_node;
	ObjectID _current_paint_project;

	bool _ignore_preset_changed_event;
	bool _ignore_color_event;
};

#endif

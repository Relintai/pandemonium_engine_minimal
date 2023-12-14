#ifndef PAINT_EDITOR__PLUGIN_H
#define PAINT_EDITOR__PLUGIN_H

/*
Copyright (c) 2019-2023 Péter Magyar

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

#include "core/object/reference.h"
#include "editor/editor_plugin.h"

class PaintWindow;
class Texture;
class PaintNode;
class PaintInspectorPlugin;

class PaintEditorPlugin : public EditorPlugin {
	GDCLASS(PaintEditorPlugin, EditorPlugin);

public:
	void make_visible(const bool visible);
	String get_name() const;
	void edit(Object *p_object);
	bool handles(Object *p_object) const;
	void edited_scene_changed();

	bool forward_canvas_gui_input(const Ref<InputEvent> &p_event);
	void forward_canvas_draw_over_viewport(Control *p_overlay);
	void forward_canvas_force_draw_over_viewport(Control *p_overlay);

	PaintEditorPlugin(EditorNode *p_node);
	~PaintEditorPlugin();

	EditorNode *editor;

protected:
	void on_node_removed(Node *node);

	void _notification(int p_what);

	static void _bind_methods();

	PaintNode *_active_node;
	PaintInspectorPlugin *_inspector_plugin;
};

#endif

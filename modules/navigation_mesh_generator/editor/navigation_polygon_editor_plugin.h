/**************************************************************************/
/*  navigation_polygon_editor_plugin.h                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef NAVIGATION_POLYGON_EDITOR_PLUGIN_H
#define NAVIGATION_POLYGON_EDITOR_PLUGIN_H

#include "editor/plugins/abstract_polygon_2d_editor.h"
#include "scene/2d/navigation_polygon_instance.h"

#include "editor/editor_plugin.h"

class AcceptDialog;
class OptionButton;
class HBoxContainer;

class NavigationPolygonEditor : public AbstractPolygon2DEditor {
	friend class NavigationPolygonEditorPlugin;

	GDCLASS(NavigationPolygonEditor, AbstractPolygon2DEditor);

	NavigationPolygonInstance *node = nullptr;

	Ref<NavigationPolygon> _ensure_navigation_polygon() const;

	AcceptDialog *err_dialog = nullptr;

	HBoxContainer *bake_hbox = nullptr;
	Button *button_bake = nullptr;
	Button *button_reset = nullptr;
	//Label *bake_info = nullptr;

	void _bake_pressed();
	void _clear_pressed();

	void _update_polygon_editing_state();

protected:
	void _notification(int p_what);
	static void _bind_methods();

	virtual Node2D *_get_node() const;
	virtual void _set_node(Node *p_polygon);

	virtual int _get_polygon_count() const;
	virtual Variant _get_polygon(int p_idx) const;
	virtual void _set_polygon(int p_idx, const Variant &p_polygon) const;

	virtual void _action_add_polygon(const Variant &p_polygon);
	virtual void _action_remove_polygon(int p_idx);
	virtual void _action_set_polygon(int p_idx, const Variant &p_previous, const Variant &p_polygon);

	virtual bool _has_resource() const;
	virtual void _create_resource();

public:
	NavigationPolygonEditor(EditorNode *p_editor, bool p_wip_destructive = true);
	~NavigationPolygonEditor();
};

class NavigationPolygonEditorPlugin : public AbstractPolygon2DEditorPlugin {
	GDCLASS(NavigationPolygonEditorPlugin, AbstractPolygon2DEditorPlugin);

public:
	NavigationPolygonEditorPlugin(EditorNode *p_node);
	~NavigationPolygonEditorPlugin();

protected:
	static void _bind_methods();
};

#endif // NAVIGATION_POLYGON_EDITOR_PLUGIN_H

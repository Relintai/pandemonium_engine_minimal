#ifndef EDITORPLUGINSETTINGS_H
#define EDITORPLUGINSETTINGS_H
/*************************************************************************/
/*  editor_plugin_settings.h                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "scene/gui/box_container.h"

#include "core/object/object.h"
#include "core/string/ustring.h"
#include "core/containers/vector.h"

class Button;
class PluginConfigDialog;
class Tree;
class HBoxContainer;

class EditorPluginSettings : public VBoxContainer {
	GDCLASS(EditorPluginSettings, VBoxContainer);

public:
	void add_control_to_top_bar(Control *p_control);
	void remove_control_from_top_bar(Control *p_control);

	void update_plugins();

	EditorPluginSettings();

protected:
	void _notification(int p_what);

	static void _bind_methods();

	enum {
		BUTTON_PLUGIN_EDIT
	};

	PluginConfigDialog *plugin_config_dialog;
	Button *create_plugin;
	Button *update_list;
	Tree *plugin_list;
	bool updating;

	HBoxContainer *_custom_top_bar_control_container;

	void _plugin_activity_changed();
	void _create_clicked();
	void _cell_button_pressed(Object *p_item, int p_column, int p_id);

	static Vector<String> _get_plugins(const String &p_dir);
};

#endif // EDITORPLUGINSETTINGS_H

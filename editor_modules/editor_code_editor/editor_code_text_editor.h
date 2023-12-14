#ifndef EDITOR_CODE_EDITOR_H
#define EDITOR_CODE_EDITOR_H

/*************************************************************************/
/*  code_editor.h                                                        */
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
#include "scene/gui/dialogs.h"

#include "core/containers/list.h"
#include "core/math/math_defs.h"
#include "core/object/object.h"
#include "core/object/reference.h"
#include "core/string/ustring.h"
#include "core/variant/variant.h"

class Button;
class CheckBox;
class InputEvent;
class Label;
class LineEdit;
class TextEdit;
class Texture;
class TextureButton;
class Timer;
class ToolButton;
struct ScriptCodeCompletionOption;
class EditorGotoLineDialog;
class EditorFindReplaceBar;

typedef void (*EditorCodeTextEditorCodeCompleteFunc)(void *p_ud, const String &p_code, List<ScriptCodeCompletionOption> *r_options, bool &r_forced);

class EditorCodeTextEditor : public VBoxContainer {
	GDCLASS(EditorCodeTextEditor, VBoxContainer);

	TextEdit *text_editor;
	EditorFindReplaceBar *find_replace_bar;
	HBoxContainer *status_bar;

	ToolButton *toggle_scripts_button;
	ToolButton *warning_button;
	Label *warning_count_label;

	Label *line_and_col_txt;

	Label *info;
	Timer *idle;
	Timer *code_complete_timer;

	Timer *font_resize_timer;
	int font_resize_val;
	real_t font_size;

	Label *error;
	int error_line;
	int error_column;

	void _on_settings_change();

	void _update_font();
	void _complete_request();
	Ref<Texture> _get_completion_icon(const ScriptCodeCompletionOption &p_option);
	void _font_resize_timeout();
	bool _add_font_size(int p_delta);

	void _text_editor_gui_input(const Ref<InputEvent> &p_event);
	void _zoom_in();
	void _zoom_out();
	void _zoom_changed();
	void _reset_zoom();

	Color completion_font_color;
	Color completion_string_color;
	Color completion_comment_color;
	EditorCodeTextEditorCodeCompleteFunc code_complete_func;
	void *code_complete_ud;

	void _warning_label_gui_input(const Ref<InputEvent> &p_event);
	void _warning_button_pressed();
	void _set_show_warnings_panel(bool p_show);
	void _error_pressed(const Ref<InputEvent> &p_event);

	void _delete_line(int p_line);
	void _toggle_scripts_pressed();

protected:
	virtual void _load_theme_settings() {}
	virtual void _validate_script() {}
	virtual void _code_complete_script(const String &p_code, List<ScriptCodeCompletionOption> *r_options) {}

	void _text_changed_idle_timeout();
	void _code_complete_timer_timeout();
	void _text_changed();
	void _line_col_changed();
	void _notification(int);
	static void _bind_methods();

	bool is_warnings_panel_opened;

public:
	void trim_trailing_whitespace();
	void insert_final_newline();

	void convert_indent_to_spaces();
	void convert_indent_to_tabs();

	enum CaseStyle {
		UPPER,
		LOWER,
		CAPITALIZE,
	};
	void convert_case(CaseStyle p_case);

	void move_lines_up();
	void move_lines_down();
	void delete_lines();
	void duplicate_selection();

	/// Toggle inline comment on currently selected lines, or on current line if nothing is selected,
	/// by adding or removing comment delimiter
	void toggle_inline_comment(const String &delimiter);

	void goto_line(int p_line);
	void goto_line_selection(int p_line, int p_begin, int p_end);
	void goto_line_centered(int p_line);
	void set_executing_line(int p_line);
	void clear_executing_line();

	Variant get_edit_state();
	void set_edit_state(const Variant &p_state);

	void set_warning_nb(int p_warning_nb);

	void update_editor_settings();
	void set_error(const String &p_error);
	void set_error_pos(int p_line, int p_column);
	void update_line_and_column() { _line_col_changed(); }
	TextEdit *get_text_edit() { return text_editor; }
	EditorFindReplaceBar *get_find_replace_bar() { return find_replace_bar; }
	virtual void apply_code() {}
	void goto_error();

	void toggle_bookmark();
	void goto_next_bookmark();
	void goto_prev_bookmark();
	void remove_all_bookmarks();

	void set_code_complete_func(EditorCodeTextEditorCodeCompleteFunc p_code_complete_func, void *p_ud);

	void validate_script();

	void show_toggle_scripts_button();
	void update_toggle_scripts_button();

	EditorCodeTextEditor();
};

#endif // CODE_EDITOR_H

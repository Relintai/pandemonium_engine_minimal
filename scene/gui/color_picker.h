#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

/*  color_picker.h                                                       */


#include "scene/gui/box_container.h"
#include "scene/gui/button.h"

class Control;
class TextureRect;
class GridContainer;
class HSeparator;
class ToolButton;
class CheckButton;
class HSlider;
class SpinBox;
class Label;
class LineEdit;
class PopupPanel;
class GridContainer;

class ColorPresetButton : public BaseButton {
	GDCLASS(ColorPresetButton, BaseButton);

	Color preset_color;

protected:
	void _notification(int);

public:
	void set_preset_color(const Color &p_color);
	Color get_preset_color() const;

	ColorPresetButton(Color p_color);
	~ColorPresetButton();
};

class ColorPicker : public BoxContainer {
	GDCLASS(ColorPicker, BoxContainer);

private:
	static List<Color> preset_cache;
	Control *screen;
	Control *uv_edit;
	Control *w_edit;
	TextureRect *sample;
	GridContainer *preset_container;
	HSeparator *preset_separator;
	Button *btn_add_preset;
	ToolButton *btn_pick;
	CheckButton *btn_hsv;
	CheckButton *btn_raw;
	HSlider *scroll[4];
	SpinBox *values[4];
	Label *labels[4];
	Button *text_type;
	LineEdit *c_text;
	bool edit_alpha;
	Size2i ms;
	bool text_is_constructor;

	const int preset_column_count = 10;
	List<Color> presets;

	Color color;
	Color old_color;

	bool display_old_color = false;
	bool raw_mode_enabled;
	bool hsv_mode_enabled;
	bool deferred_mode_enabled;
	bool updating;
	bool changing_color;
	bool presets_enabled;
	bool presets_visible;

	float h, s, v;
	Color last_hsv;

	void _html_entered(const String &p_html);
	void _value_changed(double);
	void _update_controls();
	void _update_color(bool p_update_sliders = true);
	void _update_text_value();
	void _text_type_toggled();
	void _sample_input(const Ref<InputEvent> &p_event);
	void _sample_draw();
	void _hsv_draw(int p_which, Control *c);

	void _uv_input(const Ref<InputEvent> &p_event);
	void _w_input(const Ref<InputEvent> &p_event);
	void _preset_input(const Ref<InputEvent> &p_event, const Color &p_color);
	void _screen_input(const Ref<InputEvent> &p_event);
	void _add_preset_pressed();
	void _screen_pick_pressed();
	void _focus_enter();
	void _focus_exit();
	void _html_focus_exit();

	inline int _get_preset_size();
	void _add_preset_button(int p_size, const Color &p_color);

protected:
	void _notification(int);
	static void _bind_methods();

public:
	void set_edit_alpha(bool p_show);
	bool is_editing_alpha() const;

	void _set_pick_color(const Color &p_color, bool p_update_sliders);
	void set_pick_color(const Color &p_color);
	Color get_pick_color() const;
	void set_old_color(const Color &p_color);

	void set_display_old_color(bool p_enabled);
	bool is_displaying_old_color() const;

	void add_preset(const Color &p_color);
	void erase_preset(const Color &p_color);
	PoolColorArray get_presets() const;
	void _update_presets();

	void set_hsv_mode(bool p_enabled);
	bool is_hsv_mode() const;

	void set_raw_mode(bool p_enabled);
	bool is_raw_mode() const;

	void set_deferred_mode(bool p_enabled);
	bool is_deferred_mode() const;

	void set_presets_enabled(bool p_enabled);
	bool are_presets_enabled() const;

	void set_presets_visible(bool p_visible);
	bool are_presets_visible() const;

	void set_focus_on_line_edit();

	ColorPicker();
};

class ColorPickerButton : public Button {
	GDCLASS(ColorPickerButton, Button);

	PopupPanel *popup;
	ColorPicker *picker;
	Color color;
	bool edit_alpha;

	void _about_to_show();
	void _color_changed(const Color &p_color);
	void _modal_closed();

	virtual void pressed();

	void _update_picker();

protected:
	void _notification(int);
	static void _bind_methods();

public:
	void set_pick_color(const Color &p_color);
	Color get_pick_color() const;

	void set_edit_alpha(bool p_show);
	bool is_editing_alpha() const;

	ColorPicker *get_picker();
	PopupPanel *get_popup();

	ColorPickerButton();
};

class ColorSelectorButton : public Button {
	GDCLASS(ColorSelectorButton, Button);

	PopupPanel *popup;
	ColorPicker *picker;
	Color color;
	bool edit_alpha;

	void _about_to_show();
	void _color_changed(const Color &p_color);
	void _modal_closed();

	virtual void popup_open_request();

	void _update_picker();

protected:
	void pressed();
	void toggled(bool p_pressed);
	void _gui_input(Ref<InputEvent> p_event);

	void _notification(int);
	static void _bind_methods();

	int _button;

public:
	void set_pick_color(const Color &p_color);
	Color get_pick_color() const;

	void set_edit_alpha(bool p_show);
	bool is_editing_alpha() const;

	ColorPicker *get_picker();
	PopupPanel *get_popup();

	ColorSelectorButton();
};

#endif // COLOR_PICKER_H

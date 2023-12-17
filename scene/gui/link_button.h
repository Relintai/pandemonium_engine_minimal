#ifndef LINKBUTTON_H
#define LINKBUTTON_H

/*  link_button.h                                                        */


#include "scene/gui/base_button.h"

class LinkButton : public BaseButton {
	GDCLASS(LinkButton, BaseButton);

public:
	enum UnderlineMode {
		UNDERLINE_MODE_ALWAYS,
		UNDERLINE_MODE_ON_HOVER,
		UNDERLINE_MODE_NEVER
	};

private:
	String text;
	String xl_text;
	UnderlineMode underline_mode;
	String uri;

protected:
	virtual void pressed();
	virtual Size2 get_minimum_size() const;
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_text(const String &p_text);
	String get_text() const;
	void set_uri(const String &p_uri);
	String get_uri() const;

	void set_underline_mode(UnderlineMode p_underline_mode);
	UnderlineMode get_underline_mode() const;

	LinkButton();
	~LinkButton();
};

VARIANT_ENUM_CAST(LinkButton::UnderlineMode);

#endif // LINKBUTTON_H

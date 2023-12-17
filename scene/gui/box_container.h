#ifndef BOX_CONTAINER_H
#define BOX_CONTAINER_H

/*  box_container.h                                                      */


#include "scene/gui/container.h"

class BoxContainer : public Container {
	GDCLASS(BoxContainer, Container);

	struct _MinSizeCache {
		int min_size;
		bool will_stretch;
		int final_size;
	};

public:
	enum AlignMode {
		ALIGN_BEGIN,
		ALIGN_CENTER,
		ALIGN_END
	};

	void add_spacer(bool p_begin = false);

	void set_alignment(AlignMode p_align);
	AlignMode get_alignment() const;

	virtual Size2 get_minimum_size() const;

	BoxContainer(bool p_vertical = false);

protected:
	bool _get_vertical() const;
	void _set_vertical(bool p_vertical);

	void _resort();

	void _notification(int p_what);

	static void _bind_methods();

private:
	bool vertical;
	AlignMode align;
};

class HBoxContainer : public BoxContainer {
	GDCLASS(HBoxContainer, BoxContainer);

public:
	HBoxContainer() :
			BoxContainer(false) {}
};

class MarginContainer;
class VBoxContainer : public BoxContainer {
	GDCLASS(VBoxContainer, BoxContainer);

public:
	MarginContainer *add_margin_child(const String &p_label, Control *p_control, bool p_expand = false);

	VBoxContainer() :
			BoxContainer(true) {}
};

class CBoxContainer : public BoxContainer {
	GDCLASS(CBoxContainer, BoxContainer);

public:
	enum ContainerMode {
		CONTAINER_MODE_HORIZONTAL = 0,
		CONTAINER_MODE_VERTICAL,
	};

	ContainerMode get_mode() const;
	void set_mode(const ContainerMode p_mode);

	CBoxContainer();
	~CBoxContainer();

protected:
	static void _bind_methods();

	ContainerMode _mode;
};

VARIANT_ENUM_CAST(BoxContainer::AlignMode);
VARIANT_ENUM_CAST(CBoxContainer::ContainerMode);

#endif // BOX_CONTAINER_H

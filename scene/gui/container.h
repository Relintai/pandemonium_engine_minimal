#ifndef CONTAINER_H
#define CONTAINER_H

/*  container.h                                                          */


#include "scene/main/control.h"

class Container : public Control {
	GDCLASS(Container, Control);

	bool pending_sort;
	void _sort_children();
	void _child_minsize_changed();

protected:
	void queue_sort();
	virtual void add_child_notify(Node *p_child);
	virtual void move_child_notify(Node *p_child);
	virtual void remove_child_notify(Node *p_child);

	void _notification(int p_what);
	static void _bind_methods();

public:
	enum {
		NOTIFICATION_SORT_CHILDREN = 50
	};

	void fit_child_in_rect(Control *p_child, const Rect2 &p_rect);

	virtual String get_configuration_warning() const;

	Container();
};

#endif // CONTAINER_H

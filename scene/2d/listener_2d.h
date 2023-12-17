#ifndef LISTENER_2D_H
#define LISTENER_2D_H

/*  listener_2d.h                                                        */


#include "scene/main/node_2d.h"

class Listener2D : public Node2D {
	GDCLASS(Listener2D, Node2D);

private:
	bool current = false;

	friend class Viewport;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	void _notification(int p_what);

	static void _bind_methods();

public:
	void make_current();
	void clear_current();
	bool is_current() const;
};

#endif

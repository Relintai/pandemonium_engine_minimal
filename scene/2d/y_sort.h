#ifndef Y_SORT_H
#define Y_SORT_H

/*  y_sort.h                                                             */


#include "scene/main/node_2d.h"

class YSort : public Node2D {
	GDCLASS(YSort, Node2D);
	bool sort_enabled;
	static void _bind_methods();

public:
	void set_sort_enabled(bool p_enabled);
	bool is_sort_enabled() const;
	YSort();
};

#endif // Y_SORT_H

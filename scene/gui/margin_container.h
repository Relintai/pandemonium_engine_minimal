#ifndef MARGIN_CONTAINER_H
#define MARGIN_CONTAINER_H

/*  margin_container.h                                                   */


#include "scene/gui/container.h"

class MarginContainer : public Container {
	GDCLASS(MarginContainer, Container);

protected:
	void _notification(int p_what);

public:
	virtual Size2 get_minimum_size() const;

	MarginContainer();
};

#endif // MARGIN_CONTAINER_H

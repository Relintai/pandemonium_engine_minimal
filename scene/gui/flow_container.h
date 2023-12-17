#ifndef FLOW_CONTAINER_H
#define FLOW_CONTAINER_H

/*  flow_container.h                                                     */


#include "scene/gui/container.h"

class FlowContainer : public Container {
	GDCLASS(FlowContainer, Container);

private:
	int cached_size;
	int cached_line_count;

	bool vertical;

	void _resort();

protected:
	void _notification(int p_what);

	static void _bind_methods();

public:
	int get_line_count() const;

	virtual Size2 get_minimum_size() const;

	FlowContainer(bool p_vertical = false);
};

class HFlowContainer : public FlowContainer {
	GDCLASS(HFlowContainer, FlowContainer);

public:
	HFlowContainer() :
			FlowContainer(false) {}
};

class VFlowContainer : public FlowContainer {
	GDCLASS(VFlowContainer, FlowContainer);

public:
	VFlowContainer() :
			FlowContainer(true) {}
};

#endif // FLOW_CONTAINER_H

#ifndef PANEL_H
#define PANEL_H

/*  panel.h                                                              */


#include "scene/main/control.h"

class Panel : public Control {
	GDCLASS(Panel, Control);

protected:
	void _notification(int p_what);

public:
	Panel();
	~Panel();
};

#endif

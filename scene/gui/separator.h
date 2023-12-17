#ifndef SEPARATOR_H
#define SEPARATOR_H

/*  separator.h                                                          */


#include "scene/main/control.h"
class Separator : public Control {
	GDCLASS(Separator, Control);

protected:
	Orientation orientation;
	void _notification(int p_what);

public:
	virtual Size2 get_minimum_size() const;

	Separator();
	~Separator();
};

class VSeparator : public Separator {
	GDCLASS(VSeparator, Separator);

public:
	VSeparator();
};

class HSeparator : public Separator {
	GDCLASS(HSeparator, Separator);

public:
	HSeparator();
};

#endif

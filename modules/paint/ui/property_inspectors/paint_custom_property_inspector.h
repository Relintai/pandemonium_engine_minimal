#ifndef PAINT_CUSTOM_PROPERTY_INSPECTOR_H
#define PAINT_CUSTOM_PROPERTY_INSPECTOR_H

/*
Copyright (c) 2022-2023 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "scene/gui/panel_container.h"

class PaintNode;

class PaintCustomPropertyInspector : public PanelContainer {
	GDCLASS(PaintCustomPropertyInspector, PanelContainer);

public:
	void set_paint_node(PaintNode *paint_node);
	void set_paint_node_bind(Node *paint_node);
	
	virtual void _set_paint_node(Node *paint_node);

	PaintCustomPropertyInspector();
	~PaintCustomPropertyInspector();

protected:
	static void _bind_methods();
};

#endif

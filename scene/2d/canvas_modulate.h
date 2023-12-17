#ifndef CANVASMODULATE_H
#define CANVASMODULATE_H

/*  canvas_modulate.h                                                    */


#include "scene/main/node_2d.h"

class CanvasModulate : public Node2D {
	GDCLASS(CanvasModulate, Node2D);

	Color color;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_color(const Color &p_color);
	Color get_color() const;

	String get_configuration_warning() const;

	CanvasModulate();
	~CanvasModulate();
};

#endif // CANVASMODULATE_H

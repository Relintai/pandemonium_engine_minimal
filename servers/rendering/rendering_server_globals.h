#ifndef RENDERING_SERVER_GLOBALS_H
#define RENDERING_SERVER_GLOBALS_H

/*  rendering_server_globals.h                                              */


#include "rasterizer.h"

class RenderingServerCanvas;
class RenderingServerViewport;

class RenderingServerGlobals {
public:
	static RasterizerStorage *storage;
	static RasterizerCanvas *canvas_render;
	static Rasterizer *rasterizer;

	static RenderingServerCanvas *canvas;
	static RenderingServerViewport *viewport;
};

#define RSG RenderingServerGlobals

#endif // RENDERING_SERVER_GLOBALS_H

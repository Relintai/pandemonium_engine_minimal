#ifndef WORLD_2D_H
#define WORLD_2D_H

/*  world_2d.h                                                           */


#include "core/config/project_settings.h"
#include "core/object/resource.h"
#include "servers/physics_2d_server.h"

class VisibilityNotifier2D;
class Viewport;
class World;
struct SpatialIndexer2D;

class World2D : public Resource {
	GDCLASS(World2D, Resource);

	RID canvas;
	RID space;
	RID navigation_map;

	SpatialIndexer2D *indexer;

protected:
	static void _bind_methods();
	friend class Viewport;
	friend class World;
	friend class VisibilityNotifier2D;

	void _register_world(World *p_world, const Rect2 &p_rect);
	void _update_world(World *p_world, const Rect2 &p_rect);
	void _remove_world(World *p_world);

	void _register_notifier(VisibilityNotifier2D *p_notifier, const Rect2 &p_rect);
	void _update_notifier(VisibilityNotifier2D *p_notifier, const Rect2 &p_rect);
	void _remove_notifier(VisibilityNotifier2D *p_notifier);

	void _update();

public:
	RID get_canvas();
	RID get_space();

	Physics2DDirectSpaceState *get_direct_space_state();

	void get_world_list(List<World *> *r_worlds);
	void get_viewport_list(List<Viewport *> *r_worlds);

	World2D();
	~World2D();
};

#endif // WORLD_2D_H

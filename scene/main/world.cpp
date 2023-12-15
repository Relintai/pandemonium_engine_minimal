#include "world.h"

#include "core/config/engine.h"
#include "core/core_string_names.h"
#include "scene/resources/world_2d.h"
#include "viewport.h"

Ref<World2D> World::get_world_2d() const {
	return world_2d;
}

void World::set_world_2d(const Ref<World2D> &p_world_2d) {
	if (world_2d == p_world_2d) {
		return;
	}

	if (_parent_world && _parent_world->find_world_2d() == p_world_2d) {
		WARN_PRINT("Unable to use parent world as world_2d");
		return;
	}

	Ref<World2D> old_world;

	if (is_inside_tree()) {
		old_world = find_world_2d();
	}

	if (p_world_2d.is_valid()) {
		world_2d = p_world_2d;
	} else {
		WARN_PRINT("Invalid world");
		world_2d = Ref<World2D>(memnew(World2D));
	}

	_on_set_world_2d(old_world);
}

bool World::get_override_in_parent_viewport() {
	return _override_in_parent_viewport;
}
void World::set_override_in_parent_viewport(const bool value) {
	if (_override_in_parent_viewport == value) {
		return;
	}

	if (!Engine::get_singleton()->is_editor_hint() && is_inside_tree()) {
		World *w = get_viewport();

		if (w) {
			if (_override_in_parent_viewport) {
				if (w->get_override_world() == this) {
					w->set_override_world(NULL);
				}
			} else {
				w->set_override_world(this);
			}
		}
	}

	_override_in_parent_viewport = value;
}

Ref<World2D> World::find_world_2d() const {
	if (_override_world) {
		return _override_world->find_world_2d();
	}

	if (world_2d.is_valid()) {
		return world_2d;
	} else if (_parent_world) {
		return _parent_world->find_world_2d();
	} else {
		return Ref<World2D>();
	}
}

Ref<World2D> World::find_world_2d_no_override() const {
	if (world_2d.is_valid()) {
		return world_2d;
	} else if (_parent_world) {
		return _parent_world->find_world_2d();
	} else {
		return Ref<World2D>();
	}
}

World *World::get_override_world() {
	return _override_world;
}
World *World::get_override_world_or_this() {
	if (!_override_world) {
		return this;
	}

	return _override_world;
}
void World::set_override_world(World *p_world) {
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	if (p_world == _override_world || p_world == this) {
		return;
	}

	_on_before_world_override_changed();

	if (_overriding_world) {
		_overriding_world->set_override_world(NULL);
		_overriding_world = NULL;
	}

	_override_world = p_world;

	if (_override_world) {
		_override_world->_overriding_world = this;
	}

	_on_after_world_override_changed();
}
void World::set_override_world_bind(Node *p_world) {
	World *w = Object::cast_to<World>(p_world);

	ERR_FAIL_COND(p_world && !w);

	set_override_world(w);
}

void World::gui_reset_canvas_sort_index() {
	if (_overriding_world) {
		_overriding_world->gui_reset_canvas_sort_index();
	}
}
int World::gui_get_canvas_sort_index() {
	if (_overriding_world) {
		return _overriding_world->gui_get_canvas_sort_index();
	}

	return 0;
}
void World::enable_canvas_transform_override(bool p_enable) {
	if (_overriding_world) {
		_overriding_world->enable_canvas_transform_override(p_enable);
	}
}
bool World::is_canvas_transform_override_enbled() const {
	if (_overriding_world) {
		return _overriding_world->is_canvas_transform_override_enbled();
	}

	return false;
}
void World::set_canvas_transform_override(const Transform2D &p_transform) {
	if (_overriding_world) {
		_overriding_world->set_canvas_transform_override(p_transform);
	}
}
Transform2D World::get_canvas_transform_override() const {
	if (_overriding_world) {
		return _overriding_world->get_canvas_transform_override();
	}

	return Transform2D();
}

void World::set_canvas_transform(const Transform2D &p_transform) {
	if (_overriding_world) {
		_overriding_world->set_canvas_transform(p_transform);
	}
}

Transform2D World::get_canvas_transform() const {
	if (_overriding_world) {
		return _overriding_world->get_canvas_transform();
	}

	return Transform2D();
}

void World::set_global_canvas_transform(const Transform2D &p_transform) {
	if (_overriding_world) {
		_overriding_world->set_global_canvas_transform(p_transform);
	}
}

Transform2D World::get_global_canvas_transform() const {
	if (_overriding_world) {
		return _overriding_world->get_global_canvas_transform();
	}

	return Transform2D();
}

Transform2D World::get_final_transform() const {
	if (_overriding_world) {
		return _overriding_world->get_final_transform();
	}

	return Transform2D();
}

Rect2 World::get_visible_rect() const {
	if (_overriding_world) {
		return _overriding_world->get_visible_rect();
	}

	return Rect2();
}

RID World::get_viewport_rid() const {
	if (_overriding_world) {
		return _overriding_world->get_viewport_rid();
	}

	return RID();
}

Vector2 World::get_camera_coords(const Vector2 &p_viewport_coords) const {
	if (_overriding_world) {
		return _overriding_world->get_camera_coords(p_viewport_coords);
	}

	Transform2D xf = get_final_transform();
	return xf.xform(p_viewport_coords);
}

Vector2 World::get_camera_rect_size() const {
	if (_overriding_world) {
		return _overriding_world->get_camera_rect_size();
	}

	return size;
}

void World::update_worlds() {
	if (!is_inside_tree()) {
		return;
	}

	Rect2 abstracted_rect = Rect2(Vector2(), get_visible_rect().size);
	Rect2 xformed_rect = (get_global_canvas_transform() * get_canvas_transform()).affine_inverse().xform(abstracted_rect);
	find_world_2d()->_update_world(this, xformed_rect);
	find_world_2d()->_update();
}

World::World() {
	world_2d = Ref<World2D>(memnew(World2D));
	_override_world = NULL;
	_overriding_world = NULL;
	_override_in_parent_viewport = false;
}
World::~World() {
}

void World::_update_listener_2d() {}

void World::_propagate_enter_world(Node *p_node) {
	if (p_node != this) {
		if (!p_node->is_inside_tree()) { //may not have entered scene yet
			return;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_propagate_enter_world(p_node->get_child(i));
	}
}

void World::_propagate_exit_world(Node *p_node) {
	if (p_node != this) {
		if (!p_node->is_inside_tree()) { //may have exited scene already
			return;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		_propagate_exit_world(p_node->get_child(i));
	}
}

void World::_on_set_world_2d(const Ref<World2D> &p_old_world_2d) {
}

void World::_on_before_world_override_changed() {
}
void World::_on_after_world_override_changed() {
}

void World::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (get_parent()) {
				_parent_world = get_parent()->get_world();
			} else {
				_parent_world = nullptr;
			}
		} break;
		case NOTIFICATION_READY: {
			if (_override_in_parent_viewport && !Engine::get_singleton()->is_editor_hint()) {
				if (get_parent()) {
					World *w = get_parent()->get_viewport();

					if (w) {
						w->set_override_world(this);
					}
				}
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (!Engine::get_singleton()->is_editor_hint()) {
				set_override_world(NULL);
			}
		} break;
	}
}

void World::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_world_2d"), &World::get_world_2d);
	ClassDB::bind_method(D_METHOD("set_world_2d", "world_2d"), &World::set_world_2d);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "world_2d", PROPERTY_HINT_RESOURCE_TYPE, "World2D", 0), "set_world_2d", "get_world_2d");

	ClassDB::bind_method(D_METHOD("get_override_in_parent_viewport"), &World::get_override_in_parent_viewport);
	ClassDB::bind_method(D_METHOD("set_override_in_parent_viewport", "enable"), &World::set_override_in_parent_viewport);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "override_in_parent_viewport"), "set_override_in_parent_viewport", "get_override_in_parent_viewport");

	ClassDB::bind_method(D_METHOD("find_world_2d"), &World::find_world_2d);

	ClassDB::bind_method(D_METHOD("get_override_world"), &World::get_override_world);
	ClassDB::bind_method(D_METHOD("get_override_world_or_this"), &World::get_override_world_or_this);
	ClassDB::bind_method(D_METHOD("set_override_world", "world"), &World::set_override_world_bind);

	ClassDB::bind_method(D_METHOD("set_canvas_transform", "xform"), &World::set_canvas_transform);
	ClassDB::bind_method(D_METHOD("get_canvas_transform"), &World::get_canvas_transform);
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM2D, "canvas_transform", PROPERTY_HINT_NONE, "", 0), "set_canvas_transform", "get_canvas_transform");

	ClassDB::bind_method(D_METHOD("set_global_canvas_transform", "xform"), &World::set_global_canvas_transform);
	ClassDB::bind_method(D_METHOD("get_global_canvas_transform"), &World::get_global_canvas_transform);
	ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM2D, "global_canvas_transform", PROPERTY_HINT_NONE, "", 0), "set_global_canvas_transform", "get_global_canvas_transform");

	ClassDB::bind_method(D_METHOD("get_final_transform"), &World::get_final_transform);

	ClassDB::bind_method(D_METHOD("get_visible_rect"), &World::get_visible_rect);
	ClassDB::bind_method(D_METHOD("get_viewport_rid"), &World::get_viewport_rid);

	ClassDB::bind_method(D_METHOD("update_worlds"), &World::update_worlds);
}

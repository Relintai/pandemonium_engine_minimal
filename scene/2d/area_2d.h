#ifndef AREA_2D_H
#define AREA_2D_H

/*  area_2d.h                                                            */


#include "core/containers/vset.h"
#include "scene/2d/collision_object_2d.h"

class Area2D : public CollisionObject2D {
	GDCLASS(Area2D, CollisionObject2D);

public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

private:
	SpaceOverride space_override;
	Vector2 gravity_vec;
	real_t gravity;
	bool gravity_is_point;
	real_t gravity_distance_scale;
	real_t linear_damp;
	real_t angular_damp;
	int priority;
	bool monitoring;
	bool monitorable;
	bool locked;

	void _body_inout(int p_status, const RID &p_body, int p_instance, int p_body_shape, int p_area_shape);

	void _body_enter_tree(ObjectID p_id);
	void _body_exit_tree(ObjectID p_id);

	struct ShapePair {
		int body_shape;
		int area_shape;
		bool operator<(const ShapePair &p_sp) const {
			if (body_shape == p_sp.body_shape) {
				return area_shape < p_sp.area_shape;
			} else {
				return body_shape < p_sp.body_shape;
			}
		}

		ShapePair() {}
		ShapePair(int p_bs, int p_as) {
			body_shape = p_bs;
			area_shape = p_as;
		}
	};

	struct BodyState {
		RID rid;
		int rc;
		bool in_tree;
		VSet<ShapePair> shapes;
	};

	RBMap<ObjectID, BodyState> body_map;

	void _area_inout(int p_status, const RID &p_area, int p_instance, int p_area_shape, int p_self_shape);

	void _area_enter_tree(ObjectID p_id);
	void _area_exit_tree(ObjectID p_id);

	struct AreaShapePair {
		int area_shape;
		int self_shape;
		bool operator<(const AreaShapePair &p_sp) const {
			if (area_shape == p_sp.area_shape) {
				return self_shape < p_sp.self_shape;
			} else {
				return area_shape < p_sp.area_shape;
			}
		}

		AreaShapePair() {}
		AreaShapePair(int p_bs, int p_as) {
			area_shape = p_bs;
			self_shape = p_as;
		}
	};

	struct AreaState {
		RID rid;
		int rc;
		bool in_tree;
		VSet<AreaShapePair> shapes;
	};

	RBMap<ObjectID, AreaState> area_map;
	void _clear_monitoring();

	bool audio_bus_override;
	StringName audio_bus;

protected:
	void _notification(int p_what);
	static void _bind_methods();
	void _validate_property(PropertyInfo &property) const;

public:
	void set_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_distance_scale(real_t p_scale);
	real_t get_gravity_distance_scale() const;

	void set_gravity_vector(const Vector2 &p_vec);
	Vector2 get_gravity_vector() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_priority(real_t p_priority);
	real_t get_priority() const;

	void set_monitoring(bool p_enable);
	bool is_monitoring() const;

	void set_monitorable(bool p_enable);
	bool is_monitorable() const;

	Array get_overlapping_bodies() const; //function for script
	Array get_overlapping_areas() const; //function for script

	bool overlaps_area(Node *p_area) const;
	bool overlaps_body(Node *p_body) const;

	void set_audio_bus_override(bool p_override);
	bool is_overriding_audio_bus() const;

	void set_audio_bus_name(const StringName &p_audio_bus);
	StringName get_audio_bus_name() const;

	Area2D();
	~Area2D();
};

VARIANT_ENUM_CAST(Area2D::SpaceOverride);

#endif // AREA_2D_H

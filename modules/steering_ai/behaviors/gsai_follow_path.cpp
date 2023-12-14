
#include "gsai_follow_path.h"

#include "../gsai_path.h"
#include "../gsai_steering_agent.h"
#include "../gsai_target_acceleration.h"

Ref<GSAIPath> GSAIFollowPath::get_path() {
	return path;
}

void GSAIFollowPath::set_path(const Ref<GSAIPath> &val) {
	path = val;
}

float GSAIFollowPath::get_path_offset() const {
	return path_offset;
}

void GSAIFollowPath::set_path_offset(const float val) {
	path_offset = val;
}

bool GSAIFollowPath::get_is_arrive_enabled() const {
	return is_arrive_enabled;
}

void GSAIFollowPath::set_is_arrive_enabled(const bool val) {
	is_arrive_enabled = val;
}

float GSAIFollowPath::get_prediction_time() const {
	return prediction_time;
}

void GSAIFollowPath::set_prediction_time(const float val) {
	prediction_time = val;
}

void GSAIFollowPath::_calculate_steering(Ref<GSAITargetAcceleration> acceleration) {
	ERR_FAIL_COND(!agent.is_valid());
	ERR_FAIL_COND(!path.is_valid());

	Vector3 location;

	if (prediction_time == 0) {
		location = agent->get_position();
	} else {
		location = agent->get_position() + (agent->get_linear_velocity() * prediction_time);
	}

	float distance = path->calculate_distance(location);
	float target_distance = distance + path_offset;

	if (prediction_time > 0 && path->get_is_open()) {
		if (target_distance < path->calculate_distance(agent->get_position())) {
			target_distance = path->get_length();
		}
	}

	Vector3 target_position = path->calculate_target_position(target_distance);

	if (is_arrive_enabled && path->get_is_open()) {
		if (path_offset >= 0) {
			if (target_distance > path->get_length() - deceleration_radius) {
				arrive(acceleration, target_position);
				return;
			}
		} else {
			if (target_distance < deceleration_radius) {
				arrive(acceleration, target_position);
				return;
			}
		}
	}

	Vector3 linear = (target_position - agent->get_position()).normalized();
	linear *= agent->get_linear_acceleration_max();

	acceleration->set_linear(linear);
	acceleration->set_angular(0);
}

GSAIFollowPath::GSAIFollowPath() {
	path_offset = 0.0;
	is_arrive_enabled = true;
	prediction_time = 0.0;
}

GSAIFollowPath::~GSAIFollowPath() {
}

void GSAIFollowPath::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_path"), &GSAIFollowPath::get_path);
	ClassDB::bind_method(D_METHOD("set_path", "value"), &GSAIFollowPath::set_path);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "path", PROPERTY_HINT_RESOURCE_TYPE, "GSAIPath"), "set_path", "get_path");

	ClassDB::bind_method(D_METHOD("get_path_offset"), &GSAIFollowPath::get_path_offset);
	ClassDB::bind_method(D_METHOD("set_path_offset", "value"), &GSAIFollowPath::set_path_offset);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "path_offset"), "set_path_offset", "get_path_offset");

	ClassDB::bind_method(D_METHOD("get_is_arrive_enabled"), &GSAIFollowPath::get_is_arrive_enabled);
	ClassDB::bind_method(D_METHOD("set_is_arrive_enabled", "value"), &GSAIFollowPath::set_is_arrive_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_arrive_enabled"), "set_is_arrive_enabled", "get_is_arrive_enabled");

	ClassDB::bind_method(D_METHOD("get_prediction_time"), &GSAIFollowPath::get_prediction_time);
	ClassDB::bind_method(D_METHOD("set_prediction_time", "value"), &GSAIFollowPath::set_prediction_time);
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "prediction_time"), "set_prediction_time", "get_prediction_time");
}


#include "gsai_blend.h"

#include "../gsai_steering_agent.h"
#include "../gsai_target_acceleration.h"
#include "../gsai_utils.h"

void GSAIBlend::add_behavior(const Ref<GSAISteeringBehavior> &behavior, const float weight) {
	ERR_FAIL_COND(!behavior.is_valid());

	GSAIBlendBehaviorEntry e;
	e.behavior = behavior;
	e.weight = weight;
	_behaviors.push_back(e);
}

Ref<GSAISteeringBehavior> GSAIBlend::get_behavior(const int index) {
	ERR_FAIL_INDEX_V(index, _behaviors.size(), Ref<GSAISteeringBehavior>());

	return _behaviors[index].behavior;
}

float GSAIBlend::get_behavior_weight(const int index) {
	ERR_FAIL_INDEX_V(index, _behaviors.size(), 0);

	return _behaviors[index].weight;
}

void GSAIBlend::remove_behavior(const int index) {
	ERR_FAIL_INDEX(index, _behaviors.size());

	_behaviors.remove(index);
}

int GSAIBlend::get_behaviour_count() {
	return _behaviors.size();
}

Ref<GSAITargetAcceleration> GSAIBlend::get_accel() {
	return _accel;
}

void GSAIBlend::_calculate_steering(Ref<GSAITargetAcceleration> blended_accel) {
	ERR_FAIL_COND(!agent.is_valid());

	blended_accel->set_zero();

	for (int i = 0; i < _behaviors.size(); ++i) { //i in range(_behaviors.size())
		GSAIBlendBehaviorEntry bw = _behaviors[i];

		bw.behavior->calculate_steering(_accel);
		blended_accel->add_scaled_accel(_accel, bw.weight);
	}

	blended_accel->set_linear(GSAIUtils::clampedv3(blended_accel->get_linear(), agent->get_linear_acceleration_max()));
	float angular_acceleration_max = agent->get_angular_acceleration_max();
	blended_accel->set_angular(CLAMP(blended_accel->get_angular(), -angular_acceleration_max, angular_acceleration_max));
}

GSAIBlend::GSAIBlend() {
	_accel.instance();
}

GSAIBlend::~GSAIBlend() {
}

void GSAIBlend::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_behavior", "behavior", "weight"), &GSAIBlend::add_behavior);
	ClassDB::bind_method(D_METHOD("get_behavior", "index"), &GSAIBlend::get_behavior);
	ClassDB::bind_method(D_METHOD("get_behavior_weight", "index"), &GSAIBlend::get_behavior_weight);
	ClassDB::bind_method(D_METHOD("remove_behavior", "index"), &GSAIBlend::remove_behavior);
	ClassDB::bind_method(D_METHOD("get_behaviour_count"), &GSAIBlend::get_behaviour_count);
	ClassDB::bind_method(D_METHOD("get_accel"), &GSAIBlend::get_accel);
}

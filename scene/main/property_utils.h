#ifndef PROPERTY_UTILS_H
#define PROPERTY_UTILS_H

/*  property_utils.h                                                     */


#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

class PropertyUtils {
public:
	static bool is_property_value_different(const Variant &p_a, const Variant &p_b);
	// Gets the most pure default value, the one that would be set when the node has just been instantiated
	static Variant get_property_default_value(const Object *p_object, const StringName &p_property, bool *r_is_valid = nullptr, const Vector<SceneState::PackState> *p_states_stack_cache = nullptr, bool p_update_exports = false, const Node *p_owner = nullptr, bool *r_is_class_default = nullptr);

	// Gets the instance/inheritance states of this node, in order of precedence,
	// that is, from the topmost (the most able to override values) to the lowermost
	// (Note that in nested instancing the one with the greatest precedence is the furthest
	// in the tree, since every owner found while traversing towards the root gets a chance
	// to override property values.)
	static Vector<SceneState::PackState> get_node_states_stack(const Node *p_node, const Node *p_owner = nullptr, bool *r_instanced_by_owner = nullptr);
};

#endif // PROPERTY_UTILS_H

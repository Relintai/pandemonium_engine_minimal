#ifndef NAVIGATION_AGENT_H
#define NAVIGATION_AGENT_H

/*************************************************************************/
/*  navigation_agent.h                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "core/containers/vector.h"

#include "scene/main/node.h"
#include "servers/navigation/navigation_path_query_parameters_3d.h"

class Spatial;
class Navigation;
class SpatialMaterial;
class NavigationPathQueryParameters3D;
class NavigationPathQueryResult3D;

class NavigationAgent : public Node {
	GDCLASS(NavigationAgent, Node);

	Spatial *agent_parent;
	Navigation *navigation;

	RID agent;
	RID map_override;

	bool avoidance_enabled;
	bool use_3d_avoidance;
	uint32_t avoidance_layers;
	uint32_t avoidance_mask;
	real_t avoidance_priority;
	uint32_t navigation_layers;
	NavigationPathQueryParameters3D::PathfindingAlgorithm pathfinding_algorithm;
	NavigationPathQueryParameters3D::PathPostProcessing path_postprocessing;
	int path_metadata_flags;

	real_t path_desired_distance;
	real_t target_desired_distance;
	real_t height;
	real_t radius;
	real_t path_height_offset;
	real_t neighbor_distance;
	int max_neighbors;
	real_t time_horizon_agents;
	real_t time_horizon_obstacles;
	real_t max_speed;
	real_t path_max_distance;

	Vector3 target_position;
	bool target_position_submitted;

	Ref<NavigationPathQueryParameters3D> navigation_query;
	Ref<NavigationPathQueryResult3D> navigation_result;
	int nav_path_index;

	// the velocity result of the avoidance simulation step
	Vector3 safe_velocity;

	/// The submitted target velocity, sets the "wanted" rvo agent velocity on the next update
	// this velocity is not guaranteed, the simulation will try to fulfil it if possible
	// if other agents or obstacles interfere it will be changed accordingly
	Vector3 velocity;

	bool velocity_submitted;

	/// The submitted forced velocity, overrides the rvo agent velocity on the next update
	// should only be used very intentionally and not every frame as it interferes with the simulation stability
	Vector3 velocity_forced;
	bool velocity_forced_submitted;

	// 2D avoidance has no y-axis. This stores and reapplies the y-axis velocity to the agent before and after the avoidance step.
	// While not perfect it at least looks way better than agent's that clip through everything that is not a flat surface
	float stored_y_velocity;

	bool target_reached;
	bool navigation_finished;
	// No initialized on purpose
	uint32_t update_frame_id;

#ifdef DEBUG_ENABLED
	bool debug_enabled;
	bool debug_path_dirty;
	RID debug_path_instance;
	Ref<ArrayMesh> debug_path_mesh;
	float debug_path_custom_point_size;
	bool debug_use_custom;
	Color debug_path_custom_color;
	Ref<SpatialMaterial> debug_agent_path_line_custom_material;
	Ref<SpatialMaterial> debug_agent_path_point_custom_material;
#endif // DEBUG_ENABLED

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	NavigationAgent();
	virtual ~NavigationAgent();

	void set_navigation(Navigation *p_nav);
	const Navigation *get_navigation() const {
		return navigation;
	}

	void set_navigation_node(Node *p_nav);
	Node *get_navigation_node() const;

	RID get_rid() const {
		return agent;
	}

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_agent_parent(Node *p_agent_parent);

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_pathfinding_algorithm(const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm);
	NavigationPathQueryParameters3D::PathfindingAlgorithm get_pathfinding_algorithm() const {
		return pathfinding_algorithm;
	}

	void set_path_postprocessing(const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing);
	NavigationPathQueryParameters3D::PathPostProcessing get_path_postprocessing() const {
		return path_postprocessing;
	}

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_avoidance_mask_value(int p_mask_number, bool p_value);
	bool get_avoidance_mask_value(int p_mask_number) const;

	void set_avoidance_priority(real_t p_priority);
	real_t get_avoidance_priority() const;

	void set_path_metadata_flags(const int p_flags);
	int get_path_metadata_flags() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_path_desired_distance(real_t p_dd);
	real_t get_path_desired_distance() const {
		return path_desired_distance;
	}

	void set_target_desired_distance(real_t p_dd);
	real_t get_target_desired_distance() const {
		return target_desired_distance;
	}

	void set_radius(real_t p_radius);
	real_t get_radius() const {
		return radius;
	}

	void set_height(real_t p_height);
	real_t get_height() const { return height; }

	void set_path_height_offset(real_t p_path_height_offset);
	real_t get_path_height_offset() const { return path_height_offset; }

	void set_use_3d_avoidance(bool p_use_3d_avoidance);
	bool get_use_3d_avoidance() const { return use_3d_avoidance; }

	void set_neighbor_distance(real_t p_dist);
	real_t get_neighbor_distance() const {
		return neighbor_distance;
	}

	void set_max_neighbors(int p_count);
	int get_max_neighbors() const {
		return max_neighbors;
	}

	void set_time_horizon_agents(real_t p_time_horizon);
	real_t get_time_horizon_agents() const { return time_horizon_agents; }

	void set_time_horizon_obstacles(real_t p_time_horizon);
	real_t get_time_horizon_obstacles() const { return time_horizon_obstacles; }

	void set_max_speed(real_t p_max_speed);
	real_t get_max_speed() const {
		return max_speed;
	}

	void set_path_max_distance(real_t p_pmd);
	real_t get_path_max_distance();

	void set_target_position(Vector3 p_position);
	Vector3 get_target_position() const;

	Vector3 get_next_position();

	Ref<NavigationPathQueryResult3D> get_current_navigation_result() const;

	Vector<Vector3> get_current_navigation_path() const;

	int get_current_navigation_path_index() const {
		return nav_path_index;
	}

	real_t distance_to_target() const;
	bool is_target_reached() const;
	bool is_target_reachable();
	bool is_navigation_finished();
	Vector3 get_final_position();

	void set_velocity(const Vector3 p_velocity);
	Vector3 get_velocity() { return velocity; }

	void set_velocity_forced(const Vector3 p_velocity);

	void _avoidance_done(Vector3 p_new_velocity);

	virtual String get_configuration_warning() const;

#ifdef DEBUG_ENABLED
	void set_debug_enabled(bool p_enabled);
	bool get_debug_enabled() const;

	void set_debug_use_custom(bool p_enabled);
	bool get_debug_use_custom() const;

	void set_debug_path_custom_color(Color p_color);
	Color get_debug_path_custom_color() const;

	void set_debug_path_custom_point_size(float p_point_size);
	float get_debug_path_custom_point_size() const;
#endif // DEBUG_ENABLE

private:
	void update_navigation();
	void _request_repath();
	void _check_distance_to_target();

#ifdef DEBUG_ENABLED
	void _navigation_debug_changed();
	void _update_debug_path();
#endif // DEBUG_ENABLED
};

#endif

#ifndef SKELETON_MODIFICATION_3D_FABRIK_H
#define SKELETON_MODIFICATION_3D_FABRIK_H

/*************************************************************************/
/*  skeleton_modification_3d_fabrik.h                                    */
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

#include "core/containers/local_vector.h"
#include "skeleton_modification_3d.h"

class SkeletonModification3DFABRIK : public SkeletonModification3D {
	GDCLASS(SkeletonModification3DFABRIK, SkeletonModification3D);

private:
	struct FABRIK_Joint_Data {
		String bone_name;
		int bone_idx;
		real_t length;
		Vector3 magnet_position;

		bool auto_calculate_length;
		bool use_tip_node;
		NodePath tip_node;
		ObjectID tip_node_cache;

		bool use_target_basis;
		real_t roll;

		FABRIK_Joint_Data() {
			bone_idx = -1;
			length = -1;

			auto_calculate_length = true;
			use_tip_node = false;
			tip_node_cache = 0;

			use_target_basis = false;
			roll = 0;
		}
	};

	LocalVector<FABRIK_Joint_Data> fabrik_data_chain;
	LocalVector<Transform> fabrik_transforms;

	NodePath target_node;
	ObjectID target_node_cache;

	real_t chain_tolerance;
	int chain_max_iterations;
	int chain_iterations;

	void update_target_cache();
	void update_joint_tip_cache(int p_joint_idx);

	int final_joint_idx;
	Transform target_global_pose;
	Transform origin_global_pose;

	void chain_backwards();
	void chain_forwards();
	void chain_apply();

protected:
	static void _bind_methods();
	bool _get(const StringName &p_path, Variant &r_ret) const;
	bool _set(const StringName &p_path, const Variant &p_value);
	void _get_property_list(List<PropertyInfo> *p_list) const;

public:
	virtual void _execute(real_t p_delta);
	virtual void _setup_modification(Ref<SkeletonModificationStack3D> p_stack);

	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;

	int get_fabrik_data_chain_length();
	void set_fabrik_data_chain_length(int p_new_length);

	real_t get_chain_tolerance();
	void set_chain_tolerance(real_t p_tolerance);

	int get_chain_max_iterations();
	void set_chain_max_iterations(int p_iterations);

	String get_fabrik_joint_bone_name(int p_joint_idx) const;
	void set_fabrik_joint_bone_name(int p_joint_idx, String p_bone_name);
	int get_fabrik_joint_bone_index(int p_joint_idx) const;
	void set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	real_t get_fabrik_joint_length(int p_joint_idx) const;
	void set_fabrik_joint_length(int p_joint_idx, real_t p_bone_length);
	Vector3 get_fabrik_joint_magnet(int p_joint_idx) const;
	void set_fabrik_joint_magnet(int p_joint_idx, Vector3 p_magnet);
	bool get_fabrik_joint_auto_calculate_length(int p_joint_idx) const;
	void set_fabrik_joint_auto_calculate_length(int p_joint_idx, bool p_auto_calculate);
	void fabrik_joint_auto_calculate_length(int p_joint_idx);
	bool get_fabrik_joint_use_tip_node(int p_joint_idx) const;
	void set_fabrik_joint_use_tip_node(int p_joint_idx, bool p_use_tip_node);
	NodePath get_fabrik_joint_tip_node(int p_joint_idx) const;
	void set_fabrik_joint_tip_node(int p_joint_idx, NodePath p_tip_node);
	bool get_fabrik_joint_use_target_basis(int p_joint_idx) const;
	void set_fabrik_joint_use_target_basis(int p_joint_idx, bool p_use_basis);
	real_t get_fabrik_joint_roll(int p_joint_idx) const;
	void set_fabrik_joint_roll(int p_joint_idx, real_t p_roll);

	SkeletonModification3DFABRIK();
	~SkeletonModification3DFABRIK();
};

#endif // SKELETON_MODIFICATION_3D_FABRIK_H

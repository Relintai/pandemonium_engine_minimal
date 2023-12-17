#ifndef MULTIMESH_INSTANCE_2D_H
#define MULTIMESH_INSTANCE_2D_H

/*  multimesh_instance_2d.h                                              */


#include "scene/main/node_2d.h"

class MultiMesh;

class MultiMeshInstance2D : public Node2D {
	GDCLASS(MultiMeshInstance2D, Node2D);

	Ref<MultiMesh> multimesh;

	Ref<Texture> texture;
	Ref<Texture> normal_map;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_multimesh(const Ref<MultiMesh> &p_multimesh);
	Ref<MultiMesh> get_multimesh() const;

	void set_texture(const Ref<Texture> &p_texture);
	Ref<Texture> get_texture() const;

	void set_normal_map(const Ref<Texture> &p_texture);
	Ref<Texture> get_normal_map() const;

	MultiMeshInstance2D();
	~MultiMeshInstance2D();
};

#endif // MULTIMESH_INSTANCE_2D_H

#ifndef MESH_INSTANCE_2D_H
#define MESH_INSTANCE_2D_H

/*  mesh_instance_2d.h                                                   */


#include "scene/main/node_2d.h"

class MeshInstance2D : public Node2D {
	GDCLASS(MeshInstance2D, Node2D);

	Ref<Mesh> mesh;

	Ref<Texture> texture;
	Ref<Texture> normal_map;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_mesh(const Ref<Mesh> &p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_texture(const Ref<Texture> &p_texture);
	Ref<Texture> get_texture() const;

	void set_normal_map(const Ref<Texture> &p_texture);
	Ref<Texture> get_normal_map() const;

	MeshInstance2D();
};

#endif // MESH_INSTANCE_2D_H

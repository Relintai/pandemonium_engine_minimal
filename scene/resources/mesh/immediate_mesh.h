
/*  immediate_mesh.h                                                     */


#ifndef IMMEDIATE_MESH_H
#define IMMEDIATE_MESH_H

#include "core/containers/vector.h"
#include "scene/resources/mesh/mesh.h"

class ImmediateMesh : public Mesh {
	GDCLASS(ImmediateMesh, Mesh)

	RID mesh;

	bool uses_colors = false;
	bool uses_normals = false;
	bool uses_tangents = false;
	bool uses_uvs = false;
	bool uses_uv2s = false;

	Color current_color;
	Vector3 current_normal;
	Plane current_tangent;
	Vector2 current_uv;
	Vector2 current_uv2;

	Vector<Color> colors;
	Vector<Vector3> normals;
	Vector<Plane> tangents;
	Vector<Vector2> uvs;
	Vector<Vector2> uv2s;
	Vector<Vector3> vertices;

	struct Surface {
		PrimitiveType primitive;
		Ref<Material> material;
		bool vertex_2d = false;
		int array_len = 0;
		uint32_t format = 0;
		AABB aabb;
	};

	Vector<Surface> surfaces;

	bool surface_active = false;
	Surface active_surface_data;

	PoolVector<uint8_t> surface_array_create_cache;

	const Vector3 SMALL_VEC3 = Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON);

protected:
	static void _bind_methods();

public:
	void surface_begin(PrimitiveType p_primitive, const Ref<Material> &p_material = Ref<Material>());
	void surface_set_color(const Color &p_color);
	void surface_set_normal(const Vector3 &p_normal);
	void surface_set_tangent(const Plane &p_tangent);
	void surface_set_uv(const Vector2 &p_uv);
	void surface_set_uv2(const Vector2 &p_uv2);
	void surface_add_vertex(const Vector3 &p_vertex);
	void surface_add_vertex_2d(const Vector2 &p_vertex);
	void surface_end();

	void clear_surfaces();

	virtual int get_surface_count() const;
	virtual int surface_get_array_len(int p_idx) const;
	virtual int surface_get_array_index_len(int p_idx) const;
	virtual Array surface_get_arrays(int p_surface) const;
	virtual Array surface_get_blend_shape_arrays(int p_surface) const;
	virtual Dictionary surface_get_lods(int p_surface) const;
	virtual uint32_t surface_get_format(int p_idx) const;
	virtual PrimitiveType surface_get_primitive_type(int p_idx) const;
	virtual void surface_set_material(int p_idx, const Ref<Material> &p_material);
	virtual Ref<Material> surface_get_material(int p_idx) const;
	virtual int get_blend_shape_count() const;
	virtual StringName get_blend_shape_name(int p_index) const;
	virtual void set_blend_shape_name(int p_index, const StringName &p_name);

	virtual AABB get_aabb() const;

	virtual RID get_rid() const;

	ImmediateMesh();
	~ImmediateMesh();
};

#endif // IMMEDIATE_MESH_H


/*  importer_mesh.h                                                      */


#ifndef IMPORTER_MESH_H
#define IMPORTER_MESH_H

#include "core/object/resource.h"
#include "core/containers/local_vector.h"
#include "scene/resources/mesh/mesh.h"

#include <cstdint>

// The following classes are used by importers instead of ArrayMesh and MeshInstance3D
// so the data is not registered (hence, quality loss), importing happens faster and
// its easier to modify before saving

class ImporterMesh : public Resource {
	GDCLASS(ImporterMesh, Resource)

	struct Surface {
		Mesh::PrimitiveType primitive;
		Array arrays;
		
		struct BlendShape {
			Array arrays;
		};

		Vector<BlendShape> blend_shape_data;

		struct LOD {
			Vector<int> indices;
			float distance = 0.0f;
		};
		Vector<LOD> lods;
		Ref<Material> material;
		String name;
		uint32_t flags = 0;

		struct LODComparator {
			_FORCE_INLINE_ bool operator()(const LOD &l, const LOD &r) const {
				return l.distance < r.distance;
			}
		};

		void split_normals(const LocalVector<int> &p_indices, const LocalVector<Vector3> &p_normals);
		static void _split_normals(Array &r_arrays, const LocalVector<int> &p_indices, const LocalVector<Vector3> &p_normals);
	};
	Vector<Surface> surfaces;
	Vector<String> blend_shapes;
	Mesh::BlendShapeMode blend_shape_mode = Mesh::BLEND_SHAPE_MODE_NORMALIZED;

	Ref<ArrayMesh> mesh;

	Ref<ImporterMesh> shadow_mesh;

	Size2i lightmap_size_hint;

protected:
	void _set_data(const Dictionary &p_data);
	Dictionary _get_data() const;

	static void _bind_methods();

public:
	void add_blend_shape(const String &p_name);
	int get_blend_shape_count() const;
	String get_blend_shape_name(int p_blend_shape) const;

	void add_surface(Mesh::PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes = Array(), const Dictionary &p_lods = Dictionary(), const Ref<Material> &p_material = Ref<Material>(), const String &p_name = String(), const uint32_t p_flags = 0);
	int get_surface_count() const;

	void set_blend_shape_mode(Mesh::BlendShapeMode p_blend_shape_mode);
	Mesh::BlendShapeMode get_blend_shape_mode() const;

	Mesh::PrimitiveType get_surface_primitive_type(int p_surface);
	String get_surface_name(int p_surface) const;
	void set_surface_name(int p_surface, const String &p_name);
	Array get_surface_arrays(int p_surface) const;
	Array get_surface_blend_shape_arrays(int p_surface, int p_blend_shape) const;
	int get_surface_lod_count(int p_surface) const;
	Vector<int> get_surface_lod_indices(int p_surface, int p_lod) const;
	float get_surface_lod_size(int p_surface, int p_lod) const;
	Ref<Material> get_surface_material(int p_surface) const;
	uint32_t get_surface_format(int p_surface) const;

	void set_surface_material(int p_surface, const Ref<Material> &p_material);

	Vector<Face3> get_faces() const;

	bool has_mesh() const;
	Ref<ArrayMesh> get_mesh(const Ref<ArrayMesh> &p_base = Ref<ArrayMesh>());
	void clear();
};

#endif // IMPORTER_MESH_H

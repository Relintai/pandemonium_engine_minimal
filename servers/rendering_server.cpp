
/*  rendering_server.cpp                                                    */


#include "rendering_server.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/object/method_bind_ext.gen.inc"

RenderingServer *RenderingServer::singleton = nullptr;
RenderingServer *(*RenderingServer::create_func)() = nullptr;

RenderingServer *RenderingServer::get_singleton() {
	return singleton;
}

RenderingServer *RenderingServer::create() {
	ERR_FAIL_COND_V(singleton, nullptr);

	if (create_func) {
		return create_func();
	}

	return nullptr;
}

RID RenderingServer::texture_create_from_image(const Ref<Image> &p_image, uint32_t p_flags) {
	ERR_FAIL_COND_V(!p_image.is_valid(), RID());
	RID texture = texture_create();
	texture_allocate(texture, p_image->get_width(), p_image->get_height(), 0, p_image->get_format(), RS::TEXTURE_TYPE_2D, p_flags); //if it has mipmaps, use, else generate
	ERR_FAIL_COND_V(!texture.is_valid(), texture);

	texture_set_data(texture, p_image);

	return texture;
}

Array RenderingServer::_texture_debug_usage_bind() {
	List<TextureInfo> list;
	texture_debug_usage(&list);
	Array arr;
	for (const List<TextureInfo>::Element *E = list.front(); E; E = E->next()) {
		Dictionary dict;
		dict["texture"] = E->get().texture;
		dict["width"] = E->get().width;
		dict["height"] = E->get().height;
		dict["depth"] = E->get().depth;
		dict["format"] = E->get().format;
		dict["bytes"] = E->get().bytes;
		dict["path"] = E->get().path;
		arr.push_back(dict);
	}
	return arr;
}

Array RenderingServer::_shader_get_param_list_bind(RID p_shader) const {
	List<PropertyInfo> l;
	shader_get_param_list(p_shader, &l);
	return convert_property_list(&l);
}

RID RenderingServer::get_test_texture() {
	if (test_texture.is_valid()) {
		return test_texture;
	};

#define TEST_TEXTURE_SIZE 256

	PoolVector<uint8_t> test_data;
	test_data.resize(TEST_TEXTURE_SIZE * TEST_TEXTURE_SIZE * 3);

	{
		PoolVector<uint8_t>::Write w = test_data.write();

		for (int x = 0; x < TEST_TEXTURE_SIZE; x++) {
			for (int y = 0; y < TEST_TEXTURE_SIZE; y++) {
				Color c;
				int r = 255 - (x + y) / 2;

				if ((x % (TEST_TEXTURE_SIZE / 8)) < 2 || (y % (TEST_TEXTURE_SIZE / 8)) < 2) {
					c.r = y;
					c.g = r;
					c.b = x;

				} else {
					c.r = r;
					c.g = x;
					c.b = y;
				}

				w[(y * TEST_TEXTURE_SIZE + x) * 3 + 0] = uint8_t(CLAMP(c.r * 255, 0, 255));
				w[(y * TEST_TEXTURE_SIZE + x) * 3 + 1] = uint8_t(CLAMP(c.g * 255, 0, 255));
				w[(y * TEST_TEXTURE_SIZE + x) * 3 + 2] = uint8_t(CLAMP(c.b * 255, 0, 255));
			}
		}
	}

	Ref<Image> data = memnew(Image(TEST_TEXTURE_SIZE, TEST_TEXTURE_SIZE, false, Image::FORMAT_RGB8, test_data));

	test_texture = RID_PRIME(texture_create_from_image(data));

	return test_texture;
}

void RenderingServer::_free_internal_rids() {
	if (test_texture.is_valid()) {
		free(test_texture);
	}
	if (white_texture.is_valid()) {
		free(white_texture);
	}
	if (test_material.is_valid()) {
		free(test_material);
	}
}

RID RenderingServer::_make_test_cube() {
	PoolVector<Vector3> vertices;
	PoolVector<Vector3> normals;
	PoolVector<float> tangents;
	PoolVector<Vector3> uvs;

#define ADD_VTX(m_idx)                           \
	vertices.push_back(face_points[m_idx]);      \
	normals.push_back(normal_points[m_idx]);     \
	tangents.push_back(normal_points[m_idx][1]); \
	tangents.push_back(normal_points[m_idx][2]); \
	tangents.push_back(normal_points[m_idx][0]); \
	tangents.push_back(1.0);                     \
	uvs.push_back(Vector3(uv_points[m_idx * 2 + 0], uv_points[m_idx * 2 + 1], 0));

	for (int i = 0; i < 6; i++) {
		Vector3 face_points[4];
		Vector3 normal_points[4];
		float uv_points[8] = { 0, 0, 0, 1, 1, 1, 1, 0 };

		for (int j = 0; j < 4; j++) {
			float v[3];
			v[0] = 1.0;
			v[1] = 1 - 2 * ((j >> 1) & 1);
			v[2] = v[1] * (1 - 2 * (j & 1));

			for (int k = 0; k < 3; k++) {
				if (i < 3) {
					face_points[j][(i + k) % 3] = v[k];
				} else {
					face_points[3 - j][(i + k) % 3] = -v[k];
				}
			}
			normal_points[j] = Vector3();
			normal_points[j][i % 3] = (i >= 3 ? -1 : 1);
		}

		//tri 1
		ADD_VTX(0);
		ADD_VTX(1);
		ADD_VTX(2);
		//tri 2
		ADD_VTX(2);
		ADD_VTX(3);
		ADD_VTX(0);
	}

	RID test_cube = mesh_create();

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[RenderingServer::ARRAY_NORMAL] = normals;
	d[RenderingServer::ARRAY_TANGENT] = tangents;
	d[RenderingServer::ARRAY_TEX_UV] = uvs;
	d[RenderingServer::ARRAY_VERTEX] = vertices;

	PoolVector<int> indices;
	indices.resize(vertices.size());
	for (int i = 0; i < vertices.size(); i++) {
		indices.set(i, i);
	}
	d[RenderingServer::ARRAY_INDEX] = indices;

	mesh_add_surface_from_arrays(test_cube, PRIMITIVE_TRIANGLES, d);

	/*
	test_material = fixed_material_create();
	//material_set_flag(material, MATERIAL_FLAG_BILLBOARD_TOGGLE,true);
	fixed_material_set_texture( test_material, FIXED_MATERIAL_PARAM_DIFFUSE, get_test_texture() );
	fixed_material_set_param( test_material, FIXED_MATERIAL_PARAM_SPECULAR_EXP, 70 );
	fixed_material_set_param( test_material, FIXED_MATERIAL_PARAM_EMISSION, Color(0.2,0.2,0.2) );

	fixed_material_set_param( test_material, FIXED_MATERIAL_PARAM_DIFFUSE, Color(1, 1, 1) );
	fixed_material_set_param( test_material, FIXED_MATERIAL_PARAM_SPECULAR, Color(1,1,1) );
*/
	mesh_surface_set_material(test_cube, 0, test_material);

	return test_cube;
}

RID RenderingServer::make_sphere_mesh(int p_lats, int p_lons, float p_radius) {
	PoolVector<Vector3> vertices;
	PoolVector<Vector3> normals;

	for (int i = 1; i <= p_lats; i++) {
		double lat0 = Math_PI * (-0.5 + (double)(i - 1) / p_lats);
		double z0 = Math::sin(lat0);
		double zr0 = Math::cos(lat0);

		double lat1 = Math_PI * (-0.5 + (double)i / p_lats);
		double z1 = Math::sin(lat1);
		double zr1 = Math::cos(lat1);

		for (int j = p_lons; j >= 1; j--) {
			double lng0 = 2 * Math_PI * (double)(j - 1) / p_lons;
			double x0 = Math::cos(lng0);
			double y0 = Math::sin(lng0);

			double lng1 = 2 * Math_PI * (double)(j) / p_lons;
			double x1 = Math::cos(lng1);
			double y1 = Math::sin(lng1);

			Vector3 v[4] = {
				Vector3(x1 * zr0, z0, y1 * zr0),
				Vector3(x1 * zr1, z1, y1 * zr1),
				Vector3(x0 * zr1, z1, y0 * zr1),
				Vector3(x0 * zr0, z0, y0 * zr0)
			};

#define ADD_POINT(m_idx)         \
	normals.push_back(v[m_idx]); \
	vertices.push_back(v[m_idx] * p_radius);

			ADD_POINT(0);
			ADD_POINT(1);
			ADD_POINT(2);

			ADD_POINT(2);
			ADD_POINT(3);
			ADD_POINT(0);
		}
	}

	RID mesh = mesh_create();
	Array d;
	d.resize(RS::ARRAY_MAX);

	d[ARRAY_VERTEX] = vertices;
	d[ARRAY_NORMAL] = normals;

	mesh_add_surface_from_arrays(mesh, PRIMITIVE_TRIANGLES, d);

	return mesh;
}

RID RenderingServer::get_white_texture() {
	if (white_texture.is_valid()) {
		return white_texture;
	}

	PoolVector<uint8_t> wt;
	wt.resize(16 * 3);
	{
		PoolVector<uint8_t>::Write w = wt.write();
		for (int i = 0; i < 16 * 3; i++) {
			w[i] = 255;
		}
	}
	Ref<Image> white = memnew(Image(4, 4, 0, Image::FORMAT_RGB8, wt));
	white_texture = RID_PRIME(texture_create());
	texture_allocate(white_texture, 4, 4, 0, Image::FORMAT_RGB8, TEXTURE_TYPE_2D);
	texture_set_data(white_texture, white);
	return white_texture;
}

#define SMALL_VEC2 Vector2(0.00001, 0.00001)
#define SMALL_VEC3 Vector3(0.00001, 0.00001, 0.00001)

// Maps normalized vector to an octahedron projected onto the cartesian plane
// Resulting 2D vector in range [-1, 1]
// See http://jcgt.org/published/0003/02/01/ for details
Vector2 RenderingServer::norm_to_oct(const Vector3 v) {
	const float L1Norm = Math::absf(v.x) + Math::absf(v.y) + Math::absf(v.z);

	// NOTE: this will mean it decompresses to 0,0,1
	// Discussed heavily here: https://github.com/godotengine/godot/pull/51268 as to why we did this
	if (Math::is_zero_approx(L1Norm)) {
		WARN_PRINT_ONCE("Octahedral compression cannot be used to compress a zero-length vector, please use normalized normal values or disable octahedral compression");
		return Vector2(0, 0);
	}

	const float invL1Norm = 1.0f / L1Norm;

	Vector2 res;
	if (v.z < 0.0f) {
		res.x = (1.0f - Math::absf(v.y * invL1Norm)) * SGN(v.x);
		res.y = (1.0f - Math::absf(v.x * invL1Norm)) * SGN(v.y);
	} else {
		res.x = v.x * invL1Norm;
		res.y = v.y * invL1Norm;
	}

	return res;
}

// Maps normalized tangent vector to an octahedron projected onto the cartesian plane
// Encodes the tangent vector sign in the second component of the returned Vector2 for use in shaders
// high_precision specifies whether the encoding will be 32 bit (true) or 16 bit (false)
// Resulting 2D vector in range [-1, 1]
// See http://jcgt.org/published/0003/02/01/ for details
Vector2 RenderingServer::tangent_to_oct(const Vector3 v, const float sign, const bool high_precision) {
	float bias = high_precision ? 1.0f / 32767 : 1.0f / 127;
	Vector2 res = norm_to_oct(v);
	res.y = res.y * 0.5f + 0.5f;
	res.y = MAX(res.y, bias) * SGN(sign);
	return res;
}

// Convert Octohedron-mapped normalized vector back to Cartesian
// Assumes normalized format (elements of v within range [-1, 1])
Vector3 RenderingServer::oct_to_norm(const Vector2 v) {
	Vector3 res(v.x, v.y, 1 - (Math::absf(v.x) + Math::absf(v.y)));
	float t = MAX(-res.z, 0.0f);
	res.x += t * -SGN(res.x);
	res.y += t * -SGN(res.y);
	return res.normalized();
}

// Convert Octohedron-mapped normalized tangent vector back to Cartesian
// out_sign provides the direction for the original cartesian tangent
// Assumes normalized format (elements of v within range [-1, 1])
Vector3 RenderingServer::oct_to_tangent(const Vector2 v, float *out_sign) {
	Vector2 v_decompressed = v;
	v_decompressed.y = Math::absf(v_decompressed.y) * 2 - 1;
	Vector3 res = oct_to_norm(v_decompressed);
	*out_sign = SGN(v[1]);
	return res;
}

Error RenderingServer::_surface_set_data(Array p_arrays, uint32_t p_format, uint32_t *p_offsets, uint32_t *p_stride, PoolVector<uint8_t> &r_vertex_array, int p_vertex_array_len, PoolVector<uint8_t> &r_index_array, int p_index_array_len, AABB &r_aabb, Vector<AABB> &r_bone_aabb) {
	PoolVector<uint8_t>::Write vw = r_vertex_array.write();

	PoolVector<uint8_t>::Write iw;
	if (r_index_array.size()) {
		iw = r_index_array.write();
	}

	for (int ai = 0; ai < RS::ARRAY_MAX; ai++) {
		if (!(p_format & (1 << ai))) { // no array
			continue;
		}

		switch (ai) {
			case RS::ARRAY_VERTEX: {
				if (p_format & RS::ARRAY_FLAG_USE_2D_VERTICES) {
					PoolVector<Vector2> array = p_arrays[ai];
					ERR_FAIL_COND_V(array.size() != p_vertex_array_len, ERR_INVALID_PARAMETER);

					PoolVector<Vector2>::Read read = array.read();
					const Vector2 *src = read.ptr();

					// setting vertices means regenerating the AABB
					Rect2 aabb;

					if (p_format & ARRAY_COMPRESS_VERTEX) {
						for (int i = 0; i < p_vertex_array_len; i++) {
							uint16_t vector[2] = { Math::make_half_float(src[i].x), Math::make_half_float(src[i].y) };

							memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], vector, sizeof(uint16_t) * 2);

							if (i == 0) {
								aabb = Rect2(src[i], SMALL_VEC2); //must have a bit of size
							} else {
								aabb.expand_to(src[i]);
							}
						}

					} else {
						for (int i = 0; i < p_vertex_array_len; i++) {
							float vector[2] = { src[i].x, src[i].y };

							memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], vector, sizeof(float) * 2);

							if (i == 0) {
								aabb = Rect2(src[i], SMALL_VEC2); //must have a bit of size
							} else {
								aabb.expand_to(src[i]);
							}
						}
					}

					r_aabb = AABB(Vector3(aabb.position.x, aabb.position.y, 0), Vector3(aabb.size.x, aabb.size.y, 0));

				} else {
					PoolVector<Vector3> array = p_arrays[ai];
					ERR_FAIL_COND_V(array.size() != p_vertex_array_len, ERR_INVALID_PARAMETER);

					PoolVector<Vector3>::Read read = array.read();
					const Vector3 *src = read.ptr();

					// setting vertices means regenerating the AABB
					AABB aabb;

					if (p_format & ARRAY_COMPRESS_VERTEX) {
						for (int i = 0; i < p_vertex_array_len; i++) {
							uint16_t vector[4] = { Math::make_half_float(src[i].x), Math::make_half_float(src[i].y), Math::make_half_float(src[i].z), Math::make_half_float(1.0) };

							memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], vector, sizeof(uint16_t) * 4);

							if (i == 0) {
								aabb = AABB(src[i], SMALL_VEC3);
							} else {
								aabb.expand_to(src[i]);
							}
						}

					} else {
						for (int i = 0; i < p_vertex_array_len; i++) {
							float vector[3] = { src[i].x, src[i].y, src[i].z };

							memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], vector, sizeof(float) * 3);

							if (i == 0) {
								aabb = AABB(src[i], SMALL_VEC3);
							} else {
								aabb.expand_to(src[i]);
							}
						}
					}

					r_aabb = aabb;
				}

			} break;
			case RS::ARRAY_NORMAL: {
				ERR_FAIL_COND_V(p_arrays[ai].get_type() != Variant::POOL_VECTOR3_ARRAY, ERR_INVALID_PARAMETER);

				PoolVector<Vector3> array = p_arrays[ai];
				ERR_FAIL_COND_V(array.size() != p_vertex_array_len, ERR_INVALID_PARAMETER);

				PoolVector<Vector3>::Read read = array.read();
				const Vector3 *src = read.ptr();

				// setting vertices means regenerating the AABB

				if (p_format & ARRAY_COMPRESS_NORMAL) {
					for (int i = 0; i < p_vertex_array_len; i++) {
						int8_t vector[4] = {
							(int8_t)CLAMP(src[i].x * 127, -128, 127),
							(int8_t)CLAMP(src[i].y * 127, -128, 127),
							(int8_t)CLAMP(src[i].z * 127, -128, 127),
							0,
						};

						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], vector, 4);
					}

				} else {
					for (int i = 0; i < p_vertex_array_len; i++) {
						float vector[3] = { src[i].x, src[i].y, src[i].z };
						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], vector, 3 * 4);
					}
				}

			} break;

			case RS::ARRAY_TANGENT: {
				ERR_FAIL_COND_V(p_arrays[ai].get_type() != Variant::POOL_REAL_ARRAY, ERR_INVALID_PARAMETER);

				PoolVector<real_t> array = p_arrays[ai];

				ERR_FAIL_COND_V(array.size() != p_vertex_array_len * 4, ERR_INVALID_PARAMETER);

				PoolVector<real_t>::Read read = array.read();
				const real_t *src = read.ptr();

				if (p_format & ARRAY_COMPRESS_TANGENT) {
					for (int i = 0; i < p_vertex_array_len; i++) {
						int8_t xyzw[4] = {
							(int8_t)CLAMP(src[i * 4 + 0] * 127, -128, 127),
							(int8_t)CLAMP(src[i * 4 + 1] * 127, -128, 127),
							(int8_t)CLAMP(src[i * 4 + 2] * 127, -128, 127),
							(int8_t)CLAMP(src[i * 4 + 3] * 127, -128, 127)
						};

						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], xyzw, 4);
					}

				} else {
					for (int i = 0; i < p_vertex_array_len; i++) {
						float xyzw[4] = {
							src[i * 4 + 0],
							src[i * 4 + 1],
							src[i * 4 + 2],
							src[i * 4 + 3]
						};

						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], xyzw, 4 * 4);
					}
				}

			} break;
			case RS::ARRAY_COLOR: {
				ERR_FAIL_COND_V(p_arrays[ai].get_type() != Variant::POOL_COLOR_ARRAY, ERR_INVALID_PARAMETER);

				PoolVector<Color> array = p_arrays[ai];

				ERR_FAIL_COND_V(array.size() != p_vertex_array_len, ERR_INVALID_PARAMETER);

				PoolVector<Color>::Read read = array.read();
				const Color *src = read.ptr();

				if (p_format & ARRAY_COMPRESS_COLOR) {
					for (int i = 0; i < p_vertex_array_len; i++) {
						uint8_t colors[4];

						for (int j = 0; j < 4; j++) {
							colors[j] = CLAMP(int((src[i][j]) * 255.0), 0, 255);
						}

						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], colors, 4);
					}
				} else {
					for (int i = 0; i < p_vertex_array_len; i++) {
						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], &src[i], 4 * 4);
					}
				}

			} break;
			case RS::ARRAY_TEX_UV: {
				ERR_FAIL_COND_V(p_arrays[ai].get_type() != Variant::POOL_VECTOR3_ARRAY && p_arrays[ai].get_type() != Variant::POOL_VECTOR2_ARRAY, ERR_INVALID_PARAMETER);

				PoolVector<Vector2> array = p_arrays[ai];

				ERR_FAIL_COND_V(array.size() != p_vertex_array_len, ERR_INVALID_PARAMETER);

				PoolVector<Vector2>::Read read = array.read();

				const Vector2 *src = read.ptr();

				if (p_format & ARRAY_COMPRESS_TEX_UV) {
					for (int i = 0; i < p_vertex_array_len; i++) {
						uint16_t uv[2] = { Math::make_half_float(src[i].x), Math::make_half_float(src[i].y) };
						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], uv, 2 * 2);
					}

				} else {
					for (int i = 0; i < p_vertex_array_len; i++) {
						float uv[2] = { src[i].x, src[i].y };

						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], uv, 2 * 4);
					}
				}

			} break;

			case RS::ARRAY_TEX_UV2: {
				ERR_FAIL_COND_V(p_arrays[ai].get_type() != Variant::POOL_VECTOR3_ARRAY && p_arrays[ai].get_type() != Variant::POOL_VECTOR2_ARRAY, ERR_INVALID_PARAMETER);

				PoolVector<Vector2> array = p_arrays[ai];

				ERR_FAIL_COND_V(array.size() != p_vertex_array_len, ERR_INVALID_PARAMETER);

				PoolVector<Vector2>::Read read = array.read();

				const Vector2 *src = read.ptr();

				if (p_format & ARRAY_COMPRESS_TEX_UV2) {
					for (int i = 0; i < p_vertex_array_len; i++) {
						uint16_t uv[2] = { Math::make_half_float(src[i].x), Math::make_half_float(src[i].y) };
						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], uv, 2 * 2);
					}

				} else {
					for (int i = 0; i < p_vertex_array_len; i++) {
						float uv[2] = { src[i].x, src[i].y };

						memcpy(&vw[p_offsets[ai] + i * p_stride[ai]], uv, 2 * 4);
					}
				}
			} break;
			case RS::ARRAY_INDEX: {
				ERR_FAIL_COND_V(p_index_array_len <= 0, ERR_INVALID_DATA);
				ERR_FAIL_COND_V(p_arrays[ai].get_type() != Variant::POOL_INT_ARRAY, ERR_INVALID_PARAMETER);

				PoolVector<int> indices = p_arrays[ai];
				ERR_FAIL_COND_V(indices.size() == 0, ERR_INVALID_PARAMETER);
				ERR_FAIL_COND_V(indices.size() != p_index_array_len, ERR_INVALID_PARAMETER);

				/* determine whether using 16 or 32 bits indices */

				PoolVector<int>::Read read = indices.read();
				const int *src = read.ptr();

				for (int i = 0; i < p_index_array_len; i++) {
					if (p_vertex_array_len < (1 << 16)) {
						uint16_t v = src[i];

						memcpy(&iw[i * 2], &v, 2);
					} else {
						uint32_t v = src[i];

						memcpy(&iw[i * 4], &v, 4);
					}
				}
			} break;
			default: {
				ERR_FAIL_V(ERR_INVALID_DATA);
			}
		}
	}

	return OK;
}

uint32_t RenderingServer::mesh_surface_get_format_offset(uint32_t p_format, int p_vertex_len, int p_index_len, int p_array_index) const {
	ERR_FAIL_INDEX_V(p_array_index, ARRAY_MAX, 0);
	uint32_t offsets[ARRAY_MAX];
	uint32_t strides[ARRAY_MAX];
	mesh_surface_make_offsets_from_format(p_format, p_vertex_len, p_index_len, offsets, strides);
	return offsets[p_array_index];
}

uint32_t RenderingServer::mesh_surface_get_format_stride(uint32_t p_format, int p_vertex_len, int p_index_len, int p_array_index) const {
	ERR_FAIL_INDEX_V(p_array_index, ARRAY_MAX, 0);
	uint32_t offsets[ARRAY_MAX];
	uint32_t strides[ARRAY_MAX];
	mesh_surface_make_offsets_from_format(p_format, p_vertex_len, p_index_len, offsets, strides);
	return strides[p_array_index];
}

void RenderingServer::mesh_surface_make_offsets_from_format(uint32_t p_format, int p_vertex_len, int p_index_len, uint32_t *r_offsets, uint32_t *r_strides) const {
	bool use_split_stream = GLOBAL_GET("rendering/misc/mesh_storage/split_stream") && !(p_format & RS::ARRAY_FLAG_USE_DYNAMIC_UPDATE);

	int attributes_base_offset = 0;
	int attributes_stride = 0;
	int positions_stride = 0;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		r_offsets[i] = 0; //reset

		if (!(p_format & (1 << i))) { // no array
			continue;
		}

		int elem_size = 0;

		switch (i) {
			case RS::ARRAY_VERTEX: {
				if (p_format & ARRAY_FLAG_USE_2D_VERTICES) {
					elem_size = 2;
				} else {
					elem_size = 3;
				}

				if (p_format & ARRAY_COMPRESS_VERTEX) {
					elem_size *= sizeof(int16_t);
				} else {
					elem_size *= sizeof(float);
				}

				if (elem_size == 6) {
					elem_size = 8;
				}

				r_offsets[i] = 0;
				positions_stride = elem_size;
				if (use_split_stream) {
					attributes_base_offset = elem_size * p_vertex_len;
				} else {
					attributes_base_offset = elem_size;
				}

			} break;
			case RS::ARRAY_NORMAL: {
				if (p_format & ARRAY_COMPRESS_NORMAL) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 3;
				}

				r_offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;

			case RS::ARRAY_TANGENT: {
				if (p_format & ARRAY_COMPRESS_TANGENT) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 4;
				}

				r_offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_COLOR: {
				if (p_format & ARRAY_COMPRESS_COLOR) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 4;
				}
				r_offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_TEX_UV: {
				if (p_format & ARRAY_COMPRESS_TEX_UV) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 2;
				}
				r_offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;

			case RS::ARRAY_TEX_UV2: {
				if (p_format & ARRAY_COMPRESS_TEX_UV2) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 2;
				}
				r_offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_INDEX: {
				if (p_index_len <= 0) {
					ERR_PRINT("index_array_len==NO_INDEX_ARRAY");
					break;
				}
				/* determine whether using 16 or 32 bits indices */
				if (p_vertex_len >= (1 << 16)) {
					elem_size = 4;

				} else {
					elem_size = 2;
				}
				r_offsets[i] = elem_size;
				continue;
			}
			default: {
				ERR_FAIL();
			}
		}
	}

	if (use_split_stream) {
		r_strides[RS::ARRAY_VERTEX] = positions_stride;
		for (int i = 1; i < RS::ARRAY_MAX - 1; i++) {
			r_strides[i] = attributes_stride;
		}
	} else {
		for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
			r_strides[i] = positions_stride + attributes_stride;
		}
	}
}

void RenderingServer::mesh_add_surface_from_arrays(RID p_mesh, PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes, uint32_t p_compress_format) {
	ERR_FAIL_INDEX(p_primitive, RS::PRIMITIVE_MAX);
	ERR_FAIL_COND(p_arrays.size() != RS::ARRAY_MAX);

	bool use_split_stream = GLOBAL_GET("rendering/misc/mesh_storage/split_stream") && !(p_compress_format & RS::ARRAY_FLAG_USE_DYNAMIC_UPDATE);

	uint32_t format = 0;

	// validation
	int index_array_len = 0;
	int array_len = 0;

	for (int i = 0; i < p_arrays.size(); i++) {
		if (p_arrays[i].get_type() == Variant::NIL) {
			continue;
		}

		format |= (1 << i);

		if (i == RS::ARRAY_VERTEX) {
			Variant var = p_arrays[i];
			switch (var.get_type()) {
				case Variant::POOL_VECTOR2_ARRAY: {
					PoolVector<Vector2> v2 = var;
				} break;
				case Variant::POOL_VECTOR3_ARRAY: {
					PoolVector<Vector3> v3 = var;
				} break;
				default: {
					Array v = var;
				} break;
			}

			array_len = PoolVector3Array(p_arrays[i]).size();
			ERR_FAIL_COND(array_len == 0);
		} else if (i == RS::ARRAY_INDEX) {
			index_array_len = PoolIntArray(p_arrays[i]).size();
		}
	}

	ERR_FAIL_COND((format & RS::ARRAY_FORMAT_VERTEX) == 0); // mandatory

	if (p_blend_shapes.size()) {
		//validate format for morphs
		for (int i = 0; i < p_blend_shapes.size(); i++) {
			uint32_t bsformat = 0;
			Array arr = p_blend_shapes[i];
			for (int j = 0; j < arr.size(); j++) {
				if (arr[j].get_type() != Variant::NIL) {
					bsformat |= (1 << j);
				}
			}

			ERR_FAIL_COND((bsformat) != (format & (RS::ARRAY_FORMAT_INDEX - 1)));
		}
	}

	uint32_t offsets[RS::ARRAY_MAX];
	uint32_t strides[RS::ARRAY_MAX];

	int attributes_base_offset = 0;
	int attributes_stride = 0;
	int positions_stride = 0;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		offsets[i] = 0; //reset

		if (!(format & (1 << i))) { // no array
			continue;
		}

		int elem_size = 0;

		switch (i) {
			case RS::ARRAY_VERTEX: {
				Variant arr = p_arrays[0];
				if (arr.get_type() == Variant::POOL_VECTOR2_ARRAY) {
					elem_size = 2;
					p_compress_format |= ARRAY_FLAG_USE_2D_VERTICES;
				} else if (arr.get_type() == Variant::POOL_VECTOR3_ARRAY) {
					p_compress_format &= ~ARRAY_FLAG_USE_2D_VERTICES;
					elem_size = 3;
				} else {
					elem_size = (p_compress_format & ARRAY_FLAG_USE_2D_VERTICES) ? 2 : 3;
				}

				if (p_compress_format & ARRAY_COMPRESS_VERTEX) {
					elem_size *= sizeof(int16_t);
				} else {
					elem_size *= sizeof(float);
				}

				if (elem_size == 6) {
					//had to pad
					elem_size = 8;
				}

				offsets[i] = 0;
				positions_stride = elem_size;
				if (use_split_stream) {
					attributes_base_offset = elem_size * array_len;
				} else {
					attributes_base_offset = elem_size;
				}

			} break;
			case RS::ARRAY_NORMAL: {
				if (p_compress_format & ARRAY_COMPRESS_NORMAL) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 3;
				}

				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;

			case RS::ARRAY_TANGENT: {
				if (p_compress_format & ARRAY_COMPRESS_TANGENT) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 4;
				}

				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_COLOR: {
				if (p_compress_format & ARRAY_COMPRESS_COLOR) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 4;
				}
				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_TEX_UV: {
				if (p_compress_format & ARRAY_COMPRESS_TEX_UV) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 2;
				}
				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;

			case RS::ARRAY_TEX_UV2: {
				if (p_compress_format & ARRAY_COMPRESS_TEX_UV2) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 2;
				}
				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_INDEX: {
				if (index_array_len <= 0) {
					ERR_PRINT("index_array_len==NO_INDEX_ARRAY");
					break;
				}
				/* determine whether using 16 or 32 bits indices */
				if (array_len >= (1 << 16)) {
					elem_size = 4;

				} else {
					elem_size = 2;
				}
				offsets[i] = elem_size;
				continue;
			}
			default: {
				ERR_FAIL();
			}
		}
	}

	if (use_split_stream) {
		strides[RS::ARRAY_VERTEX] = positions_stride;
		for (int i = 1; i < RS::ARRAY_MAX - 1; i++) {
			strides[i] = attributes_stride;
		}
	} else {
		for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
			strides[i] = positions_stride + attributes_stride;
		}
	}

	uint32_t mask = (1 << ARRAY_MAX) - 1;
	format |= (~mask) & p_compress_format; //make the full format

	int array_size = (positions_stride + attributes_stride) * array_len;

	PoolVector<uint8_t> vertex_array;
	vertex_array.resize(array_size);

	int index_array_size = offsets[RS::ARRAY_INDEX] * index_array_len;

	PoolVector<uint8_t> index_array;
	index_array.resize(index_array_size);

	AABB aabb;
	Vector<AABB> bone_aabb;

	Error err = _surface_set_data(p_arrays, format, offsets, strides, vertex_array, array_len, index_array, index_array_len, aabb, bone_aabb);
	ERR_FAIL_COND_MSG(err, "Invalid array format for surface.");

	Vector<PoolVector<uint8_t>> blend_shape_data;

	for (int i = 0; i < p_blend_shapes.size(); i++) {
		PoolVector<uint8_t> vertex_array_shape;
		vertex_array_shape.resize(array_size);
		PoolVector<uint8_t> noindex;

		AABB laabb;
		Error err2 = _surface_set_data(p_blend_shapes[i], format & ~ARRAY_FORMAT_INDEX, offsets, strides, vertex_array_shape, array_len, noindex, 0, laabb, bone_aabb);
		aabb.merge_with(laabb);
		ERR_FAIL_COND_MSG(err2 != OK, "Invalid blend shape array format for surface.");

		blend_shape_data.push_back(vertex_array_shape);
	}

	mesh_add_surface(p_mesh, format, p_primitive, vertex_array, array_len, index_array, index_array_len, aabb, blend_shape_data, bone_aabb);
}

Array RenderingServer::_get_array_from_surface(uint32_t p_format, PoolVector<uint8_t> p_vertex_data, int p_vertex_len, PoolVector<uint8_t> p_index_data, int p_index_len) const {
	bool use_split_stream = GLOBAL_GET("rendering/misc/mesh_storage/split_stream") && !(p_format & RS::ARRAY_FLAG_USE_DYNAMIC_UPDATE);

	uint32_t offsets[ARRAY_MAX];
	uint32_t strides[RS::ARRAY_MAX];

	int attributes_base_offset = 0;
	int attributes_stride = 0;
	int positions_stride = 0;

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		offsets[i] = 0; //reset

		if (!(p_format & (1 << i))) { // no array
			continue;
		}

		int elem_size = 0;
		switch (i) {
			case RS::ARRAY_VERTEX: {
				if (p_format & ARRAY_FLAG_USE_2D_VERTICES) {
					elem_size = 2;
				} else {
					elem_size = 3;
				}

				if (p_format & ARRAY_COMPRESS_VERTEX) {
					elem_size *= sizeof(int16_t);
				} else {
					elem_size *= sizeof(float);
				}

				if (elem_size == 6) {
					elem_size = 8;
				}

				offsets[i] = 0;
				positions_stride = elem_size;
				if (use_split_stream) {
					attributes_base_offset = elem_size * p_vertex_len;
				} else {
					attributes_base_offset = elem_size;
				}

			} break;
			case RS::ARRAY_NORMAL: {
				if (p_format & ARRAY_COMPRESS_NORMAL) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 3;
				}

				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;

			case RS::ARRAY_TANGENT: {
				if (p_format & ARRAY_COMPRESS_TANGENT) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 4;
				}

				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_COLOR: {
				if (p_format & ARRAY_COMPRESS_COLOR) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 4;
				}
				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_TEX_UV: {
				if (p_format & ARRAY_COMPRESS_TEX_UV) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 2;
				}
				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;

			case RS::ARRAY_TEX_UV2: {
				if (p_format & ARRAY_COMPRESS_TEX_UV2) {
					elem_size = sizeof(uint32_t);
				} else {
					elem_size = sizeof(float) * 2;
				}
				offsets[i] = attributes_base_offset + attributes_stride;
				attributes_stride += elem_size;

			} break;
			case RS::ARRAY_INDEX: {
				if (p_index_len <= 0) {
					ERR_PRINT("index_array_len==NO_INDEX_ARRAY");
					break;
				}
				/* determine whether using 16 or 32 bits indices */
				if (p_vertex_len >= (1 << 16)) {
					elem_size = 4;

				} else {
					elem_size = 2;
				}
				offsets[i] = elem_size;
				continue;
			}
			default: {
				ERR_FAIL_V(Array());
			}
		}
	}

	if (use_split_stream) {
		strides[RS::ARRAY_VERTEX] = positions_stride;
		for (int i = 1; i < RS::ARRAY_MAX - 1; i++) {
			strides[i] = attributes_stride;
		}
	} else {
		for (int i = 0; i < RS::ARRAY_MAX - 1; i++) {
			strides[i] = positions_stride + attributes_stride;
		}
	}

	Array ret;
	ret.resize(RS::ARRAY_MAX);

	PoolVector<uint8_t>::Read r = p_vertex_data.read();

	for (int i = 0; i < RS::ARRAY_MAX; i++) {
		if (!(p_format & (1 << i))) {
			continue;
		}

		switch (i) {
			case RS::ARRAY_VERTEX: {
				if (p_format & ARRAY_FLAG_USE_2D_VERTICES) {
					PoolVector<Vector2> arr_2d;
					arr_2d.resize(p_vertex_len);

					if (p_format & ARRAY_COMPRESS_VERTEX) {
						PoolVector<Vector2>::Write w = arr_2d.write();

						for (int j = 0; j < p_vertex_len; j++) {
							const uint16_t *v = (const uint16_t *)&r[j * strides[i] + offsets[i]];
							w[j] = Vector2(Math::halfptr_to_float(&v[0]), Math::halfptr_to_float(&v[1]));
						}
					} else {
						PoolVector<Vector2>::Write w = arr_2d.write();

						for (int j = 0; j < p_vertex_len; j++) {
							const float *v = (const float *)&r[j * strides[i] + offsets[i]];
							w[j] = Vector2(v[0], v[1]);
						}
					}

					ret[i] = arr_2d;
				} else {
					PoolVector<Vector3> arr_3d;
					arr_3d.resize(p_vertex_len);

					if (p_format & ARRAY_COMPRESS_VERTEX) {
						PoolVector<Vector3>::Write w = arr_3d.write();

						for (int j = 0; j < p_vertex_len; j++) {
							const uint16_t *v = (const uint16_t *)&r[j * strides[i] + offsets[i]];
							w[j] = Vector3(Math::halfptr_to_float(&v[0]), Math::halfptr_to_float(&v[1]), Math::halfptr_to_float(&v[2]));
						}
					} else {
						PoolVector<Vector3>::Write w = arr_3d.write();

						for (int j = 0; j < p_vertex_len; j++) {
							const float *v = (const float *)&r[j * strides[i] + offsets[i]];
							w[j] = Vector3(v[0], v[1], v[2]);
						}
					}

					ret[i] = arr_3d;
				}

			} break;
			case RS::ARRAY_NORMAL: {
				PoolVector<Vector3> arr;
				arr.resize(p_vertex_len);

				if (p_format & ARRAY_COMPRESS_NORMAL) {
					PoolVector<Vector3>::Write w = arr.write();
					const float multiplier = 1.f / 127.f;

					for (int j = 0; j < p_vertex_len; j++) {
						const int8_t *v = (const int8_t *)&r[j * strides[i] + offsets[i]];
						w[j] = Vector3(float(v[0]) * multiplier, float(v[1]) * multiplier, float(v[2]) * multiplier);
					}
				} else {
					PoolVector<Vector3>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const float *v = (const float *)&r[j * strides[i] + offsets[i]];
						w[j] = Vector3(v[0], v[1], v[2]);
					}
				}

				ret[i] = arr;

			} break;

			case RS::ARRAY_TANGENT: {
				PoolVector<float> arr;
				arr.resize(p_vertex_len * 4);

				if (p_format & ARRAY_COMPRESS_TANGENT) {
					PoolVector<float>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const int8_t *v = (const int8_t *)&r[j * strides[i] + offsets[i]];
						for (int k = 0; k < 4; k++) {
							w[j * 4 + k] = float(v[k] / 127.0);
						}
					}
				} else {
					PoolVector<float>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const float *v = (const float *)&r[j * strides[i] + offsets[i]];
						for (int k = 0; k < 4; k++) {
							w[j * 4 + k] = v[k];
						}
					}
				}

				ret[i] = arr;

			} break;
			case RS::ARRAY_COLOR: {
				PoolVector<Color> arr;
				arr.resize(p_vertex_len);

				if (p_format & ARRAY_COMPRESS_COLOR) {
					PoolVector<Color>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const uint8_t *v = (const uint8_t *)&r[j * strides[i] + offsets[i]];
						w[j] = Color(float(v[0] / 255.0), float(v[1] / 255.0), float(v[2] / 255.0), float(v[3] / 255.0));
					}
				} else {
					PoolVector<Color>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const float *v = (const float *)&r[j * strides[i] + offsets[i]];
						w[j] = Color(v[0], v[1], v[2], v[3]);
					}
				}

				ret[i] = arr;
			} break;
			case RS::ARRAY_TEX_UV: {
				PoolVector<Vector2> arr;
				arr.resize(p_vertex_len);

				if (p_format & ARRAY_COMPRESS_TEX_UV) {
					PoolVector<Vector2>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const uint16_t *v = (const uint16_t *)&r[j * strides[i] + offsets[i]];
						w[j] = Vector2(Math::halfptr_to_float(&v[0]), Math::halfptr_to_float(&v[1]));
					}
				} else {
					PoolVector<Vector2>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const float *v = (const float *)&r[j * strides[i] + offsets[i]];
						w[j] = Vector2(v[0], v[1]);
					}
				}

				ret[i] = arr;
			} break;

			case RS::ARRAY_TEX_UV2: {
				PoolVector<Vector2> arr;
				arr.resize(p_vertex_len);

				if (p_format & ARRAY_COMPRESS_TEX_UV2) {
					PoolVector<Vector2>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const uint16_t *v = (const uint16_t *)&r[j * strides[i] + offsets[i]];
						w[j] = Vector2(Math::halfptr_to_float(&v[0]), Math::halfptr_to_float(&v[1]));
					}
				} else {
					PoolVector<Vector2>::Write w = arr.write();

					for (int j = 0; j < p_vertex_len; j++) {
						const float *v = (const float *)&r[j * strides[i] + offsets[i]];
						w[j] = Vector2(v[0], v[1]);
					}
				}

				ret[i] = arr;

			} break;
			case RS::ARRAY_INDEX: {
				/* determine whether using 16 or 32 bits indices */

				PoolVector<uint8_t>::Read ir = p_index_data.read();

				PoolVector<int> arr;
				arr.resize(p_index_len);
				if (p_vertex_len < (1 << 16)) {
					PoolVector<int>::Write w = arr.write();

					for (int j = 0; j < p_index_len; j++) {
						const uint16_t *v = (const uint16_t *)&ir[j * 2];
						w[j] = *v;
					}
				} else {
					PoolVector<int>::Write w = arr.write();

					for (int j = 0; j < p_index_len; j++) {
						const int *v = (const int *)&ir[j * 4];
						w[j] = *v;
					}
				}
				ret[i] = arr;
			} break;
			default: {
				ERR_FAIL_V(ret);
			}
		}
	}

	return ret;
}

Array RenderingServer::mesh_surface_get_arrays(RID p_mesh, int p_surface) const {
	PoolVector<uint8_t> vertex_data = mesh_surface_get_array(p_mesh, p_surface);
	ERR_FAIL_COND_V(vertex_data.size() == 0, Array());
	int vertex_len = mesh_surface_get_array_len(p_mesh, p_surface);

	PoolVector<uint8_t> index_data = mesh_surface_get_index_array(p_mesh, p_surface);
	int index_len = mesh_surface_get_array_index_len(p_mesh, p_surface);

	uint32_t format = mesh_surface_get_format(p_mesh, p_surface);

	return _get_array_from_surface(format, vertex_data, vertex_len, index_data, index_len);
}

Array RenderingServer::mesh_surface_get_blend_shape_arrays(RID p_mesh, int p_surface) const {
	Vector<PoolVector<uint8_t>> blend_shape_data = mesh_surface_get_blend_shapes(p_mesh, p_surface);
	if (blend_shape_data.size() > 0) {
		int vertex_len = mesh_surface_get_array_len(p_mesh, p_surface);

		PoolVector<uint8_t> index_data = mesh_surface_get_index_array(p_mesh, p_surface);
		int index_len = mesh_surface_get_array_index_len(p_mesh, p_surface);

		uint32_t format = mesh_surface_get_format(p_mesh, p_surface);

		Array blend_shape_array;
		blend_shape_array.resize(blend_shape_data.size());
		for (int i = 0; i < blend_shape_data.size(); i++) {
			blend_shape_array.set(i, _get_array_from_surface(format, blend_shape_data[i], vertex_len, index_data, index_len));
		}

		return blend_shape_array;
	} else {
		return Array();
	}
}

void RenderingServer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("force_sync"), &RenderingServer::sync);
	ClassDB::bind_method(D_METHOD("force_draw", "swap_buffers", "frame_step"), &RenderingServer::draw, DEFVAL(true), DEFVAL(0.0));

	// "draw" and "sync" are deprecated duplicates of "force_draw" and "force_sync"
	// FIXME: Add deprecation messages using GH-4397 once available, and retire
	// once the warnings have been enabled for a full release cycle
	ClassDB::bind_method(D_METHOD("sync"), &RenderingServer::sync);
	ClassDB::bind_method(D_METHOD("draw", "swap_buffers", "frame_step"), &RenderingServer::draw, DEFVAL(true), DEFVAL(0.0));

	ClassDB::bind_method(D_METHOD("texture_create"), &RenderingServer::texture_create);
	ClassDB::bind_method(D_METHOD("texture_create_from_image", "image", "flags"), &RenderingServer::texture_create_from_image, DEFVAL(TEXTURE_FLAGS_DEFAULT));
	ClassDB::bind_method(D_METHOD("texture_allocate", "texture", "width", "height", "depth_3d", "format", "type", "flags"), &RenderingServer::texture_allocate, DEFVAL(TEXTURE_FLAGS_DEFAULT));
	ClassDB::bind_method(D_METHOD("texture_set_data", "texture", "image", "layer"), &RenderingServer::texture_set_data, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("texture_set_data_partial", "texture", "image", "src_x", "src_y", "src_w", "src_h", "dst_x", "dst_y", "dst_mip", "layer"), &RenderingServer::texture_set_data_partial, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("texture_get_data", "texture", "cube_side"), &RenderingServer::texture_get_data, DEFVAL(CUBEMAP_LEFT));
	ClassDB::bind_method(D_METHOD("texture_set_flags", "texture", "flags"), &RenderingServer::texture_set_flags);
	ClassDB::bind_method(D_METHOD("texture_get_flags", "texture"), &RenderingServer::texture_get_flags);
	ClassDB::bind_method(D_METHOD("texture_get_format", "texture"), &RenderingServer::texture_get_format);
	ClassDB::bind_method(D_METHOD("texture_get_type", "texture"), &RenderingServer::texture_get_type);
	ClassDB::bind_method(D_METHOD("texture_get_texid", "texture"), &RenderingServer::texture_get_texid);
	ClassDB::bind_method(D_METHOD("texture_get_width", "texture"), &RenderingServer::texture_get_width);
	ClassDB::bind_method(D_METHOD("texture_get_height", "texture"), &RenderingServer::texture_get_height);
	ClassDB::bind_method(D_METHOD("texture_get_depth", "texture"), &RenderingServer::texture_get_depth);
	ClassDB::bind_method(D_METHOD("texture_set_size_override", "texture", "width", "height", "depth"), &RenderingServer::texture_set_size_override);
	ClassDB::bind_method(D_METHOD("texture_set_path", "texture", "path"), &RenderingServer::texture_set_path);
	ClassDB::bind_method(D_METHOD("texture_get_path", "texture"), &RenderingServer::texture_get_path);
	ClassDB::bind_method(D_METHOD("texture_set_shrink_all_x2_on_set_data", "shrink"), &RenderingServer::texture_set_shrink_all_x2_on_set_data);
	ClassDB::bind_method(D_METHOD("texture_set_proxy", "proxy", "base"), &RenderingServer::texture_set_proxy);
	ClassDB::bind_method(D_METHOD("texture_bind", "texture", "number"), &RenderingServer::texture_bind);

	ClassDB::bind_method(D_METHOD("texture_debug_usage"), &RenderingServer::_texture_debug_usage_bind);
	ClassDB::bind_method(D_METHOD("textures_keep_original", "enable"), &RenderingServer::textures_keep_original);

	ClassDB::bind_method(D_METHOD("shader_create"), &RenderingServer::shader_create);
	ClassDB::bind_method(D_METHOD("shader_set_code", "shader", "code"), &RenderingServer::shader_set_code);
	ClassDB::bind_method(D_METHOD("shader_get_code", "shader"), &RenderingServer::shader_get_code);
	ClassDB::bind_method(D_METHOD("shader_get_param_list", "shader"), &RenderingServer::_shader_get_param_list_bind);
	ClassDB::bind_method(D_METHOD("shader_set_default_texture_param", "shader", "name", "texture"), &RenderingServer::shader_set_default_texture_param);
	ClassDB::bind_method(D_METHOD("shader_get_default_texture_param", "shader", "name"), &RenderingServer::shader_get_default_texture_param);
	ClassDB::bind_method(D_METHOD("set_shader_async_hidden_forbidden", "forbidden"), &RenderingServer::set_shader_async_hidden_forbidden);

	ClassDB::bind_method(D_METHOD("material_create"), &RenderingServer::material_create);
	ClassDB::bind_method(D_METHOD("material_set_shader", "shader_material", "shader"), &RenderingServer::material_set_shader);
	ClassDB::bind_method(D_METHOD("material_get_shader", "shader_material"), &RenderingServer::material_get_shader);
	ClassDB::bind_method(D_METHOD("material_set_param", "material", "parameter", "value"), &RenderingServer::material_set_param);
	ClassDB::bind_method(D_METHOD("material_get_param", "material", "parameter"), &RenderingServer::material_get_param);
	ClassDB::bind_method(D_METHOD("material_get_param_default", "material", "parameter"), &RenderingServer::material_get_param_default);
	ClassDB::bind_method(D_METHOD("material_set_render_priority", "material", "priority"), &RenderingServer::material_set_render_priority);
	ClassDB::bind_method(D_METHOD("material_set_line_width", "material", "width"), &RenderingServer::material_set_line_width);
	ClassDB::bind_method(D_METHOD("material_set_next_pass", "material", "next_material"), &RenderingServer::material_set_next_pass);

	ClassDB::bind_method(D_METHOD("mesh_create"), &RenderingServer::mesh_create);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_format_offset", "format", "vertex_len", "index_len", "array_index"), &RenderingServer::mesh_surface_get_format_offset);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_format_stride", "format", "vertex_len", "index_len", "array_index"), &RenderingServer::mesh_surface_get_format_stride);
	ClassDB::bind_method(D_METHOD("mesh_add_surface_from_arrays", "mesh", "primitive", "arrays", "blend_shapes", "compress_format"), &RenderingServer::mesh_add_surface_from_arrays, DEFVAL(Array()), DEFVAL(ARRAY_COMPRESS_DEFAULT));
	ClassDB::bind_method(D_METHOD("mesh_set_blend_shape_count", "mesh", "amount"), &RenderingServer::mesh_set_blend_shape_count);
	ClassDB::bind_method(D_METHOD("mesh_get_blend_shape_count", "mesh"), &RenderingServer::mesh_get_blend_shape_count);
	ClassDB::bind_method(D_METHOD("mesh_set_blend_shape_mode", "mesh", "mode"), &RenderingServer::mesh_set_blend_shape_mode);
	ClassDB::bind_method(D_METHOD("mesh_get_blend_shape_mode", "mesh"), &RenderingServer::mesh_get_blend_shape_mode);
	ClassDB::bind_method(D_METHOD("mesh_surface_update_region", "mesh", "surface", "offset", "data"), &RenderingServer::mesh_surface_update_region);
	ClassDB::bind_method(D_METHOD("mesh_surface_set_material", "mesh", "surface", "material"), &RenderingServer::mesh_surface_set_material);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_material", "mesh", "surface"), &RenderingServer::mesh_surface_get_material);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_array_len", "mesh", "surface"), &RenderingServer::mesh_surface_get_array_len);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_array_index_len", "mesh", "surface"), &RenderingServer::mesh_surface_get_array_index_len);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_array", "mesh", "surface"), &RenderingServer::mesh_surface_get_array);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_index_array", "mesh", "surface"), &RenderingServer::mesh_surface_get_index_array);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_arrays", "mesh", "surface"), &RenderingServer::mesh_surface_get_arrays);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_blend_shape_arrays", "mesh", "surface"), &RenderingServer::mesh_surface_get_blend_shape_arrays);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_format", "mesh", "surface"), &RenderingServer::mesh_surface_get_format);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_primitive_type", "mesh", "surface"), &RenderingServer::mesh_surface_get_primitive_type);
	ClassDB::bind_method(D_METHOD("mesh_surface_get_aabb", "mesh", "surface"), &RenderingServer::mesh_surface_get_aabb);
	ClassDB::bind_method(D_METHOD("mesh_remove_surface", "mesh", "index"), &RenderingServer::mesh_remove_surface);
	ClassDB::bind_method(D_METHOD("mesh_get_surface_count", "mesh"), &RenderingServer::mesh_get_surface_count);
	ClassDB::bind_method(D_METHOD("mesh_set_custom_aabb", "mesh", "aabb"), &RenderingServer::mesh_set_custom_aabb);
	ClassDB::bind_method(D_METHOD("mesh_get_custom_aabb", "mesh"), &RenderingServer::mesh_get_custom_aabb);
	ClassDB::bind_method(D_METHOD("mesh_clear", "mesh"), &RenderingServer::mesh_clear);

	ClassDB::bind_method(D_METHOD("multimesh_create"), &RenderingServer::multimesh_create);
	ClassDB::bind_method(D_METHOD("multimesh_allocate", "multimesh", "instances", "transform_format", "color_format", "custom_data_format"), &RenderingServer::multimesh_allocate, DEFVAL(MULTIMESH_CUSTOM_DATA_NONE));
	ClassDB::bind_method(D_METHOD("multimesh_get_instance_count", "multimesh"), &RenderingServer::multimesh_get_instance_count);
	ClassDB::bind_method(D_METHOD("multimesh_set_mesh", "multimesh", "mesh"), &RenderingServer::multimesh_set_mesh);
	ClassDB::bind_method(D_METHOD("multimesh_instance_set_transform", "multimesh", "index", "transform"), &RenderingServer::multimesh_instance_set_transform);
	ClassDB::bind_method(D_METHOD("multimesh_instance_set_transform_2d", "multimesh", "index", "transform"), &RenderingServer::multimesh_instance_set_transform_2d);
	ClassDB::bind_method(D_METHOD("multimesh_instance_set_color", "multimesh", "index", "color"), &RenderingServer::multimesh_instance_set_color);
	ClassDB::bind_method(D_METHOD("multimesh_instance_set_custom_data", "multimesh", "index", "custom_data"), &RenderingServer::multimesh_instance_set_custom_data);
	ClassDB::bind_method(D_METHOD("multimesh_get_mesh", "multimesh"), &RenderingServer::multimesh_get_mesh);
	ClassDB::bind_method(D_METHOD("multimesh_get_aabb", "multimesh"), &RenderingServer::multimesh_get_aabb);
	ClassDB::bind_method(D_METHOD("multimesh_instance_get_transform", "multimesh", "index"), &RenderingServer::multimesh_instance_get_transform);
	ClassDB::bind_method(D_METHOD("multimesh_instance_get_transform_2d", "multimesh", "index"), &RenderingServer::multimesh_instance_get_transform_2d);
	ClassDB::bind_method(D_METHOD("multimesh_instance_get_color", "multimesh", "index"), &RenderingServer::multimesh_instance_get_color);
	ClassDB::bind_method(D_METHOD("multimesh_instance_get_custom_data", "multimesh", "index"), &RenderingServer::multimesh_instance_get_custom_data);
	ClassDB::bind_method(D_METHOD("multimesh_set_visible_instances", "multimesh", "visible"), &RenderingServer::multimesh_set_visible_instances);
	ClassDB::bind_method(D_METHOD("multimesh_get_visible_instances", "multimesh"), &RenderingServer::multimesh_get_visible_instances);
	ClassDB::bind_method(D_METHOD("multimesh_set_as_bulk_array", "multimesh", "array"), &RenderingServer::multimesh_set_as_bulk_array);
	ClassDB::bind_method(D_METHOD("multimesh_set_as_bulk_array_interpolated", "multimesh", "array", "array_previous"), &RenderingServer::multimesh_set_as_bulk_array_interpolated);
	ClassDB::bind_method(D_METHOD("multimesh_set_physics_interpolated", "multimesh", "interpolated"), &RenderingServer::multimesh_set_physics_interpolated);
	ClassDB::bind_method(D_METHOD("multimesh_set_physics_interpolation_quality", "multimesh", "quality"), &RenderingServer::multimesh_set_physics_interpolation_quality);
	ClassDB::bind_method(D_METHOD("multimesh_instance_reset_physics_interpolation", "multimesh", "index"), &RenderingServer::multimesh_instance_reset_physics_interpolation);

	ClassDB::bind_method(D_METHOD("viewport_create"), &RenderingServer::viewport_create);
	ClassDB::bind_method(D_METHOD("viewport_set_size", "viewport", "width", "height"), &RenderingServer::viewport_set_size);
	ClassDB::bind_method(D_METHOD("viewport_set_active", "viewport", "active"), &RenderingServer::viewport_set_active);
	ClassDB::bind_method(D_METHOD("viewport_set_parent_viewport", "viewport", "parent_viewport"), &RenderingServer::viewport_set_parent_viewport);
	ClassDB::bind_method(D_METHOD("viewport_attach_to_screen", "viewport", "rect", "screen"), &RenderingServer::viewport_attach_to_screen, DEFVAL(Rect2()), DEFVAL(0));
	ClassDB::bind_method(D_METHOD("viewport_set_render_direct_to_screen", "viewport", "enabled"), &RenderingServer::viewport_set_render_direct_to_screen);
	ClassDB::bind_method(D_METHOD("viewport_detach", "viewport"), &RenderingServer::viewport_detach);
	ClassDB::bind_method(D_METHOD("viewport_set_update_mode", "viewport", "update_mode"), &RenderingServer::viewport_set_update_mode);
	ClassDB::bind_method(D_METHOD("viewport_set_vflip", "viewport", "enabled"), &RenderingServer::viewport_set_vflip);
	ClassDB::bind_method(D_METHOD("viewport_set_clear_mode", "viewport", "clear_mode"), &RenderingServer::viewport_set_clear_mode);
	ClassDB::bind_method(D_METHOD("viewport_get_texture", "viewport"), &RenderingServer::viewport_get_texture);
	ClassDB::bind_method(D_METHOD("viewport_set_hide_canvas", "viewport", "hidden"), &RenderingServer::viewport_set_hide_canvas);
	ClassDB::bind_method(D_METHOD("viewport_attach_canvas", "viewport", "canvas"), &RenderingServer::viewport_attach_canvas);
	ClassDB::bind_method(D_METHOD("viewport_remove_canvas", "viewport", "canvas"), &RenderingServer::viewport_remove_canvas);
	ClassDB::bind_method(D_METHOD("viewport_set_canvas_transform", "viewport", "canvas", "offset"), &RenderingServer::viewport_set_canvas_transform);
	ClassDB::bind_method(D_METHOD("viewport_set_transparent_background", "viewport", "enabled"), &RenderingServer::viewport_set_transparent_background);
	ClassDB::bind_method(D_METHOD("viewport_set_global_canvas_transform", "viewport", "transform"), &RenderingServer::viewport_set_global_canvas_transform);
	ClassDB::bind_method(D_METHOD("viewport_set_canvas_stacking", "viewport", "canvas", "layer", "sublayer"), &RenderingServer::viewport_set_canvas_stacking);
	ClassDB::bind_method(D_METHOD("viewport_set_msaa", "viewport", "msaa"), &RenderingServer::viewport_set_msaa);
	ClassDB::bind_method(D_METHOD("viewport_set_use_fxaa", "viewport", "fxaa"), &RenderingServer::viewport_set_use_fxaa);
	ClassDB::bind_method(D_METHOD("viewport_set_use_debanding", "viewport", "debanding"), &RenderingServer::viewport_set_use_debanding);
	ClassDB::bind_method(D_METHOD("viewport_set_sharpen_intensity", "viewport", "intensity"), &RenderingServer::viewport_set_sharpen_intensity);
	ClassDB::bind_method(D_METHOD("viewport_set_hdr", "viewport", "enabled"), &RenderingServer::viewport_set_hdr);
	ClassDB::bind_method(D_METHOD("viewport_set_use_32_bpc_depth", "viewport", "enabled"), &RenderingServer::viewport_set_use_32_bpc_depth);
	ClassDB::bind_method(D_METHOD("viewport_set_usage", "viewport", "usage"), &RenderingServer::viewport_set_usage);
	ClassDB::bind_method(D_METHOD("viewport_get_render_info", "viewport", "info"), &RenderingServer::viewport_get_render_info);
	ClassDB::bind_method(D_METHOD("viewport_set_debug_draw", "viewport", "draw"), &RenderingServer::viewport_set_debug_draw);

	ClassDB::bind_method(D_METHOD("canvas_create"), &RenderingServer::canvas_create);
	ClassDB::bind_method(D_METHOD("canvas_set_item_mirroring", "canvas", "item", "mirroring"), &RenderingServer::canvas_set_item_mirroring);
	ClassDB::bind_method(D_METHOD("canvas_set_modulate", "canvas", "color"), &RenderingServer::canvas_set_modulate);

	ClassDB::bind_method(D_METHOD("canvas_item_create"), &RenderingServer::canvas_item_create);
	ClassDB::bind_method(D_METHOD("canvas_item_set_parent", "item", "parent"), &RenderingServer::canvas_item_set_parent);
	ClassDB::bind_method(D_METHOD("canvas_item_set_visible", "item", "visible"), &RenderingServer::canvas_item_set_visible);
	ClassDB::bind_method(D_METHOD("canvas_item_set_light_mask", "item", "mask"), &RenderingServer::canvas_item_set_light_mask);
	ClassDB::bind_method(D_METHOD("canvas_item_set_transform", "item", "transform"), &RenderingServer::canvas_item_set_transform);
	ClassDB::bind_method(D_METHOD("canvas_item_set_clip", "item", "clip"), &RenderingServer::canvas_item_set_clip);
	ClassDB::bind_method(D_METHOD("canvas_item_set_distance_field_mode", "item", "enabled"), &RenderingServer::canvas_item_set_distance_field_mode);
	ClassDB::bind_method(D_METHOD("canvas_item_set_custom_rect", "item", "use_custom_rect", "rect"), &RenderingServer::canvas_item_set_custom_rect, DEFVAL(Rect2()));
	ClassDB::bind_method(D_METHOD("canvas_item_set_modulate", "item", "color"), &RenderingServer::canvas_item_set_modulate);
	ClassDB::bind_method(D_METHOD("canvas_item_set_self_modulate", "item", "color"), &RenderingServer::canvas_item_set_self_modulate);
	ClassDB::bind_method(D_METHOD("canvas_item_set_draw_behind_parent", "item", "enabled"), &RenderingServer::canvas_item_set_draw_behind_parent);
	ClassDB::bind_method(D_METHOD("canvas_item_add_line", "item", "from", "to", "color", "width", "antialiased"), &RenderingServer::canvas_item_add_line, DEFVAL(1.0), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("canvas_item_add_polyline", "item", "points", "colors", "width", "antialiased"), &RenderingServer::canvas_item_add_polyline, DEFVAL(1.0), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("canvas_item_add_rect", "item", "rect", "color"), &RenderingServer::canvas_item_add_rect);
	ClassDB::bind_method(D_METHOD("canvas_item_add_circle", "item", "pos", "radius", "color"), &RenderingServer::canvas_item_add_circle);
	ClassDB::bind_method(D_METHOD("canvas_item_add_texture_rect", "item", "rect", "texture", "tile", "modulate", "transpose", "normal_map"), &RenderingServer::canvas_item_add_texture_rect, DEFVAL(false), DEFVAL(Color(1, 1, 1)), DEFVAL(false), DEFVAL(RID()));
	ClassDB::bind_method(D_METHOD("canvas_item_add_texture_rect_region", "item", "rect", "texture", "src_rect", "modulate", "transpose", "normal_map", "clip_uv"), &RenderingServer::canvas_item_add_texture_rect_region, DEFVAL(Color(1, 1, 1)), DEFVAL(false), DEFVAL(RID()), DEFVAL(true));
	ClassDB::bind_method(D_METHOD("canvas_item_add_nine_patch", "item", "rect", "source", "texture", "topleft", "bottomright", "x_axis_mode", "y_axis_mode", "draw_center", "modulate", "normal_map"), &RenderingServer::canvas_item_add_nine_patch, DEFVAL(NINE_PATCH_STRETCH), DEFVAL(NINE_PATCH_STRETCH), DEFVAL(true), DEFVAL(Color(1, 1, 1)), DEFVAL(RID()));
	ClassDB::bind_method(D_METHOD("canvas_item_add_primitive", "item", "points", "colors", "uvs", "texture", "width", "normal_map"), &RenderingServer::canvas_item_add_primitive, DEFVAL(1.0), DEFVAL(RID()));
	ClassDB::bind_method(D_METHOD("canvas_item_add_polygon", "item", "points", "colors", "uvs", "texture", "normal_map", "antialiased"), &RenderingServer::canvas_item_add_polygon, DEFVAL(Vector<Point2>()), DEFVAL(RID()), DEFVAL(RID()), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("canvas_item_add_triangle_array", "item", "indices", "points", "colors", "uvs", "bones", "weights", "texture", "count", "normal_map", "antialiased", "antialiasing_use_indices"), &RenderingServer::canvas_item_add_triangle_array, DEFVAL(Vector<Point2>()), DEFVAL(Vector<int>()), DEFVAL(Vector<float>()), DEFVAL(RID()), DEFVAL(-1), DEFVAL(RID()), DEFVAL(false), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("canvas_item_add_mesh", "item", "mesh", "transform", "modulate", "texture", "normal_map"), &RenderingServer::canvas_item_add_mesh, DEFVAL(Transform2D()), DEFVAL(Color(1, 1, 1)), DEFVAL(RID()), DEFVAL(RID()));
	ClassDB::bind_method(D_METHOD("canvas_item_add_multimesh", "item", "mesh", "texture", "normal_map"), &RenderingServer::canvas_item_add_multimesh, DEFVAL(RID()));
	ClassDB::bind_method(D_METHOD("canvas_item_add_set_transform", "item", "transform"), &RenderingServer::canvas_item_add_set_transform);
	ClassDB::bind_method(D_METHOD("canvas_item_add_clip_ignore", "item", "ignore"), &RenderingServer::canvas_item_add_clip_ignore);
	ClassDB::bind_method(D_METHOD("canvas_item_set_sort_children_by_y", "item", "enabled"), &RenderingServer::canvas_item_set_sort_children_by_y);
	ClassDB::bind_method(D_METHOD("canvas_item_set_z_index", "item", "z_index"), &RenderingServer::canvas_item_set_z_index);
	ClassDB::bind_method(D_METHOD("canvas_item_set_z_as_relative_to_parent", "item", "enabled"), &RenderingServer::canvas_item_set_z_as_relative_to_parent);
	ClassDB::bind_method(D_METHOD("canvas_item_set_copy_to_backbuffer", "item", "enabled", "rect"), &RenderingServer::canvas_item_set_copy_to_backbuffer);
	ClassDB::bind_method(D_METHOD("canvas_item_clear", "item"), &RenderingServer::canvas_item_clear);
	ClassDB::bind_method(D_METHOD("canvas_item_set_draw_index", "item", "index"), &RenderingServer::canvas_item_set_draw_index);
	ClassDB::bind_method(D_METHOD("canvas_item_set_material", "item", "material"), &RenderingServer::canvas_item_set_material);
	ClassDB::bind_method(D_METHOD("canvas_item_set_use_parent_material", "item", "enabled"), &RenderingServer::canvas_item_set_use_parent_material);
	ClassDB::bind_method(D_METHOD("debug_canvas_item_get_rect", "item"), &RenderingServer::debug_canvas_item_get_rect);
	ClassDB::bind_method(D_METHOD("debug_canvas_item_get_local_bound", "item"), &RenderingServer::debug_canvas_item_get_local_bound);

	ClassDB::bind_method(D_METHOD("black_bars_set_margins", "left", "top", "right", "bottom"), &RenderingServer::black_bars_set_margins);
	ClassDB::bind_method(D_METHOD("black_bars_set_images", "left", "top", "right", "bottom"), &RenderingServer::black_bars_set_images);

	ClassDB::bind_method(D_METHOD("free_rid", "rid"), &RenderingServer::free); // shouldn't conflict with Object::free()

	ClassDB::bind_method(D_METHOD("request_frame_drawn_callback", "where", "method", "userdata"), &RenderingServer::request_frame_drawn_callback);
	ClassDB::bind_method(D_METHOD("has_changed", "queried_priority"), &RenderingServer::has_changed, DEFVAL(CHANGED_PRIORITY_ANY));
	ClassDB::bind_method(D_METHOD("init"), &RenderingServer::init);
	ClassDB::bind_method(D_METHOD("finish"), &RenderingServer::finish);
	ClassDB::bind_method(D_METHOD("get_render_info", "info"), &RenderingServer::get_render_info);
	ClassDB::bind_method(D_METHOD("get_video_adapter_name"), &RenderingServer::get_video_adapter_name);
	ClassDB::bind_method(D_METHOD("get_video_adapter_vendor"), &RenderingServer::get_video_adapter_vendor);
#ifndef _3D_DISABLED

	ClassDB::bind_method(D_METHOD("make_sphere_mesh", "latitudes", "longitudes", "radius"), &RenderingServer::make_sphere_mesh);
	ClassDB::bind_method(D_METHOD("get_test_cube"), &RenderingServer::get_test_cube);
#endif
	ClassDB::bind_method(D_METHOD("get_test_texture"), &RenderingServer::get_test_texture);
	ClassDB::bind_method(D_METHOD("get_white_texture"), &RenderingServer::get_white_texture);

	ClassDB::bind_method(D_METHOD("set_boot_image", "image", "color", "scale", "use_filter"), &RenderingServer::set_boot_image, DEFVAL(true));
	ClassDB::bind_method(D_METHOD("set_default_clear_color", "color"), &RenderingServer::set_default_clear_color);
	ClassDB::bind_method(D_METHOD("set_shader_time_scale", "scale"), &RenderingServer::set_shader_time_scale);

	ClassDB::bind_method(D_METHOD("has_feature", "feature"), &RenderingServer::has_feature);
	ClassDB::bind_method(D_METHOD("has_os_feature", "feature"), &RenderingServer::has_os_feature);
	ClassDB::bind_method(D_METHOD("set_debug_generate_wireframes", "generate"), &RenderingServer::set_debug_generate_wireframes);

	ClassDB::bind_method(D_METHOD("is_render_loop_enabled"), &RenderingServer::is_render_loop_enabled);
	ClassDB::bind_method(D_METHOD("set_render_loop_enabled", "enabled"), &RenderingServer::set_render_loop_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "render_loop_enabled"), "set_render_loop_enabled", "is_render_loop_enabled");

	BIND_CONSTANT(NO_INDEX_ARRAY);
	BIND_CONSTANT(ARRAY_WEIGHTS_SIZE);
	BIND_CONSTANT(CANVAS_ITEM_Z_MIN);
	BIND_CONSTANT(CANVAS_ITEM_Z_MAX);
	BIND_CONSTANT(MAX_GLOW_LEVELS);
	BIND_CONSTANT(MAX_CURSORS);
	BIND_CONSTANT(MATERIAL_RENDER_PRIORITY_MIN);
	BIND_CONSTANT(MATERIAL_RENDER_PRIORITY_MAX);

	BIND_ENUM_CONSTANT(CUBEMAP_LEFT);
	BIND_ENUM_CONSTANT(CUBEMAP_RIGHT);
	BIND_ENUM_CONSTANT(CUBEMAP_BOTTOM);
	BIND_ENUM_CONSTANT(CUBEMAP_TOP);
	BIND_ENUM_CONSTANT(CUBEMAP_FRONT);
	BIND_ENUM_CONSTANT(CUBEMAP_BACK);

	BIND_ENUM_CONSTANT(TEXTURE_TYPE_2D);
	BIND_ENUM_CONSTANT(TEXTURE_TYPE_CUBEMAP);
	BIND_ENUM_CONSTANT(TEXTURE_TYPE_2D_ARRAY);
	BIND_ENUM_CONSTANT(TEXTURE_TYPE_3D);

	BIND_ENUM_CONSTANT(TEXTURE_FLAG_MIPMAPS);
	BIND_ENUM_CONSTANT(TEXTURE_FLAG_REPEAT);
	BIND_ENUM_CONSTANT(TEXTURE_FLAG_FILTER);
	BIND_ENUM_CONSTANT(TEXTURE_FLAG_ANISOTROPIC_FILTER);
	BIND_ENUM_CONSTANT(TEXTURE_FLAG_CONVERT_TO_LINEAR);
	BIND_ENUM_CONSTANT(TEXTURE_FLAG_MIRRORED_REPEAT);
	BIND_ENUM_CONSTANT(TEXTURE_FLAG_USED_FOR_STREAMING);
	BIND_ENUM_CONSTANT(TEXTURE_FLAGS_DEFAULT);

	BIND_ENUM_CONSTANT(SHADER_CANVAS_ITEM);
	BIND_ENUM_CONSTANT(SHADER_MAX);

	BIND_ENUM_CONSTANT(ARRAY_VERTEX);
	BIND_ENUM_CONSTANT(ARRAY_NORMAL);
	BIND_ENUM_CONSTANT(ARRAY_TANGENT);
	BIND_ENUM_CONSTANT(ARRAY_COLOR);
	BIND_ENUM_CONSTANT(ARRAY_TEX_UV);
	BIND_ENUM_CONSTANT(ARRAY_TEX_UV2);
	BIND_ENUM_CONSTANT(ARRAY_INDEX);
	BIND_ENUM_CONSTANT(ARRAY_MAX);

	BIND_ENUM_CONSTANT(ARRAY_FORMAT_VERTEX);
	BIND_ENUM_CONSTANT(ARRAY_FORMAT_NORMAL);
	BIND_ENUM_CONSTANT(ARRAY_FORMAT_TANGENT);
	BIND_ENUM_CONSTANT(ARRAY_FORMAT_COLOR);
	BIND_ENUM_CONSTANT(ARRAY_FORMAT_TEX_UV);
	BIND_ENUM_CONSTANT(ARRAY_FORMAT_TEX_UV2);
	BIND_ENUM_CONSTANT(ARRAY_FORMAT_INDEX);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_VERTEX);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_NORMAL);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_TANGENT);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_COLOR);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_TEX_UV);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_TEX_UV2);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_INDEX);
	BIND_ENUM_CONSTANT(ARRAY_FLAG_USE_2D_VERTICES);
	BIND_ENUM_CONSTANT(ARRAY_COMPRESS_DEFAULT);

	BIND_ENUM_CONSTANT(PRIMITIVE_POINTS);
	BIND_ENUM_CONSTANT(PRIMITIVE_LINES);
	BIND_ENUM_CONSTANT(PRIMITIVE_LINE_STRIP);
	BIND_ENUM_CONSTANT(PRIMITIVE_LINE_LOOP);
	BIND_ENUM_CONSTANT(PRIMITIVE_TRIANGLES);
	BIND_ENUM_CONSTANT(PRIMITIVE_TRIANGLE_STRIP);
	BIND_ENUM_CONSTANT(PRIMITIVE_TRIANGLE_FAN);
	BIND_ENUM_CONSTANT(PRIMITIVE_MAX);

	BIND_ENUM_CONSTANT(BLEND_SHAPE_MODE_NORMALIZED);
	BIND_ENUM_CONSTANT(BLEND_SHAPE_MODE_RELATIVE);

	BIND_ENUM_CONSTANT(VIEWPORT_UPDATE_DISABLED);
	BIND_ENUM_CONSTANT(VIEWPORT_UPDATE_ONCE);
	BIND_ENUM_CONSTANT(VIEWPORT_UPDATE_WHEN_VISIBLE);
	BIND_ENUM_CONSTANT(VIEWPORT_UPDATE_ALWAYS);

	BIND_ENUM_CONSTANT(VIEWPORT_CLEAR_ALWAYS);
	BIND_ENUM_CONSTANT(VIEWPORT_CLEAR_NEVER);
	BIND_ENUM_CONSTANT(VIEWPORT_CLEAR_ONLY_NEXT_FRAME);

	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_DISABLED);
	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_2X);
	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_4X);
	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_8X);
	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_16X);
	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_EXT_2X);
	BIND_ENUM_CONSTANT(VIEWPORT_MSAA_EXT_4X);

	BIND_ENUM_CONSTANT(VIEWPORT_USAGE_2D);
	BIND_ENUM_CONSTANT(VIEWPORT_USAGE_2D_NO_SAMPLING);

	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_OBJECTS_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_VERTICES_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_MATERIAL_CHANGES_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_SHADER_CHANGES_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_SURFACE_CHANGES_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_DRAW_CALLS_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_2D_ITEMS_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_2D_DRAW_CALLS_IN_FRAME);
	BIND_ENUM_CONSTANT(VIEWPORT_RENDER_INFO_MAX);

	BIND_ENUM_CONSTANT(VIEWPORT_DEBUG_DRAW_DISABLED);
	BIND_ENUM_CONSTANT(VIEWPORT_DEBUG_DRAW_UNSHADED);
	BIND_ENUM_CONSTANT(VIEWPORT_DEBUG_DRAW_OVERDRAW);
	BIND_ENUM_CONSTANT(VIEWPORT_DEBUG_DRAW_WIREFRAME);

	BIND_ENUM_CONSTANT(INSTANCE_NONE);
	BIND_ENUM_CONSTANT(INSTANCE_MESH);
	BIND_ENUM_CONSTANT(INSTANCE_MULTIMESH);
	BIND_ENUM_CONSTANT(INSTANCE_MAX);
	BIND_ENUM_CONSTANT(INSTANCE_GEOMETRY_MASK);

	BIND_ENUM_CONSTANT(INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE);
	BIND_ENUM_CONSTANT(INSTANCE_FLAG_MAX);

	BIND_ENUM_CONSTANT(NINE_PATCH_STRETCH);
	BIND_ENUM_CONSTANT(NINE_PATCH_TILE);
	BIND_ENUM_CONSTANT(NINE_PATCH_TILE_FIT);

	BIND_ENUM_CONSTANT(INFO_OBJECTS_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_VERTICES_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_MATERIAL_CHANGES_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_SHADER_CHANGES_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_SHADER_COMPILES_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_SURFACE_CHANGES_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_DRAW_CALLS_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_2D_ITEMS_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_2D_DRAW_CALLS_IN_FRAME);
	BIND_ENUM_CONSTANT(INFO_USAGE_VIDEO_MEM_TOTAL);
	BIND_ENUM_CONSTANT(INFO_VIDEO_MEM_USED);
	BIND_ENUM_CONSTANT(INFO_TEXTURE_MEM_USED);
	BIND_ENUM_CONSTANT(INFO_VERTEX_MEM_USED);

	BIND_ENUM_CONSTANT(FEATURE_SHADERS);
	BIND_ENUM_CONSTANT(FEATURE_MULTITHREADED);

	BIND_ENUM_CONSTANT(MULTIMESH_TRANSFORM_2D);
	BIND_ENUM_CONSTANT(MULTIMESH_TRANSFORM_3D);
	BIND_ENUM_CONSTANT(MULTIMESH_COLOR_NONE);
	BIND_ENUM_CONSTANT(MULTIMESH_COLOR_8BIT);
	BIND_ENUM_CONSTANT(MULTIMESH_COLOR_FLOAT);
	BIND_ENUM_CONSTANT(MULTIMESH_CUSTOM_DATA_NONE);
	BIND_ENUM_CONSTANT(MULTIMESH_CUSTOM_DATA_8BIT);
	BIND_ENUM_CONSTANT(MULTIMESH_CUSTOM_DATA_FLOAT);
	BIND_ENUM_CONSTANT(MULTIMESH_INTERP_QUALITY_FAST);
	BIND_ENUM_CONSTANT(MULTIMESH_INTERP_QUALITY_HIGH);

	BIND_ENUM_CONSTANT(CHANGED_PRIORITY_ANY);
	BIND_ENUM_CONSTANT(CHANGED_PRIORITY_LOW);
	BIND_ENUM_CONSTANT(CHANGED_PRIORITY_HIGH);

	ADD_SIGNAL(MethodInfo("frame_pre_draw"));
	ADD_SIGNAL(MethodInfo("frame_post_draw"));
}

void RenderingServer::_canvas_item_add_style_box(RID p_item, const Rect2 &p_rect, const Rect2 &p_source, RID p_texture, const Vector<float> &p_margins, const Color &p_modulate) {
	ERR_FAIL_COND(p_margins.size() != 4);
	//canvas_item_add_style_box(p_item,p_rect,p_source,p_texture,Vector2(p_margins[0],p_margins[1]),Vector2(p_margins[2],p_margins[3]),true,p_modulate);
}

void RenderingServer::mesh_add_surface_from_mesh_data(RID p_mesh, const Geometry::MeshData &p_mesh_data) {
	PoolVector<Vector3> vertices;
	PoolVector<Vector3> normals;

	for (int i = 0; i < p_mesh_data.faces.size(); i++) {
		const Geometry::MeshData::Face &f = p_mesh_data.faces[i];

		for (int j = 2; j < f.indices.size(); j++) {
#define _ADD_VERTEX(m_idx)                                      \
	vertices.push_back(p_mesh_data.vertices[f.indices[m_idx]]); \
	normals.push_back(f.plane.normal);

			_ADD_VERTEX(0);
			_ADD_VERTEX(j - 1);
			_ADD_VERTEX(j);
		}
	}

	Array d;
	d.resize(RS::ARRAY_MAX);
	d[ARRAY_VERTEX] = vertices;
	d[ARRAY_NORMAL] = normals;
	mesh_add_surface_from_arrays(p_mesh, PRIMITIVE_TRIANGLES, d);
}

void RenderingServer::mesh_add_surface_from_planes(RID p_mesh, const PoolVector<Plane> &p_planes) {
	Geometry::MeshData mdata = Geometry::build_convex_mesh(p_planes);
	mesh_add_surface_from_mesh_data(p_mesh, mdata);
}

bool RenderingServer::is_render_loop_enabled() const {
	return render_loop_enabled;
}

void RenderingServer::set_render_loop_enabled(bool p_enabled) {
	render_loop_enabled = p_enabled;
}

#ifdef DEBUG_ENABLED
bool RenderingServer::is_force_shader_fallbacks_enabled() const {
	return force_shader_fallbacks;
}

void RenderingServer::set_force_shader_fallbacks_enabled(bool p_enabled) {
	force_shader_fallbacks = p_enabled;
}
#endif

RenderingServer::RenderingServer() {
	//ERR_FAIL_COND(singleton);
	singleton = this;

	GLOBAL_DEF_RST("rendering/vram_compression/import_bptc", false);
	GLOBAL_DEF_RST("rendering/vram_compression/import_s3tc", true);
	GLOBAL_DEF_RST("rendering/vram_compression/import_etc", false);
	GLOBAL_DEF_RST("rendering/vram_compression/import_etc2", true);
	GLOBAL_DEF_RST("rendering/vram_compression/import_pvrtc", false);

	GLOBAL_DEF("rendering/misc/lossless_compression/force_png", false);
	GLOBAL_DEF("rendering/misc/lossless_compression/webp_compression_level", 2);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/misc/lossless_compression/webp_compression_level", PropertyInfo(Variant::INT, "rendering/misc/lossless_compression/webp_compression_level", PROPERTY_HINT_RANGE, "0,9,1"));

	GLOBAL_DEF("rendering/limits/time/time_rollover_secs", 3600);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/limits/time/time_rollover_secs", PropertyInfo(Variant::REAL, "rendering/limits/time/time_rollover_secs", PROPERTY_HINT_RANGE, "0,10000,1,or_greater"));

	GLOBAL_DEF("rendering/quality/shadows/filter_mode", 1);
	GLOBAL_DEF("rendering/quality/shadows/filter_mode.mobile", 0);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/quality/shadows/filter_mode", PropertyInfo(Variant::INT, "rendering/quality/shadows/filter_mode", PROPERTY_HINT_ENUM, "Disabled,PCF5,PCF13"));

	GLOBAL_DEF("rendering/quality/reflections/texture_array_reflections", true);
	GLOBAL_DEF("rendering/quality/reflections/texture_array_reflections.mobile", false);
	GLOBAL_DEF("rendering/quality/reflections/high_quality_ggx", true);
	GLOBAL_DEF("rendering/quality/reflections/high_quality_ggx.mobile", false);
	GLOBAL_DEF("rendering/quality/reflections/irradiance_max_size", 128);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/quality/reflections/irradiance_max_size", PropertyInfo(Variant::INT, "rendering/quality/reflections/irradiance_max_size", PROPERTY_HINT_RANGE, "32,2048"));

	GLOBAL_DEF("rendering/quality/shading/force_vertex_shading", false);
	GLOBAL_DEF("rendering/quality/shading/force_vertex_shading.mobile", true);
	GLOBAL_DEF("rendering/quality/shading/force_lambert_over_burley", false);
	GLOBAL_DEF("rendering/quality/shading/force_lambert_over_burley.mobile", true);
	GLOBAL_DEF("rendering/quality/shading/force_blinn_over_ggx", false);
	GLOBAL_DEF("rendering/quality/shading/force_blinn_over_ggx.mobile", true);

	GLOBAL_DEF_RST("rendering/misc/mesh_storage/split_stream", false);

	GLOBAL_DEF_RST("rendering/quality/shading/use_physical_light_attenuation", false);

	GLOBAL_DEF("rendering/quality/depth_prepass/enable", true);
	GLOBAL_DEF("rendering/quality/depth_prepass/disable_for_vendors", "PowerVR,Mali,Adreno,Apple");

	GLOBAL_DEF("rendering/quality/filters/anisotropic_filter_level", 4);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/quality/filters/anisotropic_filter_level", PropertyInfo(Variant::INT, "rendering/quality/filters/anisotropic_filter_level", PROPERTY_HINT_RANGE, "1,16,1"));
	GLOBAL_DEF("rendering/quality/filters/use_nearest_mipmap_filter", false);

	GLOBAL_DEF("rendering/quality/skinning/software_skinning_fallback", true);
	GLOBAL_DEF("rendering/quality/skinning/force_software_skinning", false);

	const char *sz_balance_render_tree = "rendering/quality/spatial_partitioning/render_tree_balance";
	GLOBAL_DEF(sz_balance_render_tree, 0.0f);
	ProjectSettings::get_singleton()->set_custom_property_info(sz_balance_render_tree, PropertyInfo(Variant::REAL, sz_balance_render_tree, PROPERTY_HINT_RANGE, "0.0,1.0,0.01"));

	GLOBAL_DEF_RST("rendering/2d/options/use_software_skinning", true);
	GLOBAL_DEF_RST("rendering/2d/options/ninepatch_mode", 1);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/2d/options/ninepatch_mode", PropertyInfo(Variant::INT, "rendering/2d/options/ninepatch_mode", PROPERTY_HINT_ENUM, "Fixed,Scaling"));

	GLOBAL_DEF_RST("rendering/2d/opengl/batching_send_null", 0);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/2d/opengl/batching_send_null", PropertyInfo(Variant::INT, "rendering/2d/opengl/batching_send_null", PROPERTY_HINT_ENUM, "Default (On),Off,On"));
	GLOBAL_DEF_RST("rendering/2d/opengl/batching_stream", 0);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/2d/opengl/batching_stream", PropertyInfo(Variant::INT, "rendering/2d/opengl/batching_stream", PROPERTY_HINT_ENUM, "Default (Off),Off,On"));
	GLOBAL_DEF_RST("rendering/2d/opengl/legacy_orphan_buffers", 0);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/2d/opengl/legacy_orphan_buffers", PropertyInfo(Variant::INT, "rendering/2d/opengl/legacy_orphan_buffers", PROPERTY_HINT_ENUM, "Default (On),Off,On"));
	GLOBAL_DEF_RST("rendering/2d/opengl/legacy_stream", 0);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/2d/opengl/legacy_stream", PropertyInfo(Variant::INT, "rendering/2d/opengl/legacy_stream", PROPERTY_HINT_ENUM, "Default (On),Off,On"));

	GLOBAL_DEF("rendering/batching/options/use_batching", true);
	GLOBAL_DEF_RST("rendering/batching/options/use_batching_in_editor", true);
	GLOBAL_DEF("rendering/batching/options/single_rect_fallback", false);
	GLOBAL_DEF("rendering/batching/options/use_multirect", true);
	GLOBAL_DEF("rendering/batching/parameters/max_join_item_commands", 16);
	GLOBAL_DEF("rendering/batching/parameters/colored_vertex_format_threshold", 0.25f);
	GLOBAL_DEF("rendering/batching/lights/scissor_area_threshold", 1.0f);
	GLOBAL_DEF("rendering/batching/lights/max_join_items", 32);
	GLOBAL_DEF("rendering/batching/parameters/batch_buffer_size", 16384);
	GLOBAL_DEF("rendering/batching/parameters/item_reordering_lookahead", 4);
	GLOBAL_DEF("rendering/batching/debug/flash_batching", false);
	GLOBAL_DEF("rendering/batching/debug/diagnose_frame", false);
	GLOBAL_DEF("rendering/gles2/compatibility/disable_half_float", false);
	GLOBAL_DEF("rendering/gles2/compatibility/disable_half_float.iOS", true);
	GLOBAL_DEF("rendering/gles2/compatibility/enable_high_float.Android", false);
	GLOBAL_DEF("rendering/batching/precision/uv_contract", false);
	GLOBAL_DEF("rendering/batching/precision/uv_contract_amount", 100);

	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/parameters/max_join_item_commands", PropertyInfo(Variant::INT, "rendering/batching/parameters/max_join_item_commands", PROPERTY_HINT_RANGE, "0,65535"));
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/parameters/colored_vertex_format_threshold", PropertyInfo(Variant::REAL, "rendering/batching/parameters/colored_vertex_format_threshold", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"));
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/parameters/batch_buffer_size", PropertyInfo(Variant::INT, "rendering/batching/parameters/batch_buffer_size", PROPERTY_HINT_RANGE, "8192,65536,1024"));
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/lights/scissor_area_threshold", PropertyInfo(Variant::REAL, "rendering/batching/lights/scissor_area_threshold", PROPERTY_HINT_RANGE, "0.0,1.0"));
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/lights/max_join_items", PropertyInfo(Variant::INT, "rendering/batching/lights/max_join_items", PROPERTY_HINT_RANGE, "0,512"));
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/parameters/item_reordering_lookahead", PropertyInfo(Variant::INT, "rendering/batching/parameters/item_reordering_lookahead", PROPERTY_HINT_RANGE, "0,256"));
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/batching/precision/uv_contract_amount", PropertyInfo(Variant::INT, "rendering/batching/precision/uv_contract_amount", PROPERTY_HINT_RANGE, "0,10000"));

	// Portal rendering settings
	GLOBAL_DEF("rendering/portals/pvs/use_simple_pvs", false);
	GLOBAL_DEF("rendering/portals/pvs/pvs_logging", false);
	GLOBAL_DEF("rendering/portals/gameplay/use_signals", true);
	GLOBAL_DEF("rendering/portals/optimize/remove_danglers", true);
	GLOBAL_DEF("rendering/portals/debug/logging", true);
	GLOBAL_DEF("rendering/portals/advanced/flip_imported_portals", false);

	// Occlusion culling
	GLOBAL_DEF("rendering/misc/occlusion_culling/max_active_spheres", 8);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/misc/occlusion_culling/max_active_spheres", PropertyInfo(Variant::INT, "rendering/misc/occlusion_culling/max_active_spheres", PROPERTY_HINT_RANGE, "0,64"));
	GLOBAL_DEF("rendering/misc/occlusion_culling/max_active_polygons", 8);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/misc/occlusion_culling/max_active_polygons", PropertyInfo(Variant::INT, "rendering/misc/occlusion_culling/max_active_polygons", PROPERTY_HINT_RANGE, "0,64"));

	// Async. compilation and caching
#ifdef DEBUG_ENABLED
	if (!Engine::get_singleton()->is_editor_hint()) {
		force_shader_fallbacks = GLOBAL_GET("rendering/gles3/shaders/debug_shader_fallbacks");
	}
#endif
	GLOBAL_DEF("rendering/gles3/shaders/shader_compilation_mode", 0);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/gles3/shaders/shader_compilation_mode", PropertyInfo(Variant::INT, "rendering/gles3/shaders/shader_compilation_mode", PROPERTY_HINT_ENUM, "Synchronous,Asynchronous,Asynchronous + Cache"));
	GLOBAL_DEF("rendering/gles3/shaders/shader_compilation_mode.mobile", 0);
	GLOBAL_DEF("rendering/gles3/shaders/max_simultaneous_compiles", 2);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/gles3/shaders/max_simultaneous_compiles", PropertyInfo(Variant::INT, "rendering/gles3/shaders/max_simultaneous_compiles", PROPERTY_HINT_RANGE, "1,8,1"));
	GLOBAL_DEF("rendering/gles3/shaders/max_simultaneous_compiles.mobile", 1);
	GLOBAL_DEF("rendering/gles3/shaders/log_active_async_compiles_count", false);
	GLOBAL_DEF("rendering/gles3/shaders/shader_cache_size_mb", 512);
	ProjectSettings::get_singleton()->set_custom_property_info("rendering/gles3/shaders/shader_cache_size_mb", PropertyInfo(Variant::INT, "rendering/gles3/shaders/shader_cache_size_mb", PROPERTY_HINT_RANGE, "128,4096,128"));
	GLOBAL_DEF("rendering/gles3/shaders/shader_cache_size_mb.mobile", 128);
}

RenderingServer::~RenderingServer() {
	singleton = nullptr;
}

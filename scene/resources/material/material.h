#ifndef MATERIAL_H
#define MATERIAL_H

/*  material.h                                                           */


#include "core/object/resource.h"
#include "core/containers/self_list.h"
#include "scene/resources/shader.h"
#include "servers/rendering_server.h"

class Material : public Resource {
	GDCLASS(Material, Resource);
	RES_BASE_EXTENSION("material")
	OBJ_SAVE_TYPE(Material);

	RID material;
	Ref<Material> next_pass;
	int render_priority;

protected:
	_FORCE_INLINE_ RID _get_material() const { return material; }
	static void _bind_methods();
	virtual bool _can_do_next_pass() const { return false; }

	void _validate_property(PropertyInfo &property) const;

public:
	enum {
		RENDER_PRIORITY_MAX = RS::MATERIAL_RENDER_PRIORITY_MAX,
		RENDER_PRIORITY_MIN = RS::MATERIAL_RENDER_PRIORITY_MIN,
	};
	void set_next_pass(const Ref<Material> &p_pass);
	Ref<Material> get_next_pass() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	virtual RID get_rid() const;

	virtual Shader::Mode get_shader_mode() const = 0;
	Material();
	virtual ~Material();
};

#endif

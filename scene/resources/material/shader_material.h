#ifndef SHADER_MATERIAL_H
#define SHADER_MATERIAL_H


/*  material.h                                                           */


#include "core/object/resource.h"
#include "core/containers/self_list.h"

#include "scene/resources/material/material.h"

#include "scene/resources/shader.h"
#include "servers/rendering_server.h"

class Texture;

class ShaderMaterial : public Material {
	GDCLASS(ShaderMaterial, Material);
	Ref<Shader> shader;

protected:
	bool _set(const StringName &p_name, const Variant &p_value);
	bool _get(const StringName &p_name, Variant &r_ret) const;
	void _get_property_list(List<PropertyInfo> *p_list) const;
	bool property_can_revert(const String &p_name);
	Variant property_get_revert(const String &p_name);

	static void _bind_methods();

	void get_argument_options(const StringName &p_function, int p_idx, List<String> *r_options, const String &quote_style) const;

	virtual bool _can_do_next_pass() const;

	void _shader_changed();

public:
	void set_shader(const Ref<Shader> &p_shader);
	Ref<Shader> get_shader() const;

	void set_shader_param(const StringName &p_param, const Variant &p_value);
	Variant get_shader_param(const StringName &p_param) const;

	virtual Shader::Mode get_shader_mode() const;

	ShaderMaterial();
	~ShaderMaterial();
};

#endif

#ifndef SHADERTYPES_H
#define SHADERTYPES_H

/*  shader_types.h                                                       */


#include "core/containers/ordered_hash_map.h"
#include "servers/rendering_server.h"
#include "shader_language.h"

class ShaderTypes {
	struct Type {
		RBMap<StringName, ShaderLanguage::FunctionInfo> functions;
		Vector<StringName> modes;
	};

	RBMap<RS::ShaderMode, Type> shader_modes;

	static ShaderTypes *singleton;

	RBSet<String> shader_types;

public:
	static ShaderTypes *get_singleton() { return singleton; }

	const RBMap<StringName, ShaderLanguage::FunctionInfo> &get_functions(RS::ShaderMode p_mode);
	const Vector<StringName> &get_modes(RS::ShaderMode p_mode);
	const RBSet<String> &get_types();

	ShaderTypes();
};

#endif // SHADERTYPES_H

#ifndef SHADERCOMPILERGLES2_H
#define SHADERCOMPILERGLES2_H

/*  shader_compiler_gles2.h                                              */


#include "core/containers/pair.h"
#include "core/string/string_builder.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_types.h"
#include "servers/rendering_server.h"

class ShaderCompilerGLES2 {
public:
	struct IdentifierActions {
		RBMap<StringName, Pair<int *, int>> render_mode_values;
		RBMap<StringName, bool *> render_mode_flags;
		RBMap<StringName, bool *> usage_flag_pointers;
		RBMap<StringName, bool *> write_flag_pointers;

		RBMap<StringName, ShaderLanguage::ShaderNode::Uniform> *uniforms;
	};

	struct GeneratedCode {
		Vector<CharString> custom_defines;
		Vector<StringName> uniforms;
		Vector<StringName> texture_uniforms;
		Vector<ShaderLanguage::ShaderNode::Uniform::Hint> texture_hints;

		String vertex_global;
		String vertex;
		String fragment_global;
		String fragment;
		String light;

		bool uses_fragment_time;
		bool uses_vertex_time;
	};

private:
	ShaderLanguage parser;

	struct DefaultIdentifierActions {
		RBMap<StringName, String> renames;
		RBMap<StringName, String> render_mode_defines;
		RBMap<StringName, String> usage_defines;
	};

	void _dump_function_deps(const ShaderLanguage::ShaderNode *p_node, const StringName &p_for_func, const RBMap<StringName, String> &p_func_code, StringBuilder &r_to_add, RBSet<StringName> &r_added);
	String _dump_node_code(const ShaderLanguage::Node *p_node, int p_level, GeneratedCode &r_gen_code, IdentifierActions &p_actions, const DefaultIdentifierActions &p_default_actions, bool p_assigning, bool p_use_scope = true);

	const ShaderLanguage::ShaderNode *shader;
	const ShaderLanguage::FunctionNode *function;
	StringName current_func_name;
	StringName vertex_name;
	StringName fragment_name;
	StringName light_name;
	StringName time_name;

	RBSet<StringName> used_name_defines;
	RBSet<StringName> used_flag_pointers;
	RBSet<StringName> used_rmode_defines;
	RBSet<StringName> internal_functions;
	RBSet<StringName> fragment_varyings;

	DefaultIdentifierActions actions[RS::SHADER_MAX];

public:
	Error compile(RS::ShaderMode p_mode, const String &p_code, IdentifierActions *p_actions, const String &p_path, GeneratedCode &r_gen_code);

	ShaderCompilerGLES2();
};

#endif // SHADERCOMPILERGLES2_H

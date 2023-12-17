
/*  shader_types.cpp                                                     */


#include "shader_types.h"

const RBMap<StringName, ShaderLanguage::FunctionInfo> &ShaderTypes::get_functions(RS::ShaderMode p_mode) {
	return shader_modes[p_mode].functions;
}

const Vector<StringName> &ShaderTypes::get_modes(RS::ShaderMode p_mode) {
	return shader_modes[p_mode].modes;
}

const RBSet<String> &ShaderTypes::get_types() {
	return shader_types;
}

ShaderTypes *ShaderTypes::singleton = nullptr;

static ShaderLanguage::BuiltInInfo constt(ShaderLanguage::DataType p_type) {
	return ShaderLanguage::BuiltInInfo(p_type, true);
}

ShaderTypes::ShaderTypes() {
	singleton = this;

	/************ CANVAS ITEM **************************/

	shader_modes[RS::SHADER_CANVAS_ITEM].functions["global"].built_ins["TIME"] = constt(ShaderLanguage::TYPE_FLOAT);

	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["VERTEX"] = ShaderLanguage::TYPE_VEC2;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["UV"] = ShaderLanguage::TYPE_VEC2;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["COLOR"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["MODULATE"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["POINT_SIZE"] = ShaderLanguage::TYPE_FLOAT;

	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["WORLD_MATRIX"] = constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["PROJECTION_MATRIX"] = constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["EXTRA_MATRIX"] = constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["INSTANCE_CUSTOM"] = constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["AT_LIGHT_PASS"] = constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["TEXTURE_PIXEL_SIZE"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["INSTANCE_ID"] = constt(ShaderLanguage::TYPE_INT);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["VERTEX_ID"] = constt(ShaderLanguage::TYPE_INT);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["vertex"].main_function = true;

	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["FRAGCOORD"] = constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMAL"] = ShaderLanguage::TYPE_VEC3;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMALMAP"] = ShaderLanguage::TYPE_VEC3;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMALMAP_DEPTH"] = ShaderLanguage::TYPE_FLOAT;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["UV"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["COLOR"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["MODULATE"] = constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["TEXTURE"] = constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["TEXTURE_PIXEL_SIZE"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMAL_TEXTURE"] = constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SCREEN_UV"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SCREEN_PIXEL_SIZE"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["POINT_COORD"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["AT_LIGHT_PASS"] = constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SCREEN_TEXTURE"] = constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].can_discard = true;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["fragment"].main_function = true;

	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["FRAGCOORD"] = constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["NORMAL"] = constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["UV"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["COLOR"] = constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["MODULATE"] = constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["TEXTURE"] = constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["TEXTURE_PIXEL_SIZE"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["SCREEN_UV"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_VEC"] = ShaderLanguage::TYPE_VEC2;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["SHADOW_VEC"] = ShaderLanguage::TYPE_VEC2;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_HEIGHT"] = ShaderLanguage::TYPE_FLOAT;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_COLOR"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_UV"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["SHADOW_COLOR"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].built_ins["POINT_COORD"] = constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].can_discard = true;
	shader_modes[RS::SHADER_CANVAS_ITEM].functions["light"].main_function = true;

	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("skip_vertex_transform");

	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("blend_mix");
	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("blend_add");
	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("blend_sub");
	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("blend_mul");
	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("blend_premul_alpha");
	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("blend_disabled");

	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("unshaded");
	shader_modes[RS::SHADER_CANVAS_ITEM].modes.push_back("light_only");

	shader_types.insert("canvas_item");
}

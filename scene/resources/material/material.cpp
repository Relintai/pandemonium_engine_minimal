
/*  material.cpp                                                         */


#include "material.h"

void Material::set_next_pass(const Ref<Material> &p_pass) {
	for (Ref<Material> pass_child = p_pass; pass_child != nullptr; pass_child = pass_child->get_next_pass()) {
		ERR_FAIL_COND_MSG(pass_child == this, "Can't set as next_pass one of its parents to prevent crashes due to recursive loop.");
	}

	if (next_pass == p_pass) {
		return;
	}

	next_pass = p_pass;
	RID next_pass_rid;
	if (next_pass.is_valid()) {
		next_pass_rid = next_pass->get_rid();
	}
	RS::get_singleton()->material_set_next_pass(material, next_pass_rid);
}

Ref<Material> Material::get_next_pass() const {
	return next_pass;
}

void Material::set_render_priority(int p_priority) {
	ERR_FAIL_COND(p_priority < RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(p_priority > RENDER_PRIORITY_MAX);
	render_priority = p_priority;
	RS::get_singleton()->material_set_render_priority(material, p_priority);
}

int Material::get_render_priority() const {
	return render_priority;
}

RID Material::get_rid() const {
	return material;
}
void Material::_validate_property(PropertyInfo &property) const {
	if (!_can_do_next_pass() && property.name == "next_pass") {
		property.usage = 0;
	}
}

void Material::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_next_pass", "next_pass"), &Material::set_next_pass);
	ClassDB::bind_method(D_METHOD("get_next_pass"), &Material::get_next_pass);

	ClassDB::bind_method(D_METHOD("set_render_priority", "priority"), &Material::set_render_priority);
	ClassDB::bind_method(D_METHOD("get_render_priority"), &Material::get_render_priority);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "render_priority", PROPERTY_HINT_RANGE, itos(RENDER_PRIORITY_MIN) + "," + itos(RENDER_PRIORITY_MAX) + ",1"), "set_render_priority", "get_render_priority");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "next_pass", PROPERTY_HINT_RESOURCE_TYPE, "Material"), "set_next_pass", "get_next_pass");

	BIND_CONSTANT(RENDER_PRIORITY_MAX);
	BIND_CONSTANT(RENDER_PRIORITY_MIN);
}

Material::Material() {
	material = RID_PRIME(RenderingServer::get_singleton()->material_create());
	render_priority = 0;
}

Material::~Material() {
	RenderingServer::get_singleton()->free(material);
}

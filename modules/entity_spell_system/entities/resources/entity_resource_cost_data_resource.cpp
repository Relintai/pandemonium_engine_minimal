/*
Copyright (c) 2019-2023 Péter Magyar

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "entity_resource_cost_data_resource.h"

Ref<EntityResource> EntityResourceCostDataResource::get_entity_resource_data() {
	return _entity_resource_data;
}
void EntityResourceCostDataResource::set_entity_resource_data(Ref<EntityResource> data) {
	_entity_resource_data = data;
}

EntityResourceCostDataResource::EntityResourceCostDataResource() {
}

void EntityResourceCostDataResource::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_entity_resource_data"), &EntityResourceCostDataResource::get_entity_resource_data);
	ClassDB::bind_method(D_METHOD("set_entity_resource_data", "value"), &EntityResourceCostDataResource::set_entity_resource_data);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "entity_resource_data", PROPERTY_HINT_RESOURCE_TYPE, "EntityResource"), "set_entity_resource_data", "get_entity_resource_data");
}

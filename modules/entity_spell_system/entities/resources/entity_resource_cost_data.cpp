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

#include "entity_resource_cost_data.h"

int EntityResourceCostData::get_cost() {
	return _cost;
}
void EntityResourceCostData::set_cost(int value) {
	_cost = value;
}

EntityResourceCostData::EntityResourceCostData() {
	_cost = 0;
}

void EntityResourceCostData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_cost"), &EntityResourceCostData::get_cost);
	ClassDB::bind_method(D_METHOD("set_cost", "value"), &EntityResourceCostData::set_cost);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cost"), "set_cost", "get_cost");
}

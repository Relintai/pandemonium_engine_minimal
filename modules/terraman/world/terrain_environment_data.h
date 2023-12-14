#ifndef TERRAIN_ENVIRONMENT_DATA_H
#define TERRAIN_ENVIRONMENT_DATA_H
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

#include "core/math/color.h"
#include "core/object/resource.h"

#include "../defines.h"

#include "scene/3d/light.h"

#include "scene/3d/world_environment_3d.h"
#include "scene/main/node.h"

class TerrainEnvironmentData : public Resource {
	GDCLASS(TerrainEnvironmentData, Resource);

public:
	Ref<Environment3D> get_environment();
	void set_environment(const Ref<Environment3D> &value);

	Color get_color(const int index);
	void set_color(const int index, const Color &value);
	float get_energy(const int index);
	void set_energy(const int index, const float value);
	float get_indirect_energy(const int index);
	void set_indirect_energy(const int index, const float value);

	void setup(WorldEnvironment3D *world_environment, DirectionalLight *primary_light, DirectionalLight *secondary_light);
	void setup_bind(Node *world_environment, Node *primary_light, Node *secondary_light);

	TerrainEnvironmentData();
	~TerrainEnvironmentData();

public:
	enum {
		LIGHT_COUNT = 2,
	};

protected:
	static void _bind_methods();

private:
	Ref<Environment3D> _environment;

	Color _colors[LIGHT_COUNT];
	float _energies[LIGHT_COUNT];
	float _indirect_energies[LIGHT_COUNT];
};

#endif

#ifndef VOXEL_JOB_H
#define VOXEL_JOB_H
/*
Copyright (c) 2019-2020 Péter Magyar

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

#include "scene/resources/texture.h"

#include "core/os/thread_pool_job.h"

#include "../../defines.h"

class VoxelChunk;

class VoxelJob : public ThreadPoolJob {
	GDCLASS(VoxelJob, ThreadPoolJob);

public:
	static const String BINDING_STRING_ACTIVE_BUILD_PHASE_TYPE;

	enum ActiveBuildPhaseType {
		BUILD_PHASE_TYPE_NORMAL = 0,
		BUILD_PHASE_TYPE_PROCESS,
		BUILD_PHASE_TYPE_PHYSICS_PROCESS,
	};

public:
	ActiveBuildPhaseType get_build_phase_type();
	void set_build_phase_type(VoxelJob::ActiveBuildPhaseType build_phase_type);

	void set_chunk(const Ref<VoxelChunk> &chunk);

	int get_phase();
	void set_phase(const int phase);
	void next_phase();

	bool get_build_done();
	void set_build_done(const bool val);

	void next_job();

	void reset();
	virtual void _reset();

	void _execute();

	void execute_phase();
	virtual void _execute_phase();

	void process(const float delta);
	void physics_process(const float delta);

	void generate_ao();
	void generate_random_ao(int seed, int octaves = 4, int period = 30, float persistence = 0.3, float scale_factor = 0.6);
	Array merge_mesh_array(Array arr) const;
	Array bake_mesh_array_uv(Array arr, Ref<Texture> tex, float mul_color = 0.7) const;

	void chunk_exit_tree();

	VoxelJob();
	~VoxelJob();

protected:
	static void _bind_methods();

	ActiveBuildPhaseType _build_phase_type;
	bool _build_done;
	int _phase;
	bool _in_tree;
	Ref<VoxelChunk> _chunk;
};

VARIANT_ENUM_CAST(VoxelJob::ActiveBuildPhaseType);

#endif

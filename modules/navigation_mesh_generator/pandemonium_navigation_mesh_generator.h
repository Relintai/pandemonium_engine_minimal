#ifndef PANDEMONIUM_NAVIGATION_MESH_GENERATOR_H
#define PANDEMONIUM_NAVIGATION_MESH_GENERATOR_H

/**************************************************************************/
/*  godot_navigation_mesh_generator.h                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Pandemonium Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "servers/navigation/navigation_mesh_generator.h"

#include "core/os/thread_pool.h"
#include "core/os/thread_pool_job.h"

#include "core/object/func_ref.h"

class NavigationPolygon;
class NavigationMeshSourceGeometryData2D;
class NavigationGeometryParser2D;
class NavigationMeshSourceGeometryData3D;
class NavigationGeometryParser3D;

class PandemoniumNavigationMeshGenerator : public NavigationMeshGenerator {
	GDCLASS(PandemoniumNavigationMeshGenerator, NavigationMeshGenerator);

public:
	// =======   TASKS   =======

	class NavigationGeneratorTask2D : public ThreadPoolJob {
		GDCLASS(NavigationGeneratorTask2D, ThreadPoolJob);

	public:
		enum TaskStatus {
			PARSING_REQUIRED,
			PARSING_STARTED,
			PARSING_FINISHED,
			PARSING_FAILED,
			BAKING_STARTED,
			BAKING_FINISHED,
			BAKING_FAILED,
		};

		Ref<NavigationPolygon> navigation_polygon;
		ObjectID parse_root_object_id;
		Ref<NavigationMeshSourceGeometryData2D> source_geometry_data;
		Ref<FuncRef> callback;
		NavigationGeneratorTask2D::TaskStatus status = NavigationGeneratorTask2D::TaskStatus::PARSING_REQUIRED;
		LocalVector<Ref<NavigationGeometryParser2D>> geometry_parsers;

		void call_callback();

		void _execute();

	protected:
		static void _bind_methods();
	};

	// =======   TASKS END  =======

public:
	virtual void process();
	virtual void cleanup();

	// 2D ////////////////////////////////////
	virtual void register_geometry_parser_2d(Ref<NavigationGeometryParser2D> p_geometry_parser);
	virtual void unregister_geometry_parser_2d(Ref<NavigationGeometryParser2D> p_geometry_parser);

	virtual Ref<NavigationMeshSourceGeometryData2D> parse_2d_source_geometry_data(Ref<NavigationPolygon> p_navigation_polygon, Node *p_root_node, Ref<FuncRef> p_callback = Ref<FuncRef>());
	virtual void bake_2d_from_source_geometry_data(Ref<NavigationPolygon> p_navigation_polygon, Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Ref<FuncRef> p_callback = Ref<FuncRef>());

	virtual void parse_and_bake_2d(Ref<NavigationPolygon> p_navigation_polygon, Node *p_root_node, Ref<FuncRef> p_callback = Ref<FuncRef>());

	static void _static_parse_2d_geometry_node(Ref<NavigationPolygon> p_navigation_polygon, Node *p_node, Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, bool p_recurse_children, LocalVector<Ref<NavigationGeometryParser2D>> &p_geometry_2d_parsers);
	static void _static_parse_2d_source_geometry_data(Ref<NavigationPolygon> p_navigation_polygon, Node *p_root_node, Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, LocalVector<Ref<NavigationGeometryParser2D>> &p_geometry_2d_parsers);
	static void _static_bake_2d_from_source_geometry_data(Ref<NavigationPolygon> p_navigation_polygon, Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data);

	virtual bool is_navigation_polygon_baking(Ref<NavigationPolygon> p_navigation_polygon) const;

	PandemoniumNavigationMeshGenerator();
	~PandemoniumNavigationMeshGenerator();

private:
	void _process_2d_tasks();
	void _process_2d_parse_tasks();
	void _process_2d_bake_cleanup_tasks();

private:
	Mutex _generator_mutex;

	bool _use_thread_pool = true;
	// TODO implement support into ThreadPool
	//bool _baking_use_high_priority_threads = true;

	LocalVector<Ref<NavigationGeometryParser2D>> _geometry_2d_parsers;

	LocalVector<Ref<NavigationPolygon>> _baking_navigation_polygons;
	LocalVector<Ref<NavigationGeneratorTask2D>> _2d_parse_jobs;
	LocalVector<Ref<NavigationGeneratorTask2D>> _2d_running_jobs;
};

#endif // GODOT_NAVIGATION_MESH_GENERATOR_H

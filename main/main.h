#ifndef MAIN_H
#define MAIN_H

/*  main.h                                                               */


#include "core/error/error_list.h"
#include "core/os/thread.h"
#include "core/typedefs.h"

class Main {
	static void print_help(const char *p_binary);
	static uint64_t last_ticks;
	static uint32_t hide_print_fps_attempts;
	static uint32_t frames;
	static uint32_t frame;
	static bool force_redraw_requested;
	static int iterating;
	static bool agile_input_event_flushing;

public:
	static bool is_project_manager();

	static Error setup(const char *execpath, int argc, char *argv[], bool p_second_phase = true);
	static Error setup2(Thread::ID p_main_tid_override = 0);
	static bool start();

	static bool iteration();
	static void force_redraw();

	static bool is_iterating();

	static void cleanup(bool p_force = false);
};

#endif // MAIN_H

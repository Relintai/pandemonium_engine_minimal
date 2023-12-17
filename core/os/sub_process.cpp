
/*  file_access.cpp                                                      */


#include "sub_process.h"

SubProcess::CreateFunc SubProcess::create_func = NULL;

SubProcess *SubProcess::create() {
	SubProcess *ret = create_func();
	return ret;
}

String SubProcess::get_executable_path() const {
	return _executable_path;
}
void SubProcess::set_executable_path(const String &p_executable_path) {
	ERR_FAIL_COND(is_process_running());

	_executable_path = p_executable_path;
}

Vector<String> SubProcess::get_arguments() const {
	return _arguments;
}
void SubProcess::set_arguments(const Vector<String> &p_arguments) {
	ERR_FAIL_COND(is_process_running());

	_arguments = p_arguments;
}

bool SubProcess::get_blocking() const {
	return _blocking;
}
void SubProcess::set_blocking(const bool p_value) {
	ERR_FAIL_COND(is_process_running());

	_blocking = p_value;
}

bool SubProcess::get_read_output() const {
	return _read_output;
}
void SubProcess::set_read_output(const bool p_value) {
	ERR_FAIL_COND(is_process_running());

	_read_output = p_value;
}

bool SubProcess::get_read_std() const {
	return _read_std;
}
void SubProcess::set_read_std(const bool p_value) {
	ERR_FAIL_COND(is_process_running());

	_read_std = p_value;
}

bool SubProcess::get_read_std_err() const {
	return _read_std_err;
}
void SubProcess::set_read_std_err(const bool p_value) {
	ERR_FAIL_COND(is_process_running());

	_read_std_err = p_value;
}

bool SubProcess::get_use_pipe_mutex() const {
	return _use_pipe_mutex;
}
void SubProcess::set_use_pipe_mutex(const bool p_value) {
	ERR_FAIL_COND(is_process_running());

	_use_pipe_mutex = p_value;
}

bool SubProcess::get_open_console() const {
	return _open_console;
}
void SubProcess::set_open_console(const bool p_value) {
	ERR_FAIL_COND(is_process_running());

	_open_console = p_value;
}

Error SubProcess::run(const String &p_executable_path, const Vector<String> &p_arguments, bool p_output, bool p_blocking, bool p_read_std_err, bool p_use_pipe_mutex, bool p_open_console) {
	if (is_process_running()) {
		return ERR_ALREADY_IN_USE;
	}

	String _executable_path = p_executable_path;
	Vector<String> _arguments = p_arguments;

	_blocking = p_blocking;

	_read_output = p_output;

	_read_std = true;
	_read_std_err = p_read_std_err;

	_use_pipe_mutex = p_use_pipe_mutex;

	_open_console = p_open_console;

	_setup_pipe_mutex();

	return start();
}

SubProcess::SubProcess() {
	_blocking = false;

	_read_output = true;

	_read_std = true;
	_read_std_err = false;

	_use_pipe_mutex = false;

	_pipe_mutex = NULL;

	_open_console = false;

	_process_id = ProcessID();
	_exitcode = 0;
};

void SubProcess::_setup_pipe_mutex() {
	if (_use_pipe_mutex) {
		if (!_pipe_mutex) {
			_pipe_mutex = memnew(Mutex);
		}
	} else {
		if (_pipe_mutex) {
			memdelete(_pipe_mutex);
			_pipe_mutex = NULL;
		}
	}
}

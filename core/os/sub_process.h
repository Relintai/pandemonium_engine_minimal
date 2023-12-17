#ifndef SUB_PROCESS_H
#define SUB_PROCESS_H


/*  file_access.h                                                        */


#include "core/containers/list.h"
#include "core/math/math_defs.h"
#include "core/os/memory.h"
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/typedefs.h"

/**
 * Multi-Platform abstraction for running and communicating with sub processes
 */

class SubProcess {
public:
	typedef int64_t ProcessID;

	static SubProcess *create();

	String get_executable_path() const;
	void set_executable_path(const String &p_executable_path);

	Vector<String> get_arguments() const;
	void set_arguments(const Vector<String> &p_arguments);

	bool get_blocking() const;
	void set_blocking(const bool p_value);

	bool get_read_output() const;
	void set_read_output(const bool p_value);

	bool get_read_std() const;
	void set_read_std(const bool p_value);

	bool get_read_std_err() const;
	void set_read_std_err(const bool p_value);

	bool get_use_pipe_mutex() const;
	void set_use_pipe_mutex(const bool p_value);

	bool get_open_console() const;
	void set_open_console(const bool p_value);

	String get_data() const {
		return _pipe;
	}

	int get_process_id() const {
		return _process_id;
	}

	int get_exitcode() const {
		return _exitcode;
	}

	virtual Error start() = 0;
	virtual Error stop() = 0;
	virtual Error poll() = 0;
	virtual Error send_signal(const int p_signal) = 0;
	virtual Error send_data(const String &p_data) = 0;
	virtual bool is_process_running() const = 0;

	Error run(const String &p_executable_path, const Vector<String> &p_arguments, bool p_output = true, bool p_blocking = true, bool p_read_std_err = false, bool p_use_pipe_mutex = false, bool p_open_console = false);

	template <class T>
	static void make_default() {
		create_func = _create_builtin<T>;
	}

	SubProcess();
	virtual ~SubProcess() {}

protected:
	void _setup_pipe_mutex();

	String _executable_path;
	Vector<String> _arguments;

	bool _blocking;

	bool _read_output;

	bool _read_std;
	bool _read_std_err;

	String _pipe;

	bool _use_pipe_mutex;

	Mutex *_pipe_mutex;

	bool _open_console;

	ProcessID _process_id;
	int _exitcode;

public:
	typedef SubProcess *(*CreateFunc)();

private:
	static CreateFunc create_func;
	template <class T>
	static SubProcess *_create_builtin() {
		return memnew(T);
	}
};

struct SubProcessRef {
	SubProcess *f;

	_FORCE_INLINE_ bool is_null() const { return f == nullptr; }
	_FORCE_INLINE_ bool is_valid() const { return f != nullptr; }

	_FORCE_INLINE_ operator bool() const { return f != nullptr; }
	_FORCE_INLINE_ operator SubProcess *() { return f; }

	_FORCE_INLINE_ SubProcess *operator->() {
		return f;
	}

	SubProcessRef(SubProcess *fa) { f = fa; }
	SubProcessRef(SubProcessRef &&other) {
		f = other.f;
		other.f = nullptr;
	}
	~SubProcessRef() {
		if (f) {
			memdelete(f);
		}
	}
};

#endif

#ifndef SUB_PROCESS_WINDOWS_H
#define SUB_PROCESS_WINDOWS_H


/*  dir_access_unix.h                                                    */


#include "core/os/sub_process.h"

#include "os_windows.h"

class SubProcessWindows : public SubProcess {
public:
	virtual Error start();
	virtual Error stop();
	virtual Error poll();
	virtual Error send_signal(const int p_signal);
	virtual Error send_data(const String &p_data);
	virtual bool is_process_running() const;

	SubProcessWindows();
	~SubProcessWindows();

protected:
	String _quote_command_line_argument(const String &p_text) const;
	void _append_to_pipe(char *p_bytes, int p_size);

	struct ProcessInfo {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
	};

	bool _process_started;
	HANDLE _pipe_handles[2];
	ProcessInfo _process_info;
	LocalVector<char> _bytes;
};

#endif

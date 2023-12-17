#ifndef SUB_PROCESS_UNIX_H
#define SUB_PROCESS_UNIX_H


/*  dir_access_unix.h                                                    */


#ifdef UNIX_ENABLED

#include "core/os/sub_process.h"

#include <stdio.h>

class SubProcessUnix : public SubProcess {
public:
	virtual Error start();
	virtual Error stop();
	virtual Error poll();
	virtual Error send_signal(const int p_signal);
	virtual Error send_data(const String &p_data);
	virtual bool is_process_running() const;

	SubProcessUnix();
	~SubProcessUnix();

protected:
	FILE *_process_fp;
	char _process_buf[65535];
};

#endif //UNIX ENABLED
#endif

#ifndef WINDOWS_TERMINAL_LOGGER_H
#define WINDOWS_TERMINAL_LOGGER_H

/*  windows_terminal_logger.h                                            */


#ifdef WINDOWS_ENABLED

#include "core/io/logger.h"

class WindowsTerminalLogger : public StdLogger {
public:
	virtual void logv(const char *p_format, va_list p_list, bool p_err);
	virtual void log_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, ErrorType p_type = ERR_ERROR);
	virtual ~WindowsTerminalLogger();
};

#endif

#endif


#ifndef PLOGGER_H
#define PLOGGER_H

#include "core/object/reference.h"
#include "core/typedefs.h"
#include "core/string/ustring.h"
#include "logger_backend.h"

class String;

#define PLOG_TRACE(str) \
	PLogger::log_trace(__FUNCTION__, __FILE__, __LINE__, str);

#define PLOG_MSG(str) \
	PLogger::log_message(__FUNCTION__, __FILE__, __LINE__, str);

#define PLOG_WARN(str) \
	PLogger::log_warning(__FUNCTION__, __FILE__, __LINE__, str);

#define PLOG_ERR(str) \
	PLogger::log_error(__FUNCTION__, __FILE__, __LINE__, str);

class PLogger : public Object {
public:
	enum LogLevel {
		LOG_LEVEL_TRACE = 0,
		LOG_LEVEL_MESSAGE,
		LOG_LEVEL_WARNING,
		LOG_LEVEL_ERROR,
		LOG_LEVEL_NONE,
	};

	static void log_trace(const String &str);
	static void log_trace(const char *str);
	static void log_trace(const char *p_function, const char *p_file, int p_line, const String &str);
	static void log_trace(const char *p_function, const char *p_file, int p_line, const char *str);

	static void log_message(const String &str);
	static void log_message(const char *str);
	static void log_message(const char *p_function, const char *p_file, int p_line, const String &str);
	static void log_message(const char *p_function, const char *p_file, int p_line, const char *str);

	static void log_warning(const String &str);
	static void log_warning(const char *str);
	static void log_warning(const char *p_function, const char *p_file, int p_line, const String &str);
	static void log_warning(const char *p_function, const char *p_file, int p_line, const char *str);

	static void log_error(const String &str);
	static void log_error(const char *str);
	static void log_error(const char *p_function, const char *p_file, int p_line, const char *str);
	static void log_error(const char *p_function, const char *p_file, int p_line, const String &str);

	static void do_log_trace(const String &str);
	static void do_log_message(const String &str);
	static void do_log_warning(const String &str);
	static void do_log_error(const String &str);
	
	static LogLevel get_log_level();
	static void set_log_level(const LogLevel p_log_level);

	static LogLevel _log_level;
	static Ref<LoggerBackend> _backend;
};

#endif

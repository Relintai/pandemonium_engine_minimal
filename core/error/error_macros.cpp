
/*  error_macros.cpp                                                     */


#include "error_macros.h"

#include "core/io/logger.h"
#include "core/string/ustring.h"
#include "core/os/os.h"

static ErrorHandlerList *error_handler_list = nullptr;

void add_error_handler(ErrorHandlerList *p_handler) {
	_global_lock();
	p_handler->next = error_handler_list;
	error_handler_list = p_handler;
	_global_unlock();
}

void remove_error_handler(ErrorHandlerList *p_handler) {
	_global_lock();

	ErrorHandlerList *prev = nullptr;
	ErrorHandlerList *l = error_handler_list;

	while (l) {
		if (l == p_handler) {
			if (prev) {
				prev->next = l->next;
			} else {
				error_handler_list = l->next;
			}
			break;
		}
		prev = l;
		l = l->next;
	}

	_global_unlock();
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, "", p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), "", p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, ErrorHandlerType p_type) {
	if (OS::get_singleton()) {
		OS::get_singleton()->print_error(p_function, p_file, p_line, p_error, p_message, (Logger::ErrorType)p_type);
	} else {
		// Fallback if errors happen before OS init or after it's destroyed.
		const char *err_details = (p_message && *p_message) ? p_message : p_error;
		fprintf(stderr, "ERROR: %s\n   at: %s (%s:%i)\n", err_details, p_function, p_file, p_line);
	}

	_global_lock();
	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, p_function, p_file, p_line, p_error, p_message, p_type);
		l = l->next;
	}

	_global_unlock();
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const char *p_message, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const String &p_message, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, p_message.utf8().get_data(), p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const String &p_message, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message.utf8().get_data(), p_type);
}

void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const char *p_message, bool fatal) {
	String fstr(fatal ? "FATAL: " : "");
	String err(fstr + "Index " + p_index_str + " = " + itos(p_index) + " is out of bounds (" + p_size_str + " = " + itos(p_size) + ").");
	_err_print_error(p_function, p_file, p_line, err.utf8().get_data(), p_message);
}

void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const String &p_message, bool fatal) {
	_err_print_index_error(p_function, p_file, p_line, p_index, p_size, p_index_str, p_size_str, p_message.utf8().get_data(), fatal);
}

void _err_flush_stdout() {
	fflush(stdout);
}

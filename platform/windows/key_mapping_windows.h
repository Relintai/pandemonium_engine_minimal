#ifndef KEY_MAPPING_WINDOWS_H
#define KEY_MAPPING_WINDOWS_H

/*  key_mapping_windows.h                                                */


#include "core/os/keyboard.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winuser.h>

class KeyMappingWindows {
	KeyMappingWindows(){};

public:
	static unsigned int get_keysym(unsigned int p_code);
	static unsigned int get_scancode(unsigned int p_keycode);
	static unsigned int get_scansym(unsigned int p_code, bool p_extended);
	static bool is_extended_key(unsigned int p_code);
};

#endif // KEY_MAPPING_WINDOWS_H

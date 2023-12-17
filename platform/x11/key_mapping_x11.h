#ifndef KEY_MAPPING_X11_H
#define KEY_MAPPING_X11_H

/*  key_mapping_x11.h                                                    */


#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_XKB_KEYS
#include <X11/keysymdef.h>

#include "core/os/keyboard.h"

class KeyMappingX11 {
	KeyMappingX11(){};

public:
	static unsigned int get_keycode(KeySym p_keysym);
	static unsigned int get_xlibcode(unsigned int p_keysym);
	static unsigned int get_scancode(unsigned int p_code);
	static KeySym get_keysym(unsigned int p_code);
	static unsigned int get_unicode_from_keysym(KeySym p_keysym);
	static KeySym get_keysym_from_unicode(unsigned int p_unicode);
};

#endif


/*
**	Copyright (C) 2005 Marius Elvert
**
**	This software is provided 'as-is', without any express or implied
**	warranty.  In no event will the authors be held liable for any damages
**	arising from the use of this software.
**
**	Permission is granted to anyone to use this software for any purpose,
**	including commercial applications, and to alter it and redistribute it
**	freely, subject to the following restrictions:
**
**	1. The origin of this software must not be misrepresented; you must not
**	   claim that you wrote the original software. If you use this software
**	   in a product, an acknowledgment in the product documentation would be
**	   appreciated but is not required.
**	2. Altered source versions must be plainly marked as such, and must not be
**	   misrepresented as being the original software.
**	3. This notice may not be removed or altered from any source distribution.
**
**	Marius Elvert ( marius@cataclysm-software.net )
**
**
**		filename:		x11_main.hpp
**
**		description:	main header
**		target-system:	X-Windows
**		version-info:   $Id: x11_main.hpp 101 2009-01-26 02:21:58Z ltjax $
**
*/

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <sys/time.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>

namespace glsk { namespace detail {

	/*	Globals
	*/
	struct core
	{
		Display*				connection;
		int						screen;
		Cursor					blank_cursor;
		Atom					object_atom;
		Atom					protocols_atom;
		Atom					delete_atom;
		int						flags;

		XF86VidModeModeInfo 	desktop_video_mode;

		glsk::window*			window_head;
		glsk::window*			window_focus;
		int						window_open_count;

		int						glx_version[ 2 ];

		char*					extensions_buffer;
		int						extensions_count;
		char**					extensions;

		struct timeval			timer_start;

		static inline glsk::config::detail& get_internal( glsk::config& x ) { return *x.data; }
		static inline glsk::window::detail& get_internal( glsk::window& x ) { return *x.data; }

	};

	extern core global;

	enum {
		decode_error = 2,
		vidmode_changed = 4,
		glx_fallback = 8,
		mouse_captured = 16,
		use_vidmode_ext = 32
	};

	void					init_graphics();
	void					init_cursor();
	void					init_input();

	void 					init_module();
	void					free_module();


	int						process_event( glsk::window* window, XEvent* event );
	void					reset_vidmode();

#ifdef _DEBUG

#include <stdio.h>

#define DEBUG_PRINT0( x ) printf( (x) );
#define DEBUG_PRINT1( x, a ) printf( (x), (a) );
#define DEBUG_PRINT2( x, a, b ) printf( (x), (a), (b) );
#define DEBUG_PRINT3( x, a, b, c ) printf( (x), (a), (b), (c) );

#else

#define DEBUG_PRINT0( x )
#define DEBUG_PRINT1( x, a )
#define DEBUG_PRINT2( x, a, b )
#define DEBUG_PRINT3( x, a, b, c )

#endif

#ifdef _GLSK_USE_VIRTUAL_EVENTS_
#  ifdef _GLSK_USE_SIGNALS_
#    define _GLSK_INVOKE_EVENT( CALL ) \
	   this->owner->on_##CALL; \
       signal_##CALL
#  else
#    define _GLSK_INVOKE_EVENT( CALL ) this->owner->on_##CALL
#  endif
#else
#  ifdef _GLSK_USE_SIGNALS_
#    define _GLSK_INVOKE_EVENT( CALL ) signal_##CALL
#  endif
#endif

}}


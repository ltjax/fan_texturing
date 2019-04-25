
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
**		filename:		x11_joystick.hpp
**
**		description:	input related functions
**		target-system:	X-Windows
**		version-info:   $Id: x11_joystick.cpp 101 2009-01-26 02:21:58Z ltjax $
**
*/

// system includes
#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#include <X11/Xdefs.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86vmode.h>

// global include
#include <glsk/glsk.hpp>

#define _glsk_no_keycodes_h_
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/joystick.h>
#include "x11_main.hpp"
#include "x11_window.hpp"
#include "x11_input.hpp"
#include "x11_joystick.hpp"

namespace glsk { namespace detail {

	struct joystick
	{
		int 					fd;
		joystick* 				next;

		explicit				joystick( int fd )
		{
			this->fd = fd;
			
			joystick** hook = &list;
			
			while ( *hook )
				hook = &((*hook)->next);
				
			next = 0;
			*hook = this;
			
			++count;
		}

								~joystick()
		{
			joystick** hook = &list;

			while ( *hook != this )
				hook = &(*hook)->next;

			*hook = this->next;

			--count;

			if ( fd != -1 )
			{
				close( fd );
				fd = -1;
			}
		}

		static joystick* 		list;
		static int				count;

		static void clear_all()
		{
			while ( list )
				delete list;
		}
	};

}}

glsk::detail::joystick* glsk::detail::joystick::list = 0;
int glsk::detail::joystick::count = 0;

void glsk::scan_joysticks()
{
	const char* device_list[] = {
		"/dev/input/js0",
		"/dev/input/js1"
	};

	const int device_count = sizeof( device_list ) / sizeof( device_list[ 0 ] );

	detail::joystick::clear_all();

	for ( int i = 0; i < device_count; ++i )
	{
		int fd = open( device_list[ i ], O_RDONLY | O_NONBLOCK );

		if ( fd == -1 )
		{
			printf( "Unable to open device: %s\n", device_list[ i ] );
			continue;
		}

		new detail::joystick( fd );
	}
}
int	glsk::get_joystick_count()
{
	return detail::joystick::count;
}

void glsk::detail::joystick_free()
{
	joystick::clear_all();
}

void glsk::detail::joystick_update()
{
	struct js_event e;
	unsigned int device_id = 0;

	for ( joystick* i = joystick::list; i; i=i->next, ++device_id )
	{
		while ( read( i->fd, &e, sizeof( struct js_event ) ) > 0 )
		{
			switch ( e.type & ~JS_EVENT_INIT )
			{
			case JS_EVENT_BUTTON:
				{
					glsk::input_event event( device_id, glsk::input_event::joystick_button, e.number, e.value );

#ifdef _GLSK_USE_VIRTUAL_EVENTS_
                    global.window_focus->on_input( event );
#endif
#ifdef _GLSK_USE_SIGNALS_
					core::get_internal( *global.window_focus ).signal_input( event );
#endif
				}
				break;

			case JS_EVENT_AXIS:
				{
					glsk::input_event event( device_id, glsk::input_event::joystick_axis_changed,
                        e.number, ((e.number&1) ? -e.value : e.value)+(1<<15) );

#ifdef _GLSK_USE_VIRTUAL_EVENTS_
                    global.window_focus->on_input( event );
#endif
#ifdef _GLSK_USE_SIGNALS_
					core::get_internal( *global.window_focus ).signal_input( event );
#endif
				}
				break;

			default:
				break;
			};
		}
	}
}

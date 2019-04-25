
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
**		filename:		x_input.c
**
**		description:	input related functions
**		target-system:	X-Windows
**		version-info:   $Id: x11_input.cpp 101 2009-01-26 02:21:58Z ltjax $
**
*/


// system includes
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#include <X11/Xdefs.h>
#include <X11/keysym.h>
#include <X11/extensions/xf86vmode.h>

// global include
#include <glsk/glsk.hpp>

// internal includes
#include "x11_main.hpp"
#include "x11_input.hpp"
#include "x11_window.hpp"
#include "x11_joystick.hpp"

#define _GLSK_MOUSE_BUTTONS 8

namespace glsk { namespace detail {
	static unsigned char keytable[ 2 ][ 256 ];

	unsigned int input_translate_key( KeySym symbol );
}}


/*
	the axial device structure
*/


/*static glsk_axial_device_t* _glsk_mouse = 0;

#define _GLSK_AXIAL_DEVICE_AQUIRED 1
#define _GLSK_AXIAL_DEVICE_READ 4
#define _GLSK_KEYQUEUESIZE 64
#define _GLSK_KEYSTATESIZE 16 // 2 * 256 / 32

static int _glsk_keyqueue_size = 0;


static unsigned int _glsk_keystate[ _GLSK_KEYSTATESIZE ]; // each key uses 2 bits in here
static unsigned int _glsk_keyqueue[ _GLSK_KEYQUEUESIZE ];

#define _GLSK_KEYBOARD_AQUIRED 1

static int _glsk_input_flags = 0;*/

void glsk::detail::input_init_keytable()
{
	// init default
	for ( unsigned int i = 0; i < 256; ++i )
		keytable[ 0 ][ i ] = keytable[ 1 ][ i ] = 0;

	keytable[ 1 ][ 'a' - '\0' ] = keytable[ 1 ][ 'A' - '\0' ] = KEY_A;
	keytable[ 1 ][ 'b' - '\0' ] = keytable[ 1 ][ 'B' - '\0' ] = KEY_B;
	keytable[ 1 ][ 'c' - '\0' ] = keytable[ 1 ][ 'C' - '\0' ] = KEY_C;
	keytable[ 1 ][ 'd' - '\0' ] = keytable[ 1 ][ 'D' - '\0' ] = KEY_D;
	keytable[ 1 ][ 'e' - '\0' ] = keytable[ 1 ][ 'E' - '\0' ] = KEY_E;
	keytable[ 1 ][ 'f' - '\0' ] = keytable[ 1 ][ 'F' - '\0' ] = KEY_F;
	keytable[ 1 ][ 'g' - '\0' ] = keytable[ 1 ][ 'G' - '\0' ] = KEY_G;
	keytable[ 1 ][ 'h' - '\0' ] = keytable[ 1 ][ 'H' - '\0' ] = KEY_H;
	keytable[ 1 ][ 'i' - '\0' ] = keytable[ 1 ][ 'I' - '\0' ] = KEY_I;
	keytable[ 1 ][ 'j' - '\0' ] = keytable[ 1 ][ 'J' - '\0' ] = KEY_J;
	keytable[ 1 ][ 'k' - '\0' ] = keytable[ 1 ][ 'K' - '\0' ] = KEY_K;
	keytable[ 1 ][ 'l' - '\0' ] = keytable[ 1 ][ 'L' - '\0' ] = KEY_L;
	keytable[ 1 ][ 'm' - '\0' ] = keytable[ 1 ][ 'M' - '\0' ] = KEY_M;
	keytable[ 1 ][ 'n' - '\0' ] = keytable[ 1 ][ 'N' - '\0' ] = KEY_N;
	keytable[ 1 ][ 'o' - '\0' ] = keytable[ 1 ][ 'O' - '\0' ] = KEY_O;
	keytable[ 1 ][ 'p' - '\0' ] = keytable[ 1 ][ 'P' - '\0' ] = KEY_P;
	keytable[ 1 ][ 'q' - '\0' ] = keytable[ 1 ][ 'Q' - '\0' ] = KEY_Q;
	keytable[ 1 ][ 'r' - '\0' ] = keytable[ 1 ][ 'R' - '\0' ] = KEY_R;
	keytable[ 1 ][ 's' - '\0' ] = keytable[ 1 ][ 'S' - '\0' ] = KEY_S;
	keytable[ 1 ][ 't' - '\0' ] = keytable[ 1 ][ 'T' - '\0' ] = KEY_T;
	keytable[ 1 ][ 'u' - '\0' ] = keytable[ 1 ][ 'U' - '\0' ] = KEY_U;
	keytable[ 1 ][ 'v' - '\0' ] = keytable[ 1 ][ 'V' - '\0' ] = KEY_V;
	keytable[ 1 ][ 'w' - '\0' ] = keytable[ 1 ][ 'W' - '\0' ] = KEY_W;
	keytable[ 1 ][ 'x' - '\0' ] = keytable[ 1 ][ 'X' - '\0' ] = KEY_X;
	keytable[ 1 ][ 'y' - '\0' ] = keytable[ 1 ][ 'Y' - '\0' ] = KEY_Y;
	keytable[ 1 ][ 'z' - '\0' ] = keytable[ 1 ][ 'Z' - '\0' ] = KEY_Z;

	keytable[ 1 ][ ' ' - '\0' ] = KEY_SPACE;
	keytable[ 1 ][ ';' - '\0' ] = KEY_SEMICOLON;
	keytable[ 1 ][ ':' - '\0' ] = KEY_COLON;
	keytable[ 1 ][ ',' - '\0' ] = KEY_COMMA;
	keytable[ 1 ][ '.' - '\0' ] = KEY_PERIOD;
	keytable[ 1 ][ '/' - '\0' ] = KEY_SLASH;
	keytable[ 1 ][ '-' - '\0' ] = KEY_MINUS;
	keytable[ 1 ][ '=' - '\0' ] = KEY_EQUALS;
	keytable[ 1 ][ '[' - '\0' ] = KEY_LBRACKET;
	keytable[ 1 ][ ']' - '\0' ] = KEY_RBRACKET;
	keytable[ 1 ][ '1' - '\0' ] = KEY_1;
	keytable[ 1 ][ '2' - '\0' ] = KEY_2;
	keytable[ 1 ][ '3' - '\0' ] = KEY_3;
	keytable[ 1 ][ '4' - '\0' ] = KEY_4;
	keytable[ 1 ][ '5' - '\0' ] = KEY_5;
	keytable[ 1 ][ '6' - '\0' ] = KEY_6;
	keytable[ 1 ][ '7' - '\0' ] = KEY_7;
	keytable[ 1 ][ '8' - '\0' ] = KEY_8;
	keytable[ 1 ][ '9' - '\0' ] = KEY_9;
	keytable[ 1 ][ '0' - '\0' ] = KEY_0;

	keytable[ 0 ][XK_BackSpace&0xFF] = KEY_BACK;
	keytable[ 0 ][XK_Tab&0xFF] = KEY_TAB;
	keytable[ 0 ][XK_Clear&0xFF] = KEY_SPACE;
	keytable[ 0 ][XK_Return&0xFF] = KEY_RETURN;
	keytable[ 0 ][XK_Pause&0xFF] = KEY_PAUSE;
	keytable[ 0 ][XK_Escape&0xFF] = KEY_ESCAPE;
	keytable[ 0 ][XK_Delete&0xFF] = KEY_DELETE;

	// the same keys w/ and w/o numlock - screw numlock
	keytable[ 0 ][XK_KP_Insert&0xFF] = keytable[ 0 ][XK_KP_0&0xFF] = KEY_NUMPAD0;
	keytable[ 0 ][XK_KP_End&0xFF] = keytable[ 0 ][XK_KP_1&0xFF] = KEY_NUMPAD1;
	keytable[ 0 ][XK_KP_Down&0xFF] = keytable[ 0 ][XK_KP_2&0xFF] = KEY_NUMPAD2;
	keytable[ 0 ][XK_KP_Page_Down&0xFF] = keytable[ 0 ][XK_KP_3&0xFF] =	KEY_NUMPAD3;
	keytable[ 0 ][XK_KP_Left&0xFF] = keytable[ 0 ][XK_KP_4&0xFF] = KEY_NUMPAD4;
	keytable[ 0 ][XK_KP_Begin&0xFF] = keytable[ 0 ][XK_KP_5&0xFF] = KEY_NUMPAD5;
	keytable[ 0 ][XK_KP_Right&0xFF] = keytable[ 0 ][XK_KP_6&0xFF] = KEY_NUMPAD6;
	keytable[ 0 ][XK_KP_Home&0xFF] = keytable[ 0 ][XK_KP_7&0xFF] = KEY_NUMPAD7;
	keytable[ 0 ][XK_KP_Up&0xFF] = keytable[ 0 ][XK_KP_8&0xFF] = KEY_NUMPAD8;
	keytable[ 0 ][XK_KP_Page_Up&0xFF] = keytable[ 0 ][XK_KP_9&0xFF] = KEY_NUMPAD9;

	keytable[ 0 ][XK_KP_Delete&0xFF] = KEY_DELETE;
	keytable[ 0 ][XK_KP_Decimal&0xFF] = KEY_PERIOD;
	keytable[ 0 ][XK_KP_Divide&0xFF] = KEY_DIVIDE;
	keytable[ 0 ][XK_KP_Multiply&0xFF] = KEY_MULTIPLY;
	keytable[ 0 ][XK_KP_Subtract&0xFF] = KEY_MINUS;
	keytable[ 0 ][XK_KP_Add&0xFF] = KEY_ADD;
	keytable[ 0 ][XK_KP_Enter&0xFF] = KEY_NUMPADENTER;
	keytable[ 0 ][XK_KP_Equal&0xFF] = KEY_NUMPADEQUALS;

	keytable[ 0 ][XK_Up&0xFF] = KEY_UP;
	keytable[ 0 ][XK_Down&0xFF] = KEY_DOWN;
	keytable[ 0 ][XK_Right&0xFF] = KEY_RIGHT;
	keytable[ 0 ][XK_Left&0xFF] = KEY_LEFT;
	keytable[ 0 ][XK_Insert&0xFF] = KEY_INSERT;
	keytable[ 0 ][XK_Home&0xFF] = KEY_HOME;
	keytable[ 0 ][XK_End&0xFF] = KEY_END;
	keytable[ 0 ][XK_Page_Up&0xFF] = KEY_PRIOR;
	keytable[ 0 ][XK_Page_Down&0xFF] = KEY_NEXT;

	keytable[ 0 ][XK_F1&0xFF] = KEY_F1;
	keytable[ 0 ][XK_F2&0xFF] = KEY_F2;
	keytable[ 0 ][XK_F3&0xFF] = KEY_F3;
	keytable[ 0 ][XK_F4&0xFF] = KEY_F4;
	keytable[ 0 ][XK_F5&0xFF] = KEY_F5;
	keytable[ 0 ][XK_F6&0xFF] = KEY_F6;
	keytable[ 0 ][XK_F7&0xFF] = KEY_F7;
	keytable[ 0 ][XK_F8&0xFF] = KEY_F8;
	keytable[ 0 ][XK_F9&0xFF] = KEY_F9;
	keytable[ 0 ][XK_F10&0xFF] = KEY_F10;
	keytable[ 0 ][XK_F11&0xFF] = KEY_F11;
	keytable[ 0 ][XK_F12&0xFF] = KEY_F12;
	keytable[ 0 ][XK_F13&0xFF] = KEY_F13;
	keytable[ 0 ][XK_F14&0xFF] = KEY_F14;
	keytable[ 0 ][XK_F15&0xFF] = KEY_F15;

	keytable[ 0 ][XK_Num_Lock&0xFF] = KEY_NUMLOCK;
	keytable[ 0 ][XK_Shift_R&0xFF] = KEY_RSHIFT;
	keytable[ 0 ][XK_Shift_L&0xFF] = KEY_LSHIFT;
	keytable[ 0 ][XK_Control_R&0xFF] = KEY_RCONTROL;
	keytable[ 0 ][XK_Control_L&0xFF] = KEY_LCONTROL;
	keytable[ 0 ][XK_Alt_R&0xFF] = KEY_ALT_GR;
	keytable[ 0 ][XK_Alt_L&0xFF] = KEY_ALT;
	//keytable[ 0 ][XK_Meta_R&0xFF] = KEY_UNKNOWN; // RMETA miis
	//keytable[ 0 ][XK_Meta_L&0xFF] = KEY_UNKNOWN; // LMETA;
	keytable[ 0 ][XK_Super_L&0xFF] = KEY_LWIN;
	keytable[ 0 ][XK_Super_R&0xFF] = KEY_RWIN;
	keytable[ 0 ][XK_Mode_switch&0xFF] = KEY_ALT_GR;
}

unsigned int glsk::detail::input_translate_key( KeySym symbol )
{
	if ( !symbol )
		return 0;

	switch ( symbol>>8 )
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08:
		case 0x0A:
		case 0x0C:
		case 0x0D:
			return keytable[ 1 ][ symbol & 0xFF ];

		case 0xFF:
			return keytable[ 0 ][ symbol & 0xFF ];

		default:
			break;
	}

	return 0;
}

void glsk::detail::input_update()
{
	joystick_update();
}

void glsk::detail::init_input()
{
	input_init_keytable();
	scan_joysticks();
}

void glsk::detail::free_input()
{
	joystick_free();
}


void glsk::set_mouse_captured( bool value )
{
	using namespace glsk::detail;

	global.flags &= ~mouse_captured;
	if ( value ) global.flags |= mouse_captured;

	input_update_pointer_owner();
}


void glsk::detail::input_update_pointer_owner()
{
	using namespace glsk::detail;
	window::detail* wnd = global.window_focus ? &core::get_internal( *global.window_focus ) : 0;

	if ( (global.flags & mouse_captured) && wnd )
	{
		XGrabPointer( global.connection, wnd->handle, 0, PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, wnd->handle, global.blank_cursor, CurrentTime );

		XWarpPointer( global.connection, None, wnd->handle,
							0, 0, 0, 0, wnd->width/2, wnd->height/2 );
	}
	else
	{
		XUngrabPointer( global.connection, CurrentTime );
	}
}

void glsk::detail::input_update_pointer_button( int x, int y, int index, bool down, window* wnd )
{
	// Convert the index enum value
	switch( index )
	{
		case Button1: index = 1; break;
		case Button2: index = 3; break;
		case Button3: index = 2; break;
		case Button4: index = 4; break;
		case Button5: index = 5; break;
		default: return;
	};
	
	if ( global.flags & mouse_captured )
	{

		glsk::input_event event( 0, glsk::input_event::mouse_button, index, down ? 1 : 0 );

#ifdef _GLSK_USE_VIRTUAL_EVENTS_
        wnd->on_input( event );
#endif

#ifdef _GLSK_USE_SIGNALS_
        core::get_internal( *wnd ).signal_input( event );
#endif
	}
	else
	{
		y = core::get_internal( *wnd ).height - y;
		
#ifdef _GLSK_USE_VIRTUAL_EVENTS_
		wnd->on_mouse( x, y, input_event::mouse_button, index, down ? 1 : 0 );
#endif
		
#ifdef _GLSK_USE_SIGNALS_
		core::get_internal( *wnd ).signal_mouse(  x, y, input_event::mouse_button, index, down ? 1 : 0 );
#endif
	}
}


void glsk::detail::input_update_pointer_move( int x, int y, window* wnd )
{
	if ( global.flags & mouse_captured )
	{
		glsk::window::detail& data = core::get_internal( *wnd );

		int mx = data.width / 2;
		int my = data.height / 2;

		int dx = x-mx;
		int dy = y-my;

		if ( dx )
		{
			const glsk::input_event event( 0, glsk::input_event::mouse_axis_move, 0, dx );

#ifdef _GLSK_USE_VIRTUAL_EVENTS_
            wnd->on_input( event );
#endif

#ifdef _GLSK_USE_SIGNALS_
			data.signal_input( event );
#endif
		}

		if ( dy )
		{
			const glsk::input_event event( 0, glsk::input_event::mouse_axis_move, 1, dy );

#ifdef _GLSK_USE_VIRTUAL_EVENTS_
            wnd->on_input( event );
#endif

#ifdef _GLSK_USE_SIGNALS_
			data.signal_input( event );
#endif
		}

		if ( dx || dy ) // first call
		{
			XWarpPointer( global.connection, None, data.handle,
							0, 0, 0, 0, mx, my );
		}
	}
	else
	{
		y = core::get_internal( *wnd ).height - y;
		
#ifdef _GLSK_USE_VIRTUAL_EVENTS_
		wnd->on_mouse( x, y, input_event::mouse_axis_move, 0, 0 );
#endif
		
#ifdef _GLSK_USE_SIGNALS_
		core::get_internal( *wnd ).signal_mouse(  x, y, input_event::mouse_axis_move, 0, 0 );
#endif
	}
}

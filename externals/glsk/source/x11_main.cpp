
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
**		filename:		x11_main.cpp
**
**		description:	main file
**		target-system:	X-Windows
**		version-info:   $Id: x11_main.cpp 102 2009-02-17 19:03:30Z ltjax $
**
*/

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
// system includes
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#include <dlfcn.h>
#include <dirent.h>
#include <iostream>

// global include
#include <glsk/glsk.hpp>
#include "x11_main.hpp"
#include "x11_window.hpp"
#include "x11_input.hpp"

// instanciate the globals
glsk::detail::core	glsk::detail::global;

void glsk::detail::init_graphics()
{
	XF86VidModeModeInfo** 	video_modes = 0;
	int						video_mode_count = 0;


	if ( glXQueryVersion( global.connection, &(global.glx_version[ 0 ]), &(global.glx_version[ 1 ]) ) == True )
	{
		if ( global.glx_version[ 0 ] < 1 )
			throw failure( "unsupported glx version!" );

		if ( ( global.glx_version[ 0 ] == 1 ) && ( global.glx_version[ 1 ] < 3 ) )
			global.flags |= glx_fallback;
	}
	else
	{
		global.glx_version[ 0 ] = 1;
		global.glx_version[ 1 ] = 2;

		global.flags |= glx_fallback;
	}

	//global.flags |= glx_fallback;

	if ( global.flags & glx_fallback )
	{
		std::cout << "(glsk) using GLX version 1.2 (fallback mode)" << std::endl;
	}


	/*window = glsk_window_new();

	if ( window )
	{
		glsk_context_t* context = glsk_context_new();

		if ( glsk_window_open( window, context ) )
		{
			glsk_window_select_rendercontext( window );
			_glsk_init_extensions_string();
			glsk_window_destroy( window );

			glsk_main_run();
		}

		glsk_context_free( context );
	}

	glsk_window_free( window );*/

	// get all available modes
	int ExtMajor, ExtEvent, ExtError;
	if ( XQueryExtension( global.connection, "XFree86-VidModeExtension", &ExtMajor, &ExtEvent, &ExtError ) == True )
		global.flags |= use_vidmode_ext;


	if ( global.flags & use_vidmode_ext )
	{
		XF86VidModeGetAllModeLines( global.connection, global.screen, &video_mode_count, &video_modes );

		// save desktop-resolution
		global.desktop_video_mode = *(video_modes[ 0 ]);

		// free those modes
		XFree( video_modes );
	}
}


void glsk::detail::init_cursor()
{
	char 		data = 0;
	XColor 		dummy;
	Pixmap 		blank_pixmap = XCreateBitmapFromData( global.connection,
		RootWindow( global.connection, global.screen ), &data, 1, 1 );

	global.blank_cursor = XCreatePixmapCursor( global.connection, blank_pixmap,
								blank_pixmap, &dummy, &dummy, 0, 0 );

	XFreePixmap ( global.connection, blank_pixmap );
}

void glsk::detail::init_module()
{
	// grab our current connection to the server
	global.connection = XOpenDisplay( 0 );

	// grab the default screen for this workstation
	global.screen = DefaultScreen( global.connection );

	global.flags = 0;

	global.window_head = 0;
	global.window_focus = 0;
	global.window_open_count = 0;

#ifdef _glsk_use_idle_callback_
    global.idle_function = 0;
    global.idle_userdata = 0;
#endif

	// create an atom for windows to store the glsk_window_t pointer
	global.object_atom = XInternAtom( global.connection, "GLSK_OBJECT", false );
	global.delete_atom = XInternAtom( global.connection, "GLSK_DELETE_WINDOW", false );

	global.protocols_atom = XInternAtom( global.connection, "WM_PROTOCOLS", true );

	try	{
		init_cursor();
		init_graphics();
		init_input();

	}
	catch ( glsk::failure& x ) {
		DEBUG_PRINT1( "(glsk) init failed: ", x.what() );
		free_module();
	}

	gettimeofday( &global.timer_start, 0 );
}

void glsk::detail::free_module()
{
	glsk::detail::reset_vidmode();

	glsk::detail::free_input();

	// close the X Server connection
	XCloseDisplay( global.connection );
	global.connection = None;
}

void glsk::detail::reset_vidmode()
{
	if ( (global.flags & use_vidmode_ext) && (global.flags & vidmode_changed) )
	{
		XF86VidModeSwitchToMode( global.connection, global.screen, &global.desktop_video_mode );
		XF86VidModeSetViewPort( global.connection, global.screen, 0, 0 );

		global.flags &= ~vidmode_changed;
	}
}

glsk::object::object()
{
	unsigned int& count = refcounter();

	if ( !count )
		glsk::detail::init_module();

	++count;
}

glsk::object::object( const object& o )
{
	// another object has to exist, so don't check.
	++refcounter();
}

glsk::object::~object()
{
	if ( --refcounter() == 0 )
		glsk::detail::free_module();
}

unsigned int& glsk::object::refcounter()
{
	static unsigned int counter = 0;
	return counter;
}


int glsk::enumerate_resolutions( std::list< int3 >& result, const int3& min )
{
	using namespace glsk::detail;

	if ( (global.flags & use_vidmode_ext)==0 )
		throw unsuccessful();

	XF86VidModeModeInfo** 	video_modes = 0;
	XF86VidModeModeLine     current_mode;
	int						video_mode_count = 0;
    int                     current = 0;
    int                     dotclock = 0;

    XF86VidModeGetModeLine( global.connection, global.screen, &dotclock, &current_mode );

	// get all available modes
	XF86VidModeGetAllModeLines( global.connection, global.screen, &video_mode_count, &video_modes );

	for ( int i = 0; i < video_mode_count; ++i )
	{
	    if ( video_modes[ i ]->hdisplay >= min[ 0 ] && video_modes[ i ]->vdisplay >= min[ 1 ] )
            result.push_back( int3( video_modes[ i ]->hdisplay, video_modes[ i ]->vdisplay, 32 ) );

        if ( video_modes[ i ]->hdisplay == current_mode.hdisplay &&
             video_modes[ i ]->vdisplay == current_mode.vdisplay )
             current = i;
	}

	XFree( video_modes );

	return current;
}

void glsk::set_resolution( int width, int height, int bpp )
{
	using namespace glsk::detail;

	if ( (global.flags & use_vidmode_ext)==0 )
		throw unsuccessful();

	XF86VidModeModeInfo** 	video_modes = 0;
	int						video_mode_count = 0;

	// get all available modes
	XF86VidModeGetAllModeLines( global.connection, global.screen, &video_mode_count, &video_modes );

	for ( int i = 0; i < video_mode_count; ++i )
	{
		// search for a matching mode
		if ( ( video_modes[ i ]->hdisplay == width ) &&
			 ( video_modes[ i ]->vdisplay == height ) )
		{
			// this one matches our resolution, set it
			XF86VidModeModeInfo* mode = video_modes[ i ];

			//DEBUG_PRINT2( "(glsk) selecting video-mode: %i x %i\n", mode->hdisplay, mode->vdisplay );

			XF86VidModeSwitchToMode( global.connection, global.screen, mode );
			XF86VidModeSetViewPort( global.connection, global.screen, 0, 0 );

			XFree( video_modes );

			global.flags |= detail::vidmode_changed;

			return;
		}
	}

	XFree( video_modes );

	throw unsuccessful();
	//DEBUG_PRINT2(( "(glsk) unable to select video-mode: %i x %i\n", width, height );
}

bool glsk::main::pump_messages()
{
	using namespace glsk::detail;
	XEvent event;

	while ( global.window_open_count && XPending( global.connection ) > 0 )
	{

		XNextEvent( global.connection, &event );

		glsk::window* window = glsk::window::detail::decode_window_object( event.xany.window );

		if ( window == 0 )
			throw failure( "(glsk) unable to decode window pointer." );

		// handle this message
		core::get_internal( *window ).process_event( &event );
	}

	if ( global.window_open_count )
	{
		input_update();
		return true;
	}
	else
	{
		return false;
	}

}


void glsk::main::quit()
{
	using namespace glsk::detail;
	//global.flags |= GLSK_MAIN_QUIT;
	glsk::window* window = 0;

	for ( window = global.window_head; window != 0; window = core::get_internal( *window ).next )
		window->destroy();
}

double glsk::get_time()
{
	static struct timeval time;

	gettimeofday( &time, 0 );

	return ( time.tv_sec - glsk::detail::global.timer_start.tv_sec ) +
	       ( time.tv_usec - glsk::detail::global.timer_start.tv_usec ) * 0.000001;
}


void glsk::error_box( const std::string& text )
{
	// TODO: use X, or gnome, or kde or whatever
	printf( "Error: %s\n", text.c_str() );
}

void glsk::print( const std::string& text )
{
	printf( "%s", text.c_str() );
}

void glsk::print( const char* string )
{
	printf( "%s", string );
}

void glsk::file_system::change_directory( const std::string& path )
{
	if ( chdir( path.c_str() ) == -1 )
		throw glsk::unsuccessful();
}


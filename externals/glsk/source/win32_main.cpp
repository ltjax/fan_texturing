
/*
**	Copyright (C) 2005-2006 Marius Elvert
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
**		filename:		win32_main.cpp
**
**		description:	mainloop and general functions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_main.cpp 110 2010-03-08 14:30:21Z ltjax $
**
*/

#define _WIN32_WINNT 0x500

// system headers
#include <windows.h>
#include <wincon.h>
#include <gl/gl.h>

// global glsk header
#include <glsk/glsk.hpp>

// internal glsk headers
#include "win32_main.hpp"
#include "win32_window.hpp"
#include "config.hpp"
#include "win32_core.hpp"
#include "win32_input.hpp"

glsk::detail::wgl_type glsk::detail::wgl;

namespace {
	
	using namespace glsk;

	/* High performance timer data.
	*/
	struct {
		hyper				temp;
		hyper				frequency;
		double				resolution;
		hyper				start;
	}
	moduletime;
	
	/* Module-wide/Global Settings
	*/

	HINSTANCE				hinstance;
	TCHAR*					appname;

	DEVMODE					previous_mode;


	int						flags;

	::glsk::window*			window_head;
	unsigned int			windows_open;

	
	void init_timer()
	{
		if ( !QueryPerformanceFrequency( (LARGE_INTEGER*)( &moduletime.frequency ) ) )
			throw glsk::failure( "no performance counter available" );

		// get start time
		QueryPerformanceCounter( (LARGE_INTEGER*)( &moduletime.start ) );

		// Compute Resolution
		moduletime.resolution	= 1.0 / moduletime.frequency;
	}

	template< class T >
	void get_proc( T& function, const char* string )
	{
		function = reinterpret_cast< T >( wglGetProcAddress( string ) );
	}

	void register_class( HINSTANCE hinstance, TCHAR* appname, WNDPROC proc )
	{
		WNDCLASS c;

		if ( !hinstance )
			hinstance = GetModuleHandle( NULL );

		// init the wnd class
		c.style =			/*CS_HREDRAW | CS_VREDRAW |*/ CS_OWNDC;
		c.lpfnWndProc =		proc;
		c.cbClsExtra =		0;
		c.cbWndExtra =		0;
		c.hInstance =		hinstance;
		c.hIcon =			LoadIcon( NULL, IDI_APPLICATION );
		c.hCursor =			LoadCursor( NULL, IDC_ARROW );
		c.hbrBackground =	(HBRUSH)GetStockObject( GRAY_BRUSH );
		c.lpszMenuName =	0;
		c.lpszClassName =	appname;

		// register the class
		if ( !RegisterClass( &c ) )
			throw glsk::failure( "this program requires Unicode support." );
	}

	void init_graphics_extensions()
	{
		if ( glsk::detail::wgl.Initialized )
			return;

		// query for generic wgl extensions
		get_proc( detail::wgl.GetExtensionString, "wglGetExtensionsStringARB" );
		get_proc( detail::wgl.ChoosePixelformat, "wglChoosePixelFormatARB" );

		// query for PBuffer extensions
		get_proc( detail::wgl.CreatePbuffer, "wglCreatePbufferARB" );
		get_proc( detail::wgl.DestroyPbuffer, "wglDestroyPbufferARB" );
		get_proc( detail::wgl.GetPbufferDC, "wglGetPbufferDCARB" );
		get_proc( detail::wgl.ReleasePbufferDC, "wglReleasePbufferDCARB" );
		get_proc( detail::wgl.QueryPbuffer, "wglQueryPbufferARB" );

		// query new context creation routine
		get_proc( detail::wgl.CreateContextAttribs, "wglCreateContextAttribsARB" );

		const char* extension_string = (const char*)glGetString( GL_EXTENSIONS );

		// check for multisample support
		detail::wgl.MultisampleSupported = (strstr( extension_string, "GL_ARB_multisample" ) != NULL);
		

		detail::wgl.Initialized = true;
	}

	LRESULT CALLBACK init_graphics_callback( HWND window, UINT message, WPARAM wp, LPARAM lp )
	{
		if ( message != WM_CREATE )
			return DefWindowProc( window, message, wp, lp );

		// retrieve a device context
		HDC dc = GetDC( window );

		int result_index = 0;

		// configure a pixelformat
		PIXELFORMATDESCRIPTOR pfd;
		ZeroMemory( &pfd, sizeof( pfd ) );
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 16;
		pfd.cStencilBits = 0;

		if ( SetPixelFormat( dc, ChoosePixelFormat( dc, &pfd ), &pfd ) == 0 )
			return -1;

		// create dummy context
		HGLRC rc = wglCreateContext( dc );
		wglMakeCurrent( dc, rc );

		init_graphics_extensions();

		// free the dummy context
		wglMakeCurrent( dc, 0 );
		wglDeleteContext( rc );

		// release the dc
		ReleaseDC( window, dc );

		// kill the window
		return -1;
	}

	void init_graphics()
	{
		if ( detail::wgl.Initialized )
			return;

		TCHAR* classname = TEXT( "gfx-init-cls" );

		DEBUG_PRINT0( " initialising graphics extensions." );
		register_class( hinstance, classname, &init_graphics_callback );

		HWND window = CreateWindow( classname, TEXT( "gfx-init" ), WS_POPUP | WS_DISABLED, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinstance, NULL );

		if ( window != NULL )
			throw failure( "assertion failed: gfx-init window successfully created." );

		UnregisterClass( classname, hinstance );

		detail::wgl.Initialized = true;
		DEBUG_PRINT0( "(ok)\n" );
	}

}

HINSTANCE glsk::detail::get_module_instance()
{
	return hinstance;
}

const char* glsk::detail::get_module_appname()
{
	return appname;
}

unsigned int& glsk::detail::get_window_count()
{
	return windows_open;
}

glsk::window*& glsk::detail::get_window_list()
{
	return window_head;
}

void glsk::detail::open_console()
{
	if ( flags & console_is_open )
		return;

	// allocate the console and redirect cout
	//AllocConsole();
	//if ( AttachConsole( ATTACH_PARENT_PROCESS ) == 0 )
		AllocConsole();

	//freopen( "CONOUT$", "w", stdout );

	flags |= console_is_open;
}

/** Get the time that elapsed since glsk was started.
	\return the time in seconds.
	\ingroup Supportive
*/
double glsk::get_time()
{
	QueryPerformanceCounter( (LARGE_INTEGER*)( &moduletime.temp ) );

	return ( ( moduletime.temp - moduletime.start )
		* moduletime.resolution );
}

void glsk::detail::init_module( HINSTANCE hinstance )
{
	try {
		// init the global vars
		::hinstance = hinstance;
		appname = TEXT( "glsk-wnd-cls" );

		flags = 0;
		window_head = 0;
		windows_open = 0;

		wgl.Initialized = false;


		// save the old display setting
		EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &previous_mode );

		// register the window class
		register_class( hinstance, appname, &window::detail::proc );

		input::init();
		init_timer();

#ifdef _GLSK_OPEN_CONSOLE
		open_console();
#endif

	}
	catch ( glsk::failure& f )
	{
		glsk::error_box( std::string( "Unable to initialize subsystem:" ) + f.what() );
		free_module( hinstance );
	}
}

void glsk::detail::free_module( HINSTANCE hinstance )
{
	input::free();

	if ( flags & detail::devmode_changed ) // change it back
		ChangeDisplaySettings( &previous_mode, 0 );

	UnregisterClass( appname, hinstance );
}


int glsk::enumerate_resolutions( std::list< int3 >& result, const int3& min )
{
	result.clear();

	DEVMODE mode,cmode;
	int current = -1;

	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &cmode );

	for ( DWORD i = 0; EnumDisplaySettings( NULL, i, &mode ); ++i )
	{
		if ( mode.dmBitsPerPel > 8 && int( mode.dmPelsWidth ) >= min.get<0>() &&
			int( mode.dmPelsHeight ) >= min.get<1>() && int( mode.dmBitsPerPel ) >= min.get<2>() ) // filter out 8bit and less
		{
			if ( mode.dmPelsWidth == cmode.dmPelsWidth &&
				mode.dmPelsHeight == cmode.dmPelsHeight &&
				mode.dmBitsPerPel == cmode.dmBitsPerPel )
				current = int( result.size() );

			result.push_back( int3( mode.dmPelsWidth, mode.dmPelsHeight, int( mode.dmBitsPerPel ) ) );
		}
	}

	return current;
}

/**	Sets the monitor resolution.
	This function is used to set the resolution of the users monitor.
	The old resolution is restored when the program terminates.
	\param width The width of the new resolution in pixels.
	\param height The height of the new resolution in pixels.
	\param bpp Sets the bits per pixel for the new resolution.
	\ingroup Main
*/
void glsk::set_resolution( int width, int height, int bpp )
{
	DEVMODE device_mode;

	if ( width == 0 || height == 0 || bpp == 0 )
	{
		// reset to our initial mode
		device_mode = previous_mode;
	}
	else
	{
		// select an appropriate mode
		ZeroMemory( &device_mode, sizeof(DEVMODE) );

		device_mode.dmSize			= sizeof(DEVMODE);
		device_mode.dmPelsWidth		= width;
		device_mode.dmPelsHeight	= height;
		device_mode.dmBitsPerPel	= bpp;
		device_mode.dmFields		= ( DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT );
	}

	if ( ChangeDisplaySettings( &device_mode, CDS_FULLSCREEN ) == DISP_CHANGE_SUCCESSFUL )
	{
		// set the devmode changed flag, so we can change back on module_free
		// TODO: do not set this flag when previous mode was selected
		flags |= detail::devmode_changed;
	}
	else
	{
		throw glsk::unsuccessful();
	}
}

bool glsk::main::pump_messages()
{
	if ( !windows_open )
		return false;

	MSG message;

	while ( PeekMessage( &message, NULL, 0, 0, PM_REMOVE ) )
	{
		if ( message.message == WM_QUIT )
			return false;

		// FIXME: should we update input before event handling?
		glsk::detail::input::update();

		TranslateMessage( &message );
		DispatchMessage( &message );

		if ( !windows_open )
			return false;
	}

	// we ran out of messages ... proceed
	glsk::detail::input::update();

	return windows_open > 0;
}


/** Quits the application main loop.
*/
void glsk::main::quit()
{
	static bool block = false;

	// FIXME: this is kinda ugly
	if ( block ) // make sure this is not called recursivly
		return;

	block = true;

	for ( glsk::window* i = window_head; i; i = glsk::detail::core::get_next( i ) )
		i->destroy();

	block = false;
}


/** Creates an error box with the given text and an OK button.
	\ingroup Supportive
*/
void glsk::error_box( const std::string& text )
{
	// start a messagebox with the given text
	MessageBox( NULL, text.c_str(), "Error", MB_ICONERROR | MB_OK );
}

/** Print the given text to a console.
	\ingroup Supportive
*/
void glsk::print( const char* text )
{
	if ( flags & detail::console_is_open )
	{	
		OutputDebugString( text );
	}
	else
	{
		printf( text );
	}
}

/** Print the given text to a console.
	\ingroup Supportive
*/
void glsk::print( const std::string& text )
{
	glsk::print( text.c_str() );
}


glsk::object::object()
{
	unsigned int& count = refcounter();
	
	if ( !count )
		glsk::detail::init_module( GetModuleHandle( NULL ) );

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
		glsk::detail::free_module( GetModuleHandle( NULL ) );
}

unsigned int& glsk::object::refcounter()
{
	static unsigned int counter = 0;
	return counter;
}

void glsk::detail::init_wgl()
{
	if ( wglGetCurrentContext() == NULL )
		init_graphics();
	else
		init_graphics_extensions();
}

// Additional Documentation

/** \fn void glsk::drawable::swap_buffers()
	Swap the rendering buffers.
	If double buffering is selected, this swaps the front and back buffers, making eventual changes visible.
	\see pixelformat::doublebuffer
*/

/**	\fn bool glsk::drawable::select_rc()
	Select this rendercontext.
	Makes this drawable's rendercontext the active one, directing all OpenGL commands to it afterwards.
*/

/** \fn int glsk::input_event::get_device() const
	Get the index for the device that produced this event.
*/

/** \fn glsk::input_event::event_type glsk::input_event::get_type() const
	Get the type of event.
*/

/** \fn int glsk::input_event::get_identifier() const
	Get the index for the object on the device that produced this event, for example the button or key
*/

/** \fn int glsk::input_event::get_value() const
	Get the value to be reported.
*/

/** \fn glsk::input_event::input_event( const int device, const event_type type, const int identifier, const int value )
	Create a new input event from the given values.
*/

/** \defgroup Main Windowing System
	These are the core pieces of each glsk application.
*/

/** \defgroup Input Input Module
	Handles joystick, mouse and keyboard input.
*/

/** \defgroup Supportive Supportive
	Supportive, but non-essential classes.
*/

/** \defgroup Exceptions Exceptions
	These define exception types that are used by and with glsk.
*/


/** \mainpage glsk - an openGL Systems toolKit for C++
*
* \section intro_sec About
*
* glsk is C++ a library designed to make
* opening and managing windows for high performance OpenGL applications
* clean, portable and easy. Unlike other comparable toolkits 
* glsk is specifically designed for modern OpenGL and C++ implementations.
* It hence supports, a set of advanced features out of
* the box, that other toolkits lack.
*/



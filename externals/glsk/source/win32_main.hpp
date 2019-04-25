
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
**		filename:		win32_main.h
**
**		description:	global definitions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_main.hpp 111 2010-05-07 10:21:01Z ltjax $
**
*/

#include <glsk/glsk.hpp>
#include <windows.h>
#include <wincon.h>
#include <gl/gl.h>

/* WGL_PIXELFORMAT defines
*/
#define WGL_DRAW_TO_WINDOW_ARB			0x2001
#define WGL_DRAW_TO_BITMAP_ARB			0x2002
#define WGL_DRAW_TO_PBUFFER_ARB			0x202D
#define WGL_ACCELERATION_ARB			0x2003
#define	WGL_SUPPORT_OPENGL_ARB			0x2010
#define WGL_DOUBLE_BUFFER_ARB			0x2011
#define	WGL_COLOR_BITS_ARB				0x2014
#define	WGL_PIXEL_TYPE_ARB				0x2013
#define WGL_DEPTH_BITS_ARB				0x2022
#define WGL_STENCIL_BITS_ARB			0x2023
#define WGL_SAMPLE_BUFFERS_ARB			0x2041
#define WGL_SAMPLES_ARB					0x2042
#define WGL_NO_ACCELERATION_ARB			0x2025
#define WGL_GENERIC_ACCELERATION_ARB	0x2026
#define WGL_FULL_ACCELERATION_ARB		0x2027
#define WGL_TYPE_RGBA_ARB				0x202B

/* WGL_PBUFFER defines
*/
DECLARE_HANDLE(HPBUFFER);
#define WGL_PBUFFER_LARGEST_ARB         0x2033

/* WGL_create_context defines
*/
#define WGL_CONTEXT_MAJOR_VERSION_ARB				0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB				0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB					0x2093
#define WGL_CONTEXT_FLAGS_ARB						0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB				0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB					0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB		0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB			0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB	0x00000002


namespace glsk { namespace detail {

/* WGL function pointer declarations.
*/
struct wgl_type
{
	BOOL					(APIENTRY *ChoosePixelformat)( HDC, const int*, const FLOAT*, UINT, int*, UINT* );
	const char*				(APIENTRY *GetExtensionString)( HDC );

	HPBUFFER				(APIENTRY *CreatePbuffer)( HDC, int, int, int, const int *);
	HDC						(APIENTRY *DestroyPbuffer)( HPBUFFER );
	HDC						(APIENTRY *GetPbufferDC)( HPBUFFER );
	int						(APIENTRY *ReleasePbufferDC)( HPBUFFER, HDC );
	BOOL					(APIENTRY *QueryPbuffer)( HPBUFFER, int, int* );

	HGLRC					(APIENTRY *CreateContextAttribs)(HDC hDC, HGLRC hshareContext, const int *attribList);


	bool					MultisampleSupported;

	bool					Initialized;

	wgl_type() : MultisampleSupported( false ), Initialized( false ) {}
};

/* Module-wide Flags
*/
enum {
	devmode_changed = 1 << 0,
	console_is_open = 1 << 1
};


/* WGL function pointer declarations.
*/
extern wgl_type			wgl;

void					init_module( HINSTANCE hinstance );
void					free_module( HINSTANCE hinstance );

void					init_wgl();
HINSTANCE				get_module_instance();
const char*				get_module_appname();
glsk::window*&			get_window_list();
unsigned int&			get_window_count();

void					open_console();


/* Debug printing code
*/

#ifdef _DEBUG

#include <stdio.h>

#ifdef _NO_DEBUG_CONSOLE

#define DEBUG_PRINT0( x ) _snprintf( global.debug_buffer, DEBUG_BUFFER_SIZE, (x) ); OutputDebugString( global.debug_buffer );
#define DEBUG_PRINT1( x, a ) _snprintf( global.debug_buffer, DEBUG_BUFFER_SIZE, (x), (a) ); OutputDebugString( global.debug_buffer );
#define DEBUG_PRINT2( x, a, b ) _snprintf( global.debug_buffer, DEBUG_BUFFER_SIZE, (x), (a), (b) ); OutputDebugString( global.debug_buffer );
#define DEBUG_PRINT3( x, a, b, c ) _snprintf( global.debug_buffer, DEBUG_BUFFER_SIZE, (x), (a), (b), (c) ); OutputDebugString( global.debug_buffer );

#else

#define DEBUG_PRINT0( x ) glsk::detail::open_console(); printf( (x) ); 
#define DEBUG_PRINT1( x, a ) glsk::detail::open_console(); printf( (x), (a) );
#define DEBUG_PRINT2( x, a, b ) glsk::detail::open_console(); printf( (x), (a), (b) );
#define DEBUG_PRINT3( x, a, b, c ) glsk::detail::open_console(); printf( (x), (a), (b), (c) );

#endif
#else

#define DEBUG_PRINT0( x )
#define DEBUG_PRINT1( x, a )
#define DEBUG_PRINT2( x, a, b )
#define DEBUG_PRINT3( x, a, b, c )

#endif

}}


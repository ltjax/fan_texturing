
/*
**
**		filename:		win32_pbuffer.c
**
**		description:	pbuffer related functions
**		target-system:	microsoft windows
**		version-info:   $Id: pbuffer.cpp 101 2009-01-26 02:21:58Z ltjax $
**
*/

// global glsk header
#include <glsk/glsk.hpp>

// internal glsk headers
#ifdef _WIN32
#  include <windows.h>
#  include <gl/gl.h>
#  include "win32_main.hpp"
#  include "win32_window.hpp"
#  include "config.hpp"
#  include "pbuffer.hpp"
#  include "win32_core.hpp"
#else
#  include <stdlib.h>
#  include <X11/X.h>
#  include <X11/Xatom.h>
#  include <GL/glx.h>
#  include <X11/extensions/xf86vmode.h>
#  include "x11_main.hpp"
#  include "config.hpp"
#  include "x11_window.hpp"
#endif

struct glsk::pbuffer::detail
{
#ifdef _WIN32
	HPBUFFER				handle;
	HDC						dc;
#else
	GLXPbuffer				handle;
#endif

	glsk::window&			owner;
	glsk::config			config;

	int						width;
	int						height;

	detail( glsk::window& owner, const glsk::config& config );
	~detail();
};

glsk::pbuffer::detail::detail( glsk::window& owner, const glsk::config& config )
: owner( owner ), config( config )
{
	this->handle = 0;
#ifdef _WIN32
	this->dc = 0;
#endif

	this->width = this->height = 0;
}

glsk::pbuffer::detail::~detail()
{
}

/** Create a new pbuffer.
*/
glsk::pbuffer::pbuffer( glsk::window& owner )
: data( new detail( owner, owner.get_rc() ) )
{
}

/** Create a new pbuffer.
*/
glsk::pbuffer::pbuffer( glsk::window& owner, const glsk::config& rc )
: data( new detail( owner, rc ) )
{
}

glsk::pbuffer::~pbuffer()
{
	delete data;
}

/** Open the pbuffer.
	This creates the physical pbuffer.
	\param width Desired width of this pbuffer.
	\param height Desired height of this pbuffer.
	\param get_largest If this is set to true, the system will allocate the largest pbuffer possible if the allocation would otherwise fail.
*/
void glsk::pbuffer::open( int width, int height, bool get_largest )
{
#ifdef _WIN32
	const int attribs[] = { WGL_PBUFFER_LARGEST_ARB, get_largest ? 1 : 0, 0 };

	glsk::detail::init_wgl();

	if ( !glsk::detail::wgl.CreatePbuffer )
		throw glsk::unsuccessful();

	// get a pixelformat
	const int descriptor = glsk::detail::core::get_pixelformat_descriptor( glsk::detail::core::get_dc( data->owner ), data->config );

	// get the pbuffer handle
	this->data->handle = glsk::detail::wgl.CreatePbuffer( glsk::detail::core::get_dc( data->owner ),
		descriptor, width, height, attribs );

	if ( !this->data->handle )
		throw glsk::unsuccessful();

	// get the device context - this is not supposed to fail
	this->data->dc = glsk::detail::wgl.GetPbufferDC( this->data->handle );

	// bind/create a rendering context
	glsk::detail::core::get_physical_rc( this->data->dc, this->data->config );

	// init the structure
	this->data->width = width;
	this->data->height = height;
#else
	using namespace glsk::detail;

	int attribs[] = { GLX_PBUFFER_WIDTH, width,
					  GLX_PBUFFER_HEIGHT, height,
					  GLX_LARGEST_PBUFFER, get_largest,
					  GLX_PRESERVED_CONTENTS, 1, None };

	GLXFBConfig fbconfig = None;

	if ( global.flags & glx_fallback )
		throw glsk::unsuccessful();

	fbconfig = core::get_internal( data->config ).get_fbconfig();

	if ( fbconfig == None )
		throw glsk::unsuccessful();

	data->handle = glXCreatePbuffer( global.connection, fbconfig, attribs );

	if ( data->handle == None )
		throw glsk::unsuccessful();

	core::get_internal( data->config ).get_context( fbconfig );

	if ( get_largest )
	{
		unsigned int value = 0;

		glXQueryDrawable( global.connection, data->handle, GLX_PBUFFER_WIDTH, &value );
		data->width = value;

		glXQueryDrawable( global.connection, data->handle, GLX_PBUFFER_HEIGHT, &value );
		data->height = value;
	}
	else
	{
		data->width = width;
		data->height = height;
	}
#endif
}

/** Destroy the pbuffer.
	Destroy the physical pbuffer that is referenced by this object.
*/
void glsk::pbuffer::destroy()
{
#ifdef _WIN32
	glsk::detail::core::release_physical_rc( this->data->dc, this->data->config );

	if ( this->data->dc )
	{
		glsk::detail::wgl.ReleasePbufferDC( this->data->handle, this->data->dc );
		this->data->dc = 0;
	}

	if ( this->data->handle )
	{
		glsk::detail::wgl.DestroyPbuffer( this->data->handle );
		this->data->handle = 0;
	}

	this->data->width = 0;
	this->data->height = 0;
#else
	using namespace glsk::detail;

	if ( glXGetCurrentDrawable() == data->handle )
	{
		glXMakeContextCurrent( global.connection, None, None, None );
	}

	core::get_internal( data->config ).release_context();
	data->width = data->height = 0;

	if ( data->handle )
		glXDestroyPbuffer( global.connection, data->handle );

	data->handle = None;
#endif
}

void glsk::pbuffer::swap_buffers()
{
#ifdef _WIN32
	SwapBuffers( this->data->dc );
#else
	using namespace glsk::detail;

	glXSwapBuffers( global.connection, data->handle );
#endif
}

bool glsk::pbuffer::select_rc()
{
#ifdef _WIN32
	return wglMakeCurrent( this->data->dc, glsk::detail::core::get_rc( this->data->config ) ) != 0;
#else
	using namespace glsk::detail;

	return glXMakeContextCurrent( global.connection, data->handle,
					data->handle, core::get_internal( data->config ).handle );
#endif
}


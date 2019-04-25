
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
**		filename:		config.cpp
**
**		description:	TODO
**		version-info:   $Id: config.cpp 110 2010-03-08 14:30:21Z ltjax $
**
*/

#include <algorithm>
#include <boost/range.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <glsk/glsk.hpp>

#ifdef _WIN32
#  include "win32_main.hpp"
#else
#  include "x11_main.hpp"
#endif

#include "config.hpp"


namespace {
	void add_extensions( std::set< std::string >& extensions, const char* string )
	{
		using namespace boost::algorithm;

		boost::iterator_range< const char* > stringrange( string, string + std::strlen( string ) );

		split_iterator< const char* > token( stringrange,
			token_finder( is_space(), token_compress_on ) );

		for ( ; !token.eof(); ++token )
			extensions.insert( std::string( token->begin(), token->end() ) );
	}
}


glsk::config::detail::detail()
: object_refcount( 0 ), context_refcount( 0 ), handle( NULL )
{
}

glsk::config::detail::~detail()
{
}

/** Create a rendering config from given settings.
*/
glsk::config::config( int flags, int color_bits, int depth_bits, int stencil_bits, int samples,
					 int major_version, int minor_version, int core_profile )
: data( new detail )
{
	++data->object_refcount;
	data->flags = flags;
	data->color_bits = color_bits;
	data->depth_bits = depth_bits;
	data->stencil_bits = stencil_bits;
	data->samples = samples;
	data->major_version = major_version;
	data->minor_version = minor_version;
	data->core_profile = core_profile;
}

/** Copy-ctor. Create another reference to this config.
*/
glsk::config::config( const config& other )
: data( other.data )
{
	++data->object_refcount;
}

/** Assign to this reference object.
*/
glsk::config& glsk::config::operator =( const glsk::config& other )
{
	if ( this->data == other.data )
		return *this;

	if ( --data->object_refcount == 0 )
	{
		if ( data->handle )
			throw failure( "assertion failed: handle has not been freed yet." );

		delete data;
	}

	data = other.data;
	data->object_refcount++;

	return *this;
}

glsk::config::~config()
{
	if ( --data->object_refcount == 0 )
	{

		//if ( data->handle )
		//	throw failure( "assertion failed: handle has not been freed yet." );

		delete data;
	}
}

/** Get the number of extensions available by this system.
*/
int glsk::config::get_extension_count() const
{
	data->prepare_extension_string();

	return static_cast< int >( data->extensions.size() );
}

/** Get the n'th extension name.
*/
std::string glsk::config::get_extension_name( int index ) const
{
	data->prepare_extension_string();

	if ( index >= 0 && static_cast< unsigned int >( index ) < this->data->extensions.size() )
		return *boost::next( this->data->extensions.begin(), index );

	return std::string();
}

/** Check if the given extension is supported by this config.
*/
bool glsk::config::is_extension_supported( const std::string& extension ) const
{
	data->prepare_extension_string();

	return data->extensions.find( extension ) != data->extensions.end();
}

#ifdef _WIN32


//using namespace glsk::internal;

void glsk::config::detail::set_pixelformat_fallback( HDC dc, PIXELFORMATDESCRIPTOR* pfd )
{
	int		result_index = 0;

	std::fill_n( reinterpret_cast< unsigned char* >( pfd ), sizeof( PIXELFORMATDESCRIPTOR ), 0 );

	pfd->nSize			= sizeof(PIXELFORMATDESCRIPTOR);
	pfd->nVersion		= 1;
	pfd->iLayerType		= PFD_MAIN_PLANE;
	pfd->dwFlags		= PFD_SUPPORT_OPENGL;
	pfd->iPixelType		= PFD_TYPE_RGBA;

	if ( this->flags & glsk::config::color_bits_set )
		pfd->cColorBits	= this->color_bits;
	else
		pfd->cColorBits = 32;

	if ( this->flags & glsk::config::depth_bits_set )
		pfd->cDepthBits = this->depth_bits;
	else
		pfd->cDepthBits = 16;

	if ( this->flags & glsk::config::stencil_bits_set )
		pfd->cStencilBits = this->stencil_bits;
	else
        pfd->cStencilBits = 0;

	if ( this->flags & glsk::config::draw_to_window )
		pfd->dwFlags |= PFD_DRAW_TO_WINDOW;

	if ( this->flags & glsk::config::doublebuffer )
		pfd->dwFlags |= PFD_DOUBLEBUFFER;

	if ( this->flags & glsk::config::draw_to_pbuffer ) // this is not supported
		throw failure( "Pbuffers not supported." );

	if ( this->flags & glsk::config::multisample )
		throw failure( "Multisample not supported." );

	result_index = ChoosePixelFormat( dc, pfd );

	if ( SetPixelFormat( dc, result_index, pfd ) == 0 )
		throw glsk::unsuccessful();
}

#define set_pixelformat_value( a, b, v, c ) { (v)[ (c)++ ] = (a); (v)[ (c)++ ] = (b); }

int glsk::config::detail::get_pixelformat_descriptor( HDC dc )
{
	int						values[ 24 ];
	int						c = 0;

	// try the fallback if wglChoosePixelformat isnt loaded
	if ( glsk::detail::wgl.ChoosePixelformat == 0 )
		throw unsuccessful();

	// set minimal requirements
	set_pixelformat_value( WGL_ACCELERATION_ARB,	WGL_FULL_ACCELERATION_ARB, values, c );
	set_pixelformat_value( WGL_PIXEL_TYPE_ARB,		WGL_TYPE_RGBA_ARB, values, c );
	set_pixelformat_value( WGL_SUPPORT_OPENGL_ARB,	GL_TRUE, values, c );

    // draw to a window?
	if ( this->flags & glsk::config::draw_to_window )
		set_pixelformat_value( WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, values, c );

	// draw to a pbuffer?
	if ( this->flags & glsk::config::draw_to_pbuffer )
	{
		if ( glsk::detail::wgl.CreatePbuffer == 0 )
			throw failure( "PBuffer Extension not supported but requested." );

		set_pixelformat_value( WGL_DRAW_TO_PBUFFER_ARB,	GL_TRUE, values, c );
	}

	// FIXME: should the flag mean yes/no or yes/don't care?
	if ( this->flags & glsk::config::doublebuffer )
		set_pixelformat_value( WGL_DOUBLE_BUFFER_ARB, GL_TRUE, values, c );

	// Anti Aliasing via Multisampling?
	if ( this->flags & glsk::config::multisample )
	{
		if ( glsk::detail::wgl.MultisampleSupported )
		{
			set_pixelformat_value( WGL_SAMPLE_BUFFERS_ARB,	GL_TRUE, values, c );

			if ( this->samples > 0 )
				set_pixelformat_value( WGL_SAMPLES_ARB, this->samples, values, c );
		}
		else
		{
			// FIXME: issue a warning?
		}
	}

	if ( this->flags & glsk::config::color_bits_set )
		set_pixelformat_value( WGL_COLOR_BITS_ARB, this->color_bits, values, c );

	if ( this->flags & glsk::config::depth_bits_set )
		set_pixelformat_value( WGL_DEPTH_BITS_ARB, this->depth_bits, values, c );

	if ( this->flags & glsk::config::stencil_bits_set )
		set_pixelformat_value( WGL_STENCIL_BITS_ARB, this->stencil_bits, values, c );

	// set the trailing 0
	values[ c ] = 0;

	int						result = 0;
	unsigned int			result_count = 0;

	if ( !glsk::detail::wgl.ChoosePixelformat( dc, values, 0, 1, &result, &result_count ) || !result_count )
		throw failure( "Unable to pick selected pixelformat." );

	return result;
}

void glsk::config::detail::set_pixelformat( HDC dc )
{
	PIXELFORMATDESCRIPTOR	pfd;

	if ( this->flags & ( glsk::config::draw_to_pbuffer | glsk::config::multisample ) )
	{
		try
		{
			glsk::detail::init_wgl();

			int pixelformat_descriptor = get_pixelformat_descriptor( dc );

			if ( SetPixelFormat( dc, pixelformat_descriptor, &pfd ) == 0 )
				throw glsk::unsuccessful();
		}
		catch( glsk::unsuccessful )
		{
			set_pixelformat_fallback( dc, &pfd );		
		}
	}
	else
	{
		set_pixelformat_fallback( dc, &pfd );	
	}
}


void glsk::config::detail::prepare_extension_string()
{
	// did we already do this?
	if ( !extensions.empty() )
		return;

	// not really created yet
	if ( handle == NULL )
		throw failure( "assertion failed: context is not valid." );

	// is the right context selected?
	if ( handle != wglGetCurrentContext() )
		throw failure( "assertion failed: context is not current." );

	glsk::detail::init_wgl();

	add_extensions( this->extensions, ( const char* )( glGetString( GL_EXTENSIONS ) ) );

	// add WGL extensions, only when they are available
	if ( glsk::detail::wgl.GetExtensionString != 0 )
		add_extensions( this->extensions, glsk::detail::wgl.GetExtensionString( wglGetCurrentDC() ) );
}

/** Get the address of the given function.
*/
glsk::proc glsk::config::get_extension_proc( const char* name ) const
{
	// not really created yet
	if ( data->handle == NULL )
		throw failure( "assertion failed: context is not valid." );

	// is the right context selected?
	if ( data->handle != wglGetCurrentContext() )
		throw failure( "assertion failed: context is not current." );

	return ( glsk::proc )wglGetProcAddress( name );
}

#else

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
// system includes
#include <iostream>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
//#include <dlfcn.h>
//#include <dirent.h>

#define set_config_value( a, b, v, c ) { (v)[ (c)++ ] = (a); (v)[ (c)++ ] = (b); }

void glsk::config::detail::prepare_extension_string()
{
	// did we already do this?
	if ( !extensions.empty() )
		return;

	// not really created yet
	if ( handle == NULL )
		throw failure( "assertion failed: context is not valid." );

	// is the right context selected?
	if ( handle != glXGetCurrentContext() )
		throw failure( "assertion failed: context is not current." );

	add_extensions( this->extensions, ( const char* )( glGetString( GL_EXTENSIONS ) ) );
}

/** Get the address of the given function.
*/
glsk::proc glsk::config::get_extension_proc( const char* name ) const
{
	// not really created yet
	if ( data->handle == NULL )
		throw failure( "assertion failed: context is not valid." );

	// is the right context selected?
	if ( data->handle != glXGetCurrentContext() )
		throw failure( "assertion failed: context is not current." );
		
	return (glsk::proc)glXGetProcAddressARB( (GLubyte*) name );
}

XVisualInfo* glsk::config::detail::get_visualinfo() const
{
	int						values[ 24 ];
	int						counter = 0;

	values[ counter++ ] = GLX_RGBA;

	if ( flags & glsk::config::doublebuffer )
		values[ counter++ ] = GLX_DOUBLEBUFFER;

	if ( flags & glsk::config::color_bits_set )
	{
		switch( color_bits )
		{
			case 32:
				set_config_value( GLX_RED_SIZE,   8, values, counter );
				set_config_value( GLX_GREEN_SIZE, 8, values, counter );
				set_config_value( GLX_BLUE_SIZE,  8, values, counter );
			break;

			case 16:
				set_config_value( GLX_RED_SIZE,   5, values, counter );
				set_config_value( GLX_GREEN_SIZE, 6, values, counter );
				set_config_value( GLX_BLUE_SIZE,  5, values, counter );
			break;

			default:
				return 0;
		};
	}

	values[ counter++ ] = None;

	return glXChooseVisual( glsk::detail::global.connection,
						glsk::detail::global.screen, values );
}


GLXFBConfig glsk::config::detail::get_fbconfig() const
{
	GLXFBConfig*			Array = 0;
	GLXFBConfig				Result = 0;
	int						values[ 24 ];
	int						counter = 0;
	int						elements = 0;

	if ( flags & ( draw_to_window | draw_to_pbuffer ) )
	{
		int bits = 0;

		if ( flags & draw_to_window )
			bits |= GLX_WINDOW_BIT_SGIX;

		if ( flags & draw_to_pbuffer )
			bits |= GLX_PBUFFER_BIT_SGIX;

		set_config_value( GLX_DRAWABLE_TYPE_SGIX, bits, values, counter );
	}

	if ( flags & doublebuffer )
		set_config_value( GLX_DOUBLEBUFFER, 1, values, counter );

	if ( flags & glsk::config::color_bits_set )
	{
		switch( color_bits )
		{
			case 32:
				set_config_value( GLX_RED_SIZE,   8, values, counter );
				set_config_value( GLX_GREEN_SIZE, 8, values, counter );
				set_config_value( GLX_BLUE_SIZE,  8, values, counter );
			break;

			case 16:
				set_config_value( GLX_RED_SIZE,   5, values, counter );
				set_config_value( GLX_GREEN_SIZE, 6, values, counter );
				set_config_value( GLX_BLUE_SIZE,  5, values, counter );
			break;

			default:
				return 0;
		};
	}

	if ( flags & depth_bits_set )
		set_config_value( GLX_DEPTH_SIZE, depth_bits, values, counter );

	if ( flags & stencil_bits_set )
		set_config_value( GLX_STENCIL_SIZE, stencil_bits, values, counter );

	if ( flags & multisample )
	{
		/*if ( glsk_extensions_is_supported( "GL_ARB_multisample" ) )
		{
			set_pf_value( GLX_SAMPLE_BUFFERS_ARB, 1, values, c );

			if ( pixelformat->samples )
				set_pf_value( GLX_SAMPLES_ARB, pixelformat->samples, values, c );
		}
		else
		{
		}*/
	}

	// finish
	values[ counter++ ] = None;

	Array = glXChooseFBConfig( glsk::detail::global.connection,
						glsk::detail::global.screen, values, &elements );

	if ( elements )
	{
		Result = Array[ 0 ];
		XFree( Array );

		return Result;
	}

	return None;
}


void glsk::config::detail::get_context( GLXFBConfig fbconfig )
{
	using namespace glsk::detail;

	if ( this->handle == None )
	{
		// create the GL context
		this->handle = glXCreateNewContext( global.connection, fbconfig, GLX_RGBA_TYPE, 0, true );

		if ( this->handle == None )
			throw unsuccessful();

		if ( !glXIsDirect( global.connection, this->handle ) )
		{
			//FIXME - see below
			std::cout << "(glsk) using indirect rendering." << std::endl;
			//throw unsuccessful();
		}
	}

	++this->context_refcount;
}


void glsk::config::detail::get_context_fallback( XVisualInfo* visual )
{
	using namespace glsk::detail;

	if ( this->handle == None )
	{
		this->handle = glXCreateContext( global.connection, visual, 0, true );

		if ( this->handle == None )
			throw unsuccessful();


		// FIXME: make this a commandline option
		if ( !glXIsDirect( global.connection, this->handle ) )
		{
			std::cout << "(glsk) using indirect rendering." << std::endl;
			//throw unsuccessful();
		}

	}

	++this->context_refcount;
}

void glsk::config::detail::release_context()
{
	using namespace glsk::detail;

	--context_refcount;

	if ( context_refcount == 0 )
	{
		// destroy our GL context
		glXDestroyContext( global.connection, handle );
		handle = None;
	}
}

#endif


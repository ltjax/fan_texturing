
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
**		filename:		win32_core.cpp
**
**		description:	shared functions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_core.cpp 114 2010-07-29 23:05:27Z ltjax $
**
*/

#include <windows.h>
#include <gl/gl.h>
#include <glsk/glsk.hpp>
#include <boost/assign.hpp>
#include "config.hpp"
#include "win32_window.hpp"
#include "win32_core.hpp"
#include "win32_main.hpp"

void glsk::detail::core::set_pixelformat( HDC dc, config& cfg )
{
	cfg.data->set_pixelformat( dc );
}

int glsk::detail::core::get_pixelformat_descriptor( HDC dc, config& cfg )
{
	return cfg.data->get_pixelformat_descriptor( dc );
}

void glsk::detail::core::get_physical_rc( HDC dc, config& cfg )
{

	if ( cfg.data->handle == NULL ) // create a physical context?
	{
		if ( cfg.data->major_version )
			init_wgl();
		
		// Legacy init?
		if ( !detail::wgl.GetExtensionString ||
			 !detail::wgl.CreateContextAttribs ||
			 cfg.data->major_version==0 )
		{
			cfg.data->handle = wglCreateContext( dc );
		}
		else
		{
			using namespace boost::assign;
			std::vector<int> AttribList;
			AttribList += WGL_CONTEXT_MAJOR_VERSION_ARB,cfg.data->major_version;
			AttribList += WGL_CONTEXT_MINOR_VERSION_ARB,cfg.data->minor_version;

			bool profile_supported = (strstr( detail::wgl.GetExtensionString(dc),
				"WGL_ARB_create_context_profile" ) != NULL);

			// do we have profile selection support?
			if ( profile_supported )
			{
				if ( cfg.data->core_profile )
				{
					AttribList += WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
						WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
				}
				else
				{
					AttribList += WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
				}
			}

			AttribList += 0;

			cfg.data->handle = detail::wgl.CreateContextAttribs( dc, NULL, &(AttribList[0]));
		}
	}

	if ( cfg.data->handle == NULL )
	{
		glsk::error_box( "Unable to create rendering context!" );
		throw unsuccessful();
	}

	cfg.data->context_refcount++;	
}

void glsk::detail::core::release_physical_rc( HDC dc, glsk::config& cfg )
{
	if ( wglGetCurrentDC() == dc )
		wglMakeCurrent( dc, 0 );

	if ( cfg.data->handle != NULL )
	{
		--cfg.data->context_refcount;

		if ( cfg.data->context_refcount == 0 )
		{
			wglDeleteContext( cfg.data->handle );
			cfg.data->handle = NULL;
		}
	}
}

/** Change the current workpath.
*/
void glsk::file_system::change_directory( const std::string& path )
{
	if ( !SetCurrentDirectory( path.c_str() ) )
		throw glsk::unsuccessful();
}


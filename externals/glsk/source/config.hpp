
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
**		filename:		config.hpp
**
**		description:	TODO
**		version-info:   $Id: config.hpp 110 2010-03-08 14:30:21Z ltjax $
**
*/

#include <set>
#include <boost/algorithm/string/predicate.hpp>

struct glsk::config::detail
{
	unsigned int			object_refcount;
	unsigned int			context_refcount;

	int						flags;
	int						color_bits;
	int						depth_bits;
	int						stencil_bits;
	int						samples;

	int						major_version;
	int						minor_version;
	int						core_profile;

	std::set< std::string >	extensions;
	
							detail();
							~detail();

	void					prepare_extension_string();
	
#ifdef _WIN32
	HGLRC					handle;

	int						get_pixelformat_descriptor( HDC dc );
	void					set_pixelformat_fallback( HDC dc, PIXELFORMATDESCRIPTOR* pfd );
	void					set_pixelformat( HDC dc );
#else
	GLXContext				handle;
	
	XVisualInfo*			get_visualinfo() const;
	GLXFBConfig				get_fbconfig() const;

	void					get_context_fallback( XVisualInfo* visual );
	void					get_context( GLXFBConfig fbconfig );
	void					release_context();
#endif
};



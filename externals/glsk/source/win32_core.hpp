
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
**		filename:		win32_core.hpp
**
**		description:	Shared functions, Access rights 'Hub'
**		target-system:	microsoft windows
**		version-info:   $Id: win32_core.hpp 113 2010-05-14 20:49:58Z ltjax $
**
*/

namespace glsk { namespace detail {

class core
{
public:
	static void set_pixelformat( HDC dc, glsk::config& cfg );
	static int get_pixelformat_descriptor( HDC dc, glsk::config& cfg );
	static void get_physical_rc( HDC dc, glsk::config& cfg ); 
	static void release_physical_rc( HDC dc, glsk::config& cfg );
	static inline HGLRC get_rc( const glsk::config& cfg ) { return cfg.data->handle; }
	static inline HDC get_dc( const glsk::window& wnd ) { return wnd.data->dc; }
	static inline HWND get_handle( const glsk::window& wnd ) { return wnd.data->handle; }

	static glsk::window* get_next( glsk::window* wnd ) { return wnd->data->next; }

	static inline void send_input_event( glsk::window& receiver, const glsk::input_event& event  )
	{
		receiver.data->send_input_event( event );
	}

};

}; };


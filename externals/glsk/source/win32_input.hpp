
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
**		filename:		win32_input.hpp
**
**		description:	input related functions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_input.hpp 101 2009-01-26 02:21:58Z ltjax $
**
*/

namespace glsk { namespace detail { namespace input {

	void			init();
	void			free();

	void			set_owner( glsk::window* wnd );
	void			enable();
	void			disable();

	inline void		enable( bool value ) { if ( value ) enable(); else disable(); }

	void			update();

	/*void			read_keyboard_events();
	void			read_mouse_events();

	void			clear_joystick_list();
	void			update_joysticks();*/

}}}


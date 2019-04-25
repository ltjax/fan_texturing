
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
**		filename:		x11_window.hpp
**
**		description:	window header
**		target-system:	X-Windows
**		version-info:   $Id: x11_window.hpp 102 2009-02-17 19:03:30Z ltjax $
**
*/


struct glsk::window::detail
{
	Window								handle;
	GLXWindow							drawable;

	glsk::config						rc;

	int									width;
	int									height;

	int									uwidth;
	int									uheight;

	int									px;
	int									py;

	int									flags;

	std::string							title;

	glsk::window*						next;
	glsk::window*						prev;

	glsk::window*						owner;

#ifdef _GLSK_USE_SIGNALS_
#  ifdef _GLSK_USE_LIBSIGC_
	sigc::signal< void, const std::string& >		signal_char;
	sigc::signal< void >							signal_redraw;
	sigc::signal< void, int, int >					signal_configure;
	sigc::signal< void, int, int >					signal_create;
	sigc::signal< void >							signal_close;
	sigc::signal< void >							signal_destroy;
	sigc::signal< void, const input_event& >		signal_input;
	sigc::signal< void, bool >						signal_activate;
	sigc::signal< void, int, int, input_event::event_type, int, int > signal_mouse;
#  else

	boost::signal< void ( int, int ) >				signal_create;
	boost::signal< void ( int, int ) >				signal_configure;
	boost::signal< void () >						signal_destroy;
	boost::signal< void () >						signal_redraw;
	boost::signal< void () >						signal_close;
	boost::signal< void ( const input_event& ) > 	signal_input;
	boost::signal< void ( const std::string& ) >	signal_char;
	boost::signal< void ( bool ) >					signal_activate;
	boost::signal< void ( int, int, input_event::event_type, int, int ) > signal_mouse;
#  endif
#endif

	explicit							detail( const glsk::config& rc );
										~detail();

	void								set_owner( glsk::window* owner );
	void								set_position( int x, int y );
	void								set_size( int width, int height );
	void								set_decorated( bool enabled );
	void								set_title( const std::string& title );
	void								set_fullscreen( bool enabled );

	void								release( bool send_signal = true );
	void								destroy();

	void								process_event( XEvent* event );

	void								encode_window_object();
	static int							decode_window_object_error( Display* display, XErrorEvent* event );
	static glsk::window*				decode_window_object( Window handle );

	void								open();

	enum {
		is_open = 1 << 0,
		is_visible = 1 << 1,
		is_fullscreen = 1 << 2,
		is_decorated = 1 << 3
	};

	struct motif_wm_hints
	{
		unsigned long flags;
		unsigned long functions;
		unsigned long decorations;
		long input_mode;
		unsigned long status;
	};

	struct object_pointer
	{
		glsk::window*	pointer;
	};
};


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
**		filename:		win32_window.hpp
**
**		description:	window related functions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_window.hpp 112 2010-05-08 19:09:50Z ltjax $
**
*/


/*

	Window Structure

	Note: the structure representing a window with a GL rendercontext

*/

struct glsk::window::detail
{
	enum
	{
		has_decoration = (1 << 0),// this means the window has a border and a titlebar
		is_fullscreen = (1 << 1),// if the window is in fullscreen mode, the client area matches the screenarea
		is_visible = (1 << 2),
		is_open = (1 << 3)
	};

	HWND						handle;			// handle to the physical window
	HDC							dc;				// the client device context
	glsk::config				config;
	glsk::window*				owner;

	int							flags;			// the flags as described above

	// current size as set by the window system
	int							width;			// dimension x
	int							height;			// dimension y

	// this is the size as set by the user - not the current size
	// this is equal to the current size if fullscreen is not active
	int							uwidth;			// dimension x
	int							uheight;		// dimension y

	// position
	int							px;				// x-position
	int							py;				// y-position

	std::string					title;			// title shown in the titlebar

	glsk::window*				next;
	static glsk::window*		listhead;
	static int					total_count;


	explicit					detail( const glsk::config& rc );
								~detail();

	void						open();
	void						destroy();

	void						set_decorated( bool value );
	void						set_size( int w, int h );
	void						set_position( int x, int y );
	void						set_fullscreen( bool value );
	void						move_relative( int dx, int dy ); // this uses MS windows' coord system with y+ -> down
	void						translate_client_to_screen( RECT* rectangle );
	void						send_mouse_event( int px, int py, input_event_type type, int index, int value );
	LRESULT						message_handler( HWND hwindow, UINT message, WPARAM wparam, LPARAM lparam );
	void						update_visibility();
	void						set_owner( glsk::window* owner );

	void						set_status( bool active );

	void						send_input_event( const glsk::input_event& e );

	// global message router
	static LRESULT CALLBACK							proc( HWND hwindow, UINT message, WPARAM wparam, LPARAM lparam );

#ifdef GLSK_USE_SIGNALS
#ifdef GLSK_USE_LIBSIGC
	sigc::signal< void, const std::string& >		signal_char;
	sigc::signal< void >							signal_redraw;
	sigc::signal< void, int, int >					signal_configure;
	sigc::signal< void, int, int >					signal_create;
	sigc::signal< void >							signal_close;
	sigc::signal< void >							signal_destroy;
	sigc::signal< void, const input_event& >		signal_input;
	sigc::signal< void, const mouse_event& >		signal_mouse;
	sigc::signal< void, bool >						signal_activate;
#else
	boost::signal< void ( const std::string& ) >	signal_char;
	boost::signal< void () >						signal_redraw;
	boost::signal< void ( int, int ) >				signal_configure;
	boost::signal< void ( int, int ) >				signal_create;
	boost::signal< void () >						signal_close;
	boost::signal< void () >						signal_destroy;
	boost::signal< void ( const input_event& ) >	signal_input;
	boost::signal< void ( const mouse_event& ) >	signal_mouse;
	boost::signal< void ( bool ) >					signal_activate;
#endif
#endif

};



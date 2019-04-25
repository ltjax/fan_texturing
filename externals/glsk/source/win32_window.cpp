
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
**		filename:		win32_window.c
**
**		description:	window functions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_window.cpp 113 2010-05-14 20:49:58Z ltjax $
**
*/

// system headers
#include <windows.h>
#include <gl/gl.h>

// global glsk header
#include <glsk/glsk.hpp>

// internal glsk headers
#include "win32_main.hpp"
#include "win32_window.hpp"
#include "config.hpp"
#include "win32_core.hpp"
#include "win32_input.hpp"

/** Retrieve a native HANDLE to the window passed in.
	This handle is only valid while the window is open.
	\param wnd The window to retrieve the handle from.
*/
long glsk::platform_win32::get_native_handle( glsk::window& wnd )
{
	BOOST_STATIC_ASSERT( sizeof(long) == sizeof(HWND) );
	return reinterpret_cast<long>(detail::core::get_handle(wnd));
}

glsk::window::detail::detail( const glsk::config& rc )
: config( rc )
{
	this->handle = 0;
	this->dc = 0;
	this->owner = 0;
	this->next = 0;

	this->flags = 0;

	this->width = this->uwidth = 0;
	this->height = this->uheight = 0;

	this->px = CW_USEDEFAULT;
	this->py = CW_USEDEFAULT;
}

glsk::window::detail::~detail()
{
	if ( this->owner == glsk::detail::get_window_list() )
	{
		glsk::detail::get_window_list() = this->next;
	}
	else
	{
		glsk::window* temp = glsk::detail::get_window_list();

		while ( temp->data->next != this->owner )
			temp = temp->data->next;

		temp->data->next = this->next;
	}
}

void glsk::window::detail::set_owner( glsk::window* owner )
{
	if ( this->owner != 0 || owner == 0 )
		throw failure( "owner already set or null" );

	this->owner = owner;
	this->next = glsk::detail::get_window_list();
	glsk::detail::get_window_list() = this->owner;
}

void glsk::window::detail::open()
{
	int			x, y, w, h;
	x = y = w = h = CW_USEDEFAULT;

	// backup these because they are changed in the creation process
	int			px = this->px;
	int			py = this->py;

	// if we got fullscreen set, use it
	if ( this->flags & is_fullscreen )
	{
		DEVMODE mode;
		EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &mode );

		w = mode.dmPelsWidth;
		h = mode.dmPelsHeight;
	}

	// if we already have sizes set, use them
	else if ( ( this->uwidth > 0 ) && ( this->uheight > 0 ) )
	{
		w = this->uwidth;
		h = this->uheight;
	}

	// try to actually create the window
	this->handle = CreateWindowEx( WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, glsk::detail::get_module_appname(), "",
		WS_OVERLAPPEDWINDOW, x, y, w, h, NULL, NULL, glsk::detail::get_module_instance(), this->owner );

	// test for failure
	if ( this->handle == NULL )
		throw glsk::unsuccessful();

	// mark as opened
	this->flags |= is_open;
	glsk::detail::get_window_count()++;

	// FIXME: do this directly upon creation?
	// update the window decoration
	this->set_decorated( this->flags & has_decoration );

	// fix the size
	if ( ( this->uwidth > 0 ) && ( this->uheight > 0 ) )
		set_size( this->uwidth, this->uheight );

	// fix the position
	if ( (px != int(CW_USEDEFAULT)) && (py != int(CW_USEDEFAULT)) )
		set_position( px, py );

	// fix the title
	if ( this->title.empty() == false )
		SetWindowText( this->handle, title.c_str() );

	// setup fullscreen if it's selected
	if ( this->flags & is_fullscreen )
		set_fullscreen( true );

	update_visibility();
}

void glsk::window::detail::set_decorated( bool value )
{
	if ( value )
		this->flags |= has_decoration;
	else
		this->flags &= ~has_decoration;

	// we don't have a physical window yet, bump out
	if ( ( this->flags & is_open ) == 0 )
		return;

	DWORD	style, exstyle;
	RECT	area;
	POINT	p0, p1;
	bool	is_visible = this->handle && IsWindowVisible( this->handle );

	if ( value )
	{
		style = WS_OVERLAPPEDWINDOW;
		exstyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	}
	else
	{
		style = WS_POPUP;
		exstyle = WS_EX_APPWINDOW;
	}

	// get client coordinates before the resize
	p0.x = p0.y = 0;
	ClientToScreen( this->handle, &p0 );

	GetClientRect( this->handle, &area );
	AdjustWindowRectEx( &area, style, FALSE, exstyle );

	SetWindowLong( this->handle, GWL_STYLE, style );
	SetWindowLong( this->handle, GWL_EXSTYLE, exstyle );

	//window->has_decoration = value;
    
	SetWindowPos( this->handle, 0, 0, 0,
		area.right-area.left, area.bottom-area.top,
		SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE );

	// we moved the client, undo that
	// get the client coords after the resize
	p1.x = p1.y = 0;
	ClientToScreen( this->handle, &p1 );	

	// calculate the movement delta
	p0.x = p1.x - p0.x;
	p0.y = p1.y - p0.y;

	if ( p0.x || p0.y )
	{
		move_relative( -p0.x, -p0.y );
	}

	if ( is_visible )
	{
		owner->show();
	}

	// Invalidate the whole desktop area to force a redraw
	// FIXME: only do that on fullscreen -> windowed?
	InvalidateRect( NULL, NULL, TRUE );
}

void glsk::window::detail::send_input_event( const glsk::input_event& e )
{
	//DEBUG_PRINT1( "Sending input event: %s!\n", glsk::keyboard::get_keyname( e.get_identifier() ) );
#ifdef GLSK_USE_VIRTUAL_EVENTS
	this->owner->on_input( e );
#endif
#ifdef GLSK_USE_SIGNALS
	this->signal_input( e );
#endif
}

void glsk::window::detail::set_size( int width, int height )
{
	this->uwidth = width;
	this->uheight = height;

	if ( this->flags & is_open )
	{
		RECT	a,b,n;

		GetClientRect( this->handle, &a );
		translate_client_to_screen( &a );

		SetRect( &n, 0, 0, width, height );
		AdjustWindowRectEx( &n, GetWindowLong( this->handle, GWL_STYLE ), FALSE,
			GetWindowLong( this->handle, GWL_EXSTYLE ) );

		SetWindowPos( this->handle, HWND_TOP,
			0, 0, n.right-n.left, n.bottom-n.top, SWP_NOMOVE );
		
		GetClientRect( this->handle, &b );
		translate_client_to_screen( &b );

		move_relative( a.left-b.left, a.bottom-b.bottom );
	}
}

void glsk::window::detail::set_position( int x, int y )
{
	// this is a bit more tricky since we need to translate into windows' shitty coordinate system

    this->px = x;
	this->py = y;

	if ( this->flags & is_open )
	{
		RECT		rectangle;
		RECT		workarea;
		int			dx, dy;

		SystemParametersInfo( SPI_GETWORKAREA, 0, (LPVOID)( &workarea ), 0);

		// calculate the desired position for the lower-left corner of the client rect in windows' coords
		dx = x;
		dy = (workarea.bottom - workarea.top)-y;

		GetClientRect( this->handle, &rectangle );
		translate_client_to_screen( &rectangle );

		move_relative( dx-rectangle.left, dy-rectangle.bottom );
	}
}

void glsk::window::detail::set_fullscreen( bool value )
{
	if ( value )
		this->flags |= is_fullscreen;
	else
		this->flags &= ~is_fullscreen;

	if ( this->flags & is_open )
	{
		if ( this->flags & is_fullscreen )
		{
			DEVMODE mode;

			EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &mode );

			SetWindowPos( this->handle, HWND_TOPMOST, 0, 0,
				mode.dmPelsWidth, mode.dmPelsHeight, 0 );
		}
		else
		{
			set_position( this->px, this->py );
			set_size( this->uwidth, this->uheight );
		}
	}
}

void glsk::window::detail::destroy()
{
	if ( this->handle && ( this->flags & is_open ) )
		DestroyWindow( this->handle );
}

void glsk::window::detail::send_mouse_event( int px, int py, const input_event_type type, int index, int value )
{
	if ( glsk::get_mouse_captured() )
		return;

	glsk::mouse_event e(px,py,type,index,value);

#ifdef GLSK_USE_VIRTUAL_EVENTS
	this->owner->on_mouse( e );
#endif
#ifdef GLSK_USE_SIGNALS
	this->signal_mouse( e );
#endif
}

void glsk::window::detail::set_status( bool active )
{
	if ( active )
	{
		glsk::detail::input::set_owner( this->owner );
	}

	glsk::detail::input::enable( active );
#ifdef GLSK_USE_VIRTUAL_EVENTS
	this->owner->on_activate( active );
#endif
#ifdef GLSK_USE_SIGNALS
	this->signal_activate( active );
#endif
}

LRESULT	glsk::window::detail::message_handler( HWND hwindow, UINT message, WPARAM wparam, LPARAM lparam )
{
	switch( message )
	{
		case WM_CREATE:
		{
			CREATESTRUCT*	data = reinterpret_cast< CREATESTRUCT* >( lparam );

			try {
				glsk::detail::core::set_pixelformat( this->dc, this->config );
				glsk::detail::core::get_physical_rc( this->dc, this->config );

				RECT rectangle;
				GetClientRect( this->handle, &rectangle );

				this->width = rectangle.right-rectangle.left;
				this->height = rectangle.bottom-rectangle.top;

				if ( (this->flags & is_fullscreen ) == 0 )
				{
					this->uwidth = this->width;
					this->uheight = this->height;
				}

#ifdef GLSK_USE_VIRTUAL_EVENTS
				owner->on_create( this->width, this->height );
#endif

#ifdef GLSK_USE_SIGNALS
				signal_create( this->width, this->height );
#endif
			}
			catch( glsk::unsuccessful )
			{
				// do not create the window if anything goes wrong
				return -1;
			}
			catch( std::runtime_error& x )
			{
				const char* Message = x.what();
				DEBUG_PRINT1( "Caught runtime error while creating window: %s\n", Message );
				return -1;
			}
			catch( ... )
			{
				DEBUG_PRINT0( "Caught unknown error while creating window\n" );
				return -1;
			}
		}
		return 0;

		case WM_DESTROY:
		{
			this->flags &= ~is_open;

#ifdef GLSK_USE_VIRTUAL_EVENTS
			owner->on_destroy();
#endif

#ifdef GLSK_USE_SIGNALS
			signal_destroy();
#endif

			glsk::detail::core::release_physical_rc( this->dc, this->config );

			ReleaseDC( this->handle, this->dc );

			SetWindowLongPtr( this->handle, GWLP_USERDATA, (LONG_PTR)0 );

			this->dc = 0;
			this->handle = 0;

			glsk::detail::get_window_count()--;

			if ( glsk::detail::get_window_count() == 0 )
			{
				// TODO: this ought to be cleaner
				glsk::detail::input::free();
			}
		}
		return 0;

		// antiflicker on resize
		case WM_ERASEBKGND:
			return 0;

			
		// Intersect screensaver and monitor power-down
		case WM_SYSCOMMAND:
		{
			if ( (wparam == SC_SCREENSAVE) || (wparam == SC_MONITORPOWER) )
				return 0;

			break;
		}

		case WM_ACTIVATE:
		{
			set_status( LOWORD( wparam ) != WA_INACTIVE );
		}
		return 0;

		case WM_ENTERMENULOOP:
		case WM_ENTERSIZEMOVE:
		{
			set_status( false );
		}
        return 0;

		case WM_EXITMENULOOP:
		case WM_EXITSIZEMOVE:
		{
			set_status( (GetActiveWindow() == hwindow) || (!IsIconic( hwindow )) );
		}
        return 0;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;

			BeginPaint( this->handle, &ps );
#ifdef GLSK_USE_VIRTUAL_EVENTS
			owner->on_redraw();
#endif

#ifdef GLSK_USE_SIGNALS
			signal_redraw();
#endif

			EndPaint( this->handle, &ps );	
		}
		return 0;

		case WM_MOVE:
		{
			if ( this->flags & is_fullscreen )
				return 0;

            // update the positional values	
			RECT rectangle, workarea;

			SystemParametersInfo( SPI_GETWORKAREA, 0, (LPVOID)( &workarea ), 0);
			GetClientRect( this->handle, &rectangle );
			translate_client_to_screen( &rectangle );

			this->px = rectangle.left;
			this->py = (workarea.bottom-workarea.top) - rectangle.bottom;
		}
		return 0;

		case WM_SIZE:
		{
			if ( this->width != LOWORD( lparam ) || this->height != HIWORD( lparam ) )
			{
				this->width = LOWORD( lparam );
				this->height = HIWORD( lparam );

				if ( (this->flags & is_fullscreen ) == 0 )
				{
					this->uwidth = this->width;
					this->uheight = this->height;
				}
#ifdef GLSK_USE_VIRTUAL_EVENTS
				owner->on_configure( this->width, this->height );
#endif

#ifdef GLSK_USE_SIGNALS
				signal_configure( this->width, this->height );
#endif
			}
		}
		return 0;

		case WM_CLOSE:
		{
#ifdef GLSK_USE_VIRTUAL_EVENTS
			owner->on_close();
#endif
#ifdef GLSK_USE_SIGNALS
			signal_close();
#endif
		}
		return 0;


		case WM_CHAR:
		{
			char	string[ 2 ] = { '\0', '\0' };

			string[ 0 ] = (char)wparam;

#ifdef GLSK_USE_VIRTUAL_EVENTS
			owner->on_char( string );
#endif
#ifdef GLSK_USE_SIGNALS
			signal_char( string );
#endif
		}
		return 0;

		case WM_MOUSEMOVE:		send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_move, 0, 0 ); return 0;
		case WM_LBUTTONUP:		send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_button, 1, 0 ); return 0;
		case WM_RBUTTONUP:		send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_button, 2, 0 ); return 0;
		case WM_MBUTTONUP:		send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_button, 3, 0 ); return 0;
		case WM_LBUTTONDOWN:	send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_button, 1, 1 ); return 0;
		case WM_RBUTTONDOWN:	send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_button, 2, 1 ); return 0;
		case WM_MBUTTONDOWN:	send_mouse_event( LOWORD( lparam ), this->height-HIWORD( lparam ), glsk::mouse_button, 3, 1 ); return 0;

	default:
		break;
	};

	return DefWindowProc( hwindow, message, wparam, lparam );
}

// The microsoft compiler seems upset about the pointer casts, but they are legal according to MSDN
#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable : 4244 4312 )
#endif

LRESULT CALLBACK glsk::window::detail::proc( HWND hwindow, UINT message, WPARAM wparam, LPARAM lparam )
{
	// special part to init the userdata
	if ( message == WM_NCCREATE )
	{
		CREATESTRUCT*	data = reinterpret_cast< CREATESTRUCT* >( lparam );
		glsk::window*	window = reinterpret_cast< glsk::window* >( data->lpCreateParams );


		SetWindowLongPtr( hwindow, GWLP_USERDATA, (LONG_PTR)window );

		window->data->handle = hwindow;
		window->data->dc = GetDC( hwindow );

		return DefWindowProc( hwindow, message, wparam, lparam );
	}

	glsk::window* window = reinterpret_cast< glsk::window* >( GetWindowLongPtr( hwindow, GWLP_USERDATA ) );

	if ( !window )
		return DefWindowProc( hwindow, message, wparam, lparam );

	return window->data->message_handler( hwindow, message, wparam, lparam );
}

#ifdef _MSC_VER
#  pragma warning( pop ) 
#endif

void glsk::window::detail::update_visibility()
{
	if ( this->flags & is_open )
	{
		if ( this->flags & is_visible )
		{
			ShowWindow( this->handle, SW_SHOW );
			UpdateWindow( this->handle );
		}
		else
		{
			ShowWindow( this->handle, SW_HIDE );
		}
	}
}

void glsk::window::detail::translate_client_to_screen( RECT* rectangle )
{
	POINT delta = { 0, 0 };

	ClientToScreen( this->handle, &delta );

	// we can pretty much assume linear movement
	rectangle->left += delta.x;
	rectangle->right += delta.x;

	rectangle->top += delta.y;
	rectangle->bottom += delta.y;
}

void
glsk::window::detail::move_relative( int dx, int dy )
{
	RECT area;
	GetWindowRect( this->handle, &area );
	SetWindowPos( this->handle, 0, area.left+dx, area.top+dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER );	
}

/** Create a new window with the given config.
*/
glsk::window::window( const glsk::config& rc )
: data( new detail( rc ) )
{
	data->set_owner( this );
}

glsk::window::~window()
{
	//FIXME: destroy this window
	if ( this->data->handle )
		DestroyWindow( this->data->handle );
	
	delete data;
}

bool glsk::window::select_rc()
{
	return wglMakeCurrent( data->dc, data->config.data->handle ) != 0;
}

void glsk::window::swap_buffers()
{
	SwapBuffers( data->dc );
}

/** Get the rendering context this window is using.
*/
const glsk::config& glsk::window::get_rc() const
{
	return data->config;
}

/** Opens the physical window.
	Throws unsuccessful in case it fails.
*/
void glsk::window::open()
{ data->open(); }

/**	Destroys the physical window.
*/
void glsk::window::destroy()
{ data->destroy(); }

/** Checks whether the window is open or not.
*/
bool glsk::window::is_open()
{ return (this->data->flags & detail::is_open) != 0; }

/**	Displays an error box that is modal to this window.
	Does nothing if the window is not open yet.
*/
void
glsk::window::error_box( const std::string& text ) const
{
	if ( !this->data->handle )
		return;

	MessageBox( this->data->handle, text.c_str(), "Error", MB_ICONERROR | MB_OK );
}

/** Show this window.
*/
void glsk::window::show()
{
	// mark this as visible
	data->flags |= detail::is_visible;

	// update visibility
	data->update_visibility();
}

/** Hide this window.
*/
void glsk::window::hide()
{
	// mark as not visible
	data->flags &= ~detail::is_visible;

	// update visibility
	data->update_visibility();
}

/** Get the visibilty state of this window.
*/
bool glsk::window::is_visible() const
{
	return data->handle && IsWindowVisible( data->handle );
}

/** Turn window decorations on or off.
	It tries it's best to convince the window manager to use, or not to use window decorations.
	On Microsoft Windows, this function always works.
	\param value true turns window decorations on, false turns them off
*/
void glsk::window::set_decorated( const bool value )
{
	data->set_decorated( value );
}


/** Set this window's size.
*/
void
glsk::window::set_size( const int width, const int height )
{
	data->set_size( width, height );
}

/** Set this window's width.
*/
void
glsk::window::set_width( const int width )
{
	data->set_size( width, get_height() );
}

/** Set this window's height.
*/
void
glsk::window::set_height( const int height )
{
	data->set_size( get_width(), height );
}

/** Set this window's position.
	\note if decoration is turned on, the window manager will often interfere and this function will not work as expected
	\param x the x coordinate of this window, relative to the left side of the screen
	\param y the y coordinate of this window, relative to the lower side of the screen
*/
void
glsk::window::set_position( const int x, const int y )
{
	data->set_position( x, y );
}


/** Turn fullscreen mode of this window on or off.
	\param value true to turn on fullscreen mode, false to turn it off
*/
void glsk::window::set_fullscreen( const bool value )
{
	data->set_fullscreen( value );
}

/** Check the fullscreen state of this window.
*/
bool glsk::window::get_fullscreen() const
{
	return ( data->flags & detail::is_fullscreen ) != 0;
}

/** Check whether this window wants decoration or not.
*/
bool glsk::window::get_decorated() const
{
	return ( data->flags & detail::has_decoration ) != 0;
}

/** Sets this window's title.
	The title set will appear in the windows titlebar on most windowmanagers.
	\param title The title string.
*/
void glsk::window::set_title( const std::string& title )
{
	// do we need to change anything?
	if ( title == data->title )
		return;

	data->title = title;

	if ( data->flags & detail::is_open )
	{
		if ( title.length() > 0 )
			SetWindowText( this->data->handle, title.c_str() );
		else
			SetWindowText( this->data->handle, "" );
	}
}

/** Get this window's title.
*/
const std::string& glsk::window::get_title() const
{
	return data->title;
}

/** Get this window's width.
*/
int glsk::window::get_width() const
{
	return data->width; //FIXME: uwidth?
}

/** Get this window's height.
*/
int glsk::window::get_height() const
{
	return data->height; // FIXME: uheight?
}

/** Get this window's size.
*/
boost::tuple<int,int> glsk::window::get_size() const
{
	return boost::make_tuple( data->width, data->height ); // FIXME: uwidth, uheight?
}

/** Get this window's position.
*/
boost::tuple<int,int> glsk::window::get_position() const
{
	return boost::make_tuple( data->px, data->py );
}

/** Char signal.
	This is emitted whenever the window receives string input.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void, const std::string& >&
#  else
boost::signal< void ( const std::string& ) >& 
#endif
glsk::window::signal_char()
{ return data->signal_char; }
#endif

/** Redraw signal.
	This is emitted whenever the system requests for this window to be redrawn.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void >&
#  else
boost::signal< void () >&
#  endif
glsk::window::signal_redraw()
{ return data->signal_redraw; }
#endif

/** Configure signal.
	This is emitted whenever this window changes size.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void, int, int >&
#  else
boost::signal< void ( int, int ) >&
#  endif
glsk::window::signal_configure()
{ return data->signal_configure; }
#endif

/** Create signal.
	This is emitted right after the window is created.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void, int, int >&
#  else
boost::signal< void ( int, int ) >&
#  endif
glsk::window::signal_create()
{ return data->signal_create; }
#endif

/** Close signal.
	This is emitted when the user requests this window to be closed.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void >&
#  else
boost::signal< void () >&
#  endif
glsk::window::signal_close()
{ return data->signal_close; }
#endif

/** Destroy signal.
	This function is emitted when the window is destroyed.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void >&
#  else
boost::signal< void () >&
#  endif
glsk::window::signal_destroy()
{ return data->signal_destroy; }
#endif

/** Input event signal.
	This function is called whenever there is new an incoming input event.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void, const glsk::input_event& >&
#  else
boost::signal< void ( const glsk::input_event& ) >&
#  endif
glsk::window::signal_input()
{ return data->signal_input; }
#endif

/** Mouse event signal.
	This function is called whenever there is new event based input coming in.
	The parameters used are the x and y coordinates of the associated event, the type
	of the event (mouse_axis_move or mouse_button), the index of the button and the value of the button respectively.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void, const glsk::mouse_event& >&
#  else
boost::signal< void ( const glsk::mouse_event& ) >&
#  endif
glsk::window::signal_mouse()
{ return data->signal_mouse; }
#endif

/** Activate signal.
*/
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
sigc::signal< void, bool >&
#  else
boost::signal< void ( bool ) >&
#  endif
glsk::window::signal_activate()
{ return data->signal_activate; }
#endif



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
**		filename:		x11_window.cpp
**
**		description:	window header
**		target-system:	X-Windows
**		version-info:   $Id: x11_window.cpp 105 2009-06-04 22:48:30Z ltjax $
**
*/

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
// system includes
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/extensions/xf86vmode.h>
#include <dlfcn.h>
#include <dirent.h>

// global include
#include <glsk/glsk.hpp>
#include "x11_main.hpp"
#include "x11_window.hpp"
#include "config.hpp"
#include "x11_input.hpp"

// Attention this "thing" defines  ugly macros
#include <X11/Xlibint.h>

namespace {
struct SetInputFocusState
{
  Display *dpy;
  _XAsyncHandler async;
  unsigned long set_input_focus_req;
  unsigned long get_input_focus_req;
};

Bool set_input_focus_handler(
    Display *dpy  // must be called dpy
    , xReply  *rep
    , char    *buf
    , int      len
    , XPointer data)
{
  SetInputFocusState *state = reinterpret_cast<SetInputFocusState *>(data);

  if( dpy->last_request_read == state->set_input_focus_req )
  {
    if( rep->generic.type == X_Error &&
        rep->error.errorCode == BadMatch )
    {
      // Consume BadMatch errors, since we have no control over them.
      return True;
    }
  }

  if( dpy->last_request_read == state->get_input_focus_req )
  {
    xGetInputFocusReply replbuf;
    xGetInputFocusReply *repl;

    if (rep->generic.type != X_Error)
    {
      // Actually does nothing, since there are no additional bytes
      // to read, but maintain good form.
      repl = reinterpret_cast<xGetInputFocusReply *> (
        _XGetAsyncReply(dpy
          , reinterpret_cast<char *>(&replbuf)
          , rep
          , buf
          , len
          , (sizeof(xGetInputFocusReply) - sizeof(xReply)) >> 2,
            True)
        );
    }

    DeqAsyncHandler(state->dpy, &state->async);

    delete state;

    return (rep->generic.type != X_Error);
  }

  return False;
}

void
safe_set_input_focus(Display *dpy, Window window, int revert_to, Time time)
{
  SetInputFocusState *state;
  state = new SetInputFocusState;
  state->dpy = dpy;

  LockDisplay(dpy);

  state->async.next = dpy->async_handlers;
  state->async.handler = set_input_focus_handler;
  state->async.data = reinterpret_cast<XPointer>( state );
  dpy->async_handlers = &state->async;

  {
    xSetInputFocusReq *req;

    GetReq(SetInputFocus, req);
    req->focus = window;
    req->revertTo = revert_to;
    req->time = time;
    state->set_input_focus_req = dpy->request;
  }

  {
    xReq *req;

    GetEmptyReq(GetInputFocus, req);
    state->get_input_focus_req = dpy->request;
  }

  UnlockDisplay(dpy);
  SyncHandle();
}
}


glsk::window::window( const glsk::config& rc )
: data( new detail( rc ) )
{
	data->set_owner( this );
}

glsk::window::~window()
{
	delete data;
}

void glsk::window::open()
{
	data->open();
}

void glsk::window::destroy()
{
	data->destroy();
}

bool glsk::window::is_open()
{
	return ( data->flags & detail::is_open ) != 0;
}

void glsk::window::set_title( const std::string& text )
{
	data->set_title( text );
}

void glsk::window::set_decorated( bool enabled )
{
	data->set_decorated( enabled );
}

void glsk::window::show()
{
	using namespace glsk::detail;

	data->flags |= detail::is_visible;

	if ( data->handle )
	{
		XMapWindow( global.connection, data->handle );
	}
}

void glsk::window::hide()
{
	using namespace glsk::detail;

	XUnmapWindow( global.connection, data->handle );

	data->flags &= ~detail::is_visible;
}

bool glsk::window::is_visible() const
{
	return (data->flags & detail::is_visible) != 0;
}

const std::string& glsk::window::get_title() const
{
	return data->title;
}

void glsk::window::detail::set_title( const std::string& title )
{
	using namespace glsk::detail;

	this->title = title;

	if ( this->handle )
	{
		// convert the string we got to a text property
		XTextProperty property;
		char* string = new char[ title.length() + 1 ];

		strcpy( string, title.c_str() );

		if ( XmbTextListToTextProperty( global.connection, &string,
				1, XTextStyle, &property ) != Success )
		{
			delete[] string;
			return;
		}

		// set the physical window name
		XSetWMName( global.connection, this->handle, &property );

		XFree( property.value );
		delete[] string;
	}
}

int glsk::window::get_width() const
{
	return data->width;
}

int glsk::window::get_height() const
{
	return data->height;
}

glsk::int2 glsk::window::get_size() const
{
	return int2( data->width, data->height );
}

glsk::int2 glsk::window::get_position() const
{
	return int2( data->px, data->py );
}

bool glsk::window::get_decorated() const
{
	return ( data->flags & detail::is_decorated ) != 0;
}

bool glsk::window::get_fullscreen() const
{
	return ( data->flags & detail::is_fullscreen ) != 0;
}

void glsk::window::set_width( const int width )
{
	data->set_size( width, data->uheight );
}

void glsk::window::set_height( const int height )
{
	data->set_size( data->uwidth, height );
}

void glsk::window::set_size( const int width, const int height )
{
	data->set_size( width, height );
}

void glsk::window::set_position( const int x, const int y )
{
	data->set_position( x, y );
}

void glsk::window::set_fullscreen( const bool value )
{
	data->set_fullscreen( value );
}

#include <iostream>

void glsk::window::detail::set_fullscreen( bool value )
{
    using namespace glsk::detail;

    this->flags &= ~is_fullscreen;

	if ( value )
		this->flags |= is_fullscreen;
	
	if ( !this->handle )
		return;

	if ( this->flags & is_fullscreen )
	{
		XWindowChanges changes;
		//Window root_window = RootWindow( global.connection, global.screen );
		//XWindowAttributes root_attribs;

		//XGetWindowAttributes( global.connection, root_window, &root_attribs );

		XF86VidModeModeLine     current_mode;
	    int                     dotclock = 0;

	    XF86VidModeGetModeLine( global.connection, global.screen, &dotclock, &current_mode );
    
		changes.x = 0;
		changes.y = 0;
		//this->width = changes.width = root_attribs.width;
		//this->height = changes.height = root_attribs.height;
		this->width = changes.width = current_mode.hdisplay;
		this->height = changes.height = current_mode.vdisplay;
		
		//std::cout << "changes: " << changes.width << "x" << changes.height << std::endl;
		
		XConfigureWindow( global.connection, this->handle,
					CWWidth | CWHeight | CWX | CWY, &changes );
		XRaiseWindow( global.connection, this->handle );
	}
	else
	{
		set_size( this->uwidth, this->uheight );
	}
}


void glsk::window::detail::set_size( int width, int height )
{
    using namespace glsk::detail;

	this->uwidth = this->width = width;
	this->uheight = this->height = height;

	if ( this->handle )
	{
		XWindowChanges changes;
		Window root_window = RootWindow( global.connection, global.screen );
		XWindowAttributes root_attribs;

		XGetWindowAttributes( global.connection, root_window, &root_attribs );

		changes.x = this->px;
		changes.y = root_attribs.height - height - this->py;
		changes.width = this->width = width;
		changes.height = this->height = height;

		XConfigureWindow( global.connection, this->handle,
					CWWidth | CWHeight | CWX | CWY, &changes );
	}
}

void glsk::window::detail::set_position( int x, int y )
{
	using namespace glsk::detail;

	this->px = x;
	this->py = y;

	if ( this->handle )
	{
		Window root_window = RootWindow( global.connection, global.screen );
		XWindowAttributes root_attribs;
		XGetWindowAttributes( global.connection, root_window, &root_attribs );

		y = root_attribs.height-this->height-y;

		XMoveWindow( global.connection, this->handle, x, y );
	}
}


void glsk::window::detail::set_owner( glsk::window* owner )
{
	using namespace glsk::detail;
	this->owner = owner;

	this->next = global.window_head;
	this->prev = 0;

	if ( this->next )
		core::get_internal( *this->next ).prev = owner;

	global.window_head = owner;
}

glsk::window::detail::detail( const glsk::config& cfg )
: rc( cfg )
{
	this->handle = 0;
	this->flags = 0;
	this->px = this->py = 0;
	this->width = this->uwidth = 400;
	this->height = this->uheight = 300;
	this->next = this->prev = this->owner = 0;
}

glsk::window::detail::~detail()
{
	using namespace glsk::detail;

	if ( global.window_head == owner )
		global.window_head = this->next;

	if ( this->next )
		core::get_internal( *this->next ).prev = this->prev;

	if ( this->prev )
		core::get_internal( *this->prev ).next = this->next;
}


void glsk::window::detail::open()
{
	using namespace glsk::detail;

	XSetWindowAttributes	attributes;
	unsigned int			attributes_mask = 0;
	XVisualInfo*			visual_info = 0;
	Window					root_window = RootWindow( global.connection, global.screen );
	Atom 					wm_delete_atom = XInternAtom( global.connection, "WM_DELETE_WINDOW", true );

	try {
		GLXFBConfig fb_config;
		
		if ( (global.flags & glx_fallback) == 0 )
		{
			// select the appropriate fbconfig
			fb_config = core::get_internal( rc ).get_fbconfig();

			// did we find a suitable fb_config?
			if ( fb_config == None )
				throw unsuccessful();

			visual_info = glXGetVisualFromFBConfig( global.connection, fb_config );
		}
		else
		{
			visual_info = core::get_internal( rc ).get_visualinfo();
		}

		//std::cout << "Visual ID: " << visual_info->visualid << std::endl;

		// did we find a matching visual info too?
		if ( visual_info == 0 )
			throw unsuccessful();

		// init the colormap
		attributes.colormap = XCreateColormap( global.connection, root_window,
						visual_info->visual, AllocNone );

		attributes.border_pixel = 0;

		attributes.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
			StructureNotifyMask | PointerMotionMask | FocusChangeMask;
		
		// if override_redirect is true, this window will act as a popup
		attributes.override_redirect = ( this->flags & is_decorated )==0;
		
		attributes_mask = CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

		int viewport[ 4 ] = { this->px, this->py, this->width, this->height };

		
		// create the physical window
		this->handle = XCreateWindow( global.connection, root_window, viewport[ 0 ], viewport[ 1 ],
			viewport[ 2 ], viewport[ 3 ], 0, visual_info->depth, InputOutput, visual_info->visual,
			attributes_mask, &attributes );

		XSync( global.connection, 0 );
		//_glsk_wait_for_map_notify( window->handle );

		// flag this window as open
		flags |= is_open;

		if ( this->handle == None )
			throw unsuccessful();

		// increase the global counter
		++global.window_open_count;

		set_position( this->px, this->py );

		XSetWMProtocols( global.connection, this->handle, &wm_delete_atom, 1 );

		// set the object pointer -TODO: make this 64-bit compatible
		//printf( "Created window with address: %p\n", window );
		encode_window_object();

		// fallback if we are below GLX version 1.3 (damn ATI drivers)
		if ( (global.flags & glx_fallback) == 0 ) // GLX >= 1.3
		{
			core::get_internal( rc ).get_context( fb_config );

			this->drawable = glXCreateWindow( global.connection, fb_config, handle, 0 );

			if ( this->drawable == None )
				throw unsuccessful();
		}
		else // GLX <= 1.2
		{
			core::get_internal( rc ).get_context_fallback( visual_info );
		}
		
		XFree( visual_info );
		visual_info = 0;

		if ( this->flags & is_decorated )
			set_decorated( true );

		if ( this->title.empty() == false )
			set_title( this->title );
		
		if ( this->flags & is_fullscreen )
			set_fullscreen( true );
		
		_GLSK_INVOKE_EVENT( create( this->width, this->height ) );
	}
	catch ( glsk::unsuccessful )
	{
		if ( visual_info )
			XFree( visual_info );
		
		// TODO: roll back
		release( false );
		throw;
	}
}


void glsk::window::detail::release( bool send_signal )
{
	using namespace glsk::detail;
	if ( (this->flags & is_open) == 0 )
		return;

	flags &= ~is_open;

	// call the destroy event handler
	if ( send_signal )
	{
		_GLSK_INVOKE_EVENT( destroy() );
	}

	// glx 1.3 and higher
	if ( (global.flags & glx_fallback) == 0 )
	{
		// TODO: this unbinding is not quite right, could be considered a side effect
		// unbind our GL context, if it's bound
		if ( glXGetCurrentDrawable() == this->drawable )
			glXMakeContextCurrent( global.connection, None, None, None );

		if ( glXGetCurrentContext() == core::get_internal( rc ).handle )
			glXMakeContextCurrent( global.connection, None, None, None );

		core::get_internal( rc ).release_context();

		// destroy the gl window
		if ( drawable != None )
		{
			glXDestroyWindow( global.connection, drawable );
			drawable = None;
		}

		// destroy the window
		if ( handle != None )
			XDestroyWindow( global.connection, handle );
	}
	else // glx 1.2 and less
	{
		// unbind gl context
		if ( glXGetCurrentContext() == core::get_internal( rc ).handle )
			glXMakeContextCurrent( global.connection, None, None, None );

		core::get_internal( rc ).release_context();

		// destroy the window
		if ( handle != None )
			XDestroyWindow( global.connection, handle );
	}

	handle = None;

	--global.window_open_count;
}

void glsk::window::detail::destroy()
{
	using namespace glsk::detail;

	if ( ( handle != None ) && ( flags & is_open ) )
	{
		XEvent event;

		event.xclient.type = ClientMessage;
		event.xclient.window = handle;
		event.xclient.display = global.connection;
		event.xclient.message_type = global.delete_atom;
		event.xclient.format = 32;

		XSendEvent( global.connection, handle, false, 0, &event );
	}
}

void glsk::window::detail::process_event( XEvent* event )
{
	using namespace glsk::detail;

	switch( event->type )
	{
		case ReparentNotify:
		{
			if ( (flags & is_decorated) == 0 )
			{
				XSetWindowAttributes	attributes;

				// apparently, we just released our decoration - turn on the redirect override
				attributes.override_redirect = true;

				XChangeWindowAttributes( global.connection, handle, CWOverrideRedirect, &attributes );
			}

			return;
		};

		case MotionNotify:
		{
			input_update_pointer_move( event->xmotion.x, event->xmotion.y, this->owner );

			/*if ( _glsk_input_update_pointer_move( event->xmotion.x, event->xmotion.y, window ) == 0 )
				_glsk_send_mouse_event( window, event->xmotion.x, window->height-event->xmotion.y,
							GLSK_ME_TYPE_MOVE, 0 );*/
			return;
		};

		case ButtonPress:
		{
			XRaiseWindow( global.connection, handle );
			safe_set_input_focus(global.connection, this->handle, RevertToPointerRoot, CurrentTime );
			// XSetInputFocus( global.connection, this->handle, RevertToPointerRoot, CurrentTime );

			input_update_pointer_button( event->xbutton.x, event->xbutton.y, event->xbutton.button, true, this->owner );

			return;
		};

		case ButtonRelease:
		{
			input_update_pointer_button( event->xbutton.x, event->xbutton.y, event->xbutton.button, false, this->owner );

			return;
		};

		case MapNotify:
		{
			XRaiseWindow( global.connection, handle );
			safe_set_input_focus( global.connection, handle, RevertToPointerRoot, CurrentTime );
			// XSetInputFocus( global.connection, handle, RevertToPointerRoot, CurrentTime );
			return;
		};

		case DestroyNotify:
		{
			handle = None;
			return;
		};

		case KeyPress:
		{
			KeySym			key_symbol;
			char			buffer[ 5 ] = { '\0', '\0', '\0', '\0', '\0' };

			XLookupString( &event->xkey, buffer, 4, &key_symbol, NULL );

			if ( key_symbol )
			{
				if ( (key_symbol>>8) == 0 )
				{
					std::string text( buffer );
					
					_GLSK_INVOKE_EVENT( char( text ) );
				}

				unsigned int identifier = input_translate_key( key_symbol );

				if ( identifier )
				{
					const glsk::input_event ie( 0, glsk::input_event::key, identifier, 1 );

					_GLSK_INVOKE_EVENT( input( ie ) );
				}
			}
			return;
		}

		case KeyRelease:
		{
			KeySym key_symbol = XKeycodeToKeysym( global.connection, event->xkey.keycode, 0 );
			unsigned int identifier;

			if ( key_symbol && (identifier = input_translate_key( key_symbol )) )
			{
				const glsk::input_event ie( 0, glsk::input_event::key, identifier, 0 );
				
				_GLSK_INVOKE_EVENT( input( ie ) );
			}

			return;
		}

		case ConfigureNotify:
		{
			XWindowAttributes root_attribs;
			Window root_window = RootWindow( global.connection, global.screen );

			XGetWindowAttributes( global.connection, root_window, &root_attribs );

			width = event->xconfigure.width;
			height = event->xconfigure.height;

			if ( (flags & is_fullscreen) == 0 )
			{
				uwidth = width;
				uheight = height;
				px = event->xconfigure.x;
				py = root_attribs.height - event->xconfigure.height - event->xconfigure.y;
			}

			_GLSK_INVOKE_EVENT( configure( width, height ) );
			
			return;
		}

		case Expose:
		{
			_GLSK_INVOKE_EVENT( redraw() );
			return;
		}

		/** client messages are typically not send by X, but a window manager might use them.
		*/
		case ClientMessage:
		{
			if ( event->xclient.message_type == global.protocols_atom )
			{
				// the close event
				_GLSK_INVOKE_EVENT( close() );
			}
			else if ( event->xclient.message_type == global.delete_atom )
			{
				release( true );
			}
			else
			{
				printf( "unrecognized client-message type: %s \n",
						XGetAtomName( global.connection, event->xclient.message_type ) );
			}

			return;
		};

		case UnmapNotify:
		{
			return;
		};

		case FocusIn:
		{
			if ( (global.window_focus != 0) && (global.window_focus != owner) )
			{
				printf( "(glsk) Warning: another window is still focused.\n" );
			}

			global.window_focus = owner;

			input_update_pointer_owner();

			return;
		};

		case FocusOut:
		{
			if ( global.window_focus && (global.window_focus != owner) )
			{
				printf( "(glsk) Warning: losing focus without having focus.\n" );
			}

			global.window_focus = 0;

			input_update_pointer_owner();

			return;
		};

		default:
			// unrecognized event -> terminate
			printf( "(glsk) unrecognized event: %i - stopping.\n", event->type );
			throw glsk::failure( "unrecognized event - stopping." );
	};
}

int glsk::window::detail::decode_window_object_error( Display* display, XErrorEvent* event )
{
	glsk::detail::global.flags |= glsk::detail::decode_error;

	return 1;
}

void glsk::window::detail::encode_window_object()
{
	/*using namespace glsk::detail;
	object_pointer obj;

	obj.pointer = this->owner;

	XChangeProperty( global.connection, this->handle, global.object_atom, global.object_atom,
		32, PropModeReplace, (unsigned char*)&obj, sizeof( object_pointer )/sizeof( int ) );*/
}

glsk::window* glsk::window::detail::decode_window_object( Window handle )
{
    using namespace glsk::detail;

	glsk::window*			result = 0;
	/*Atom					return_type;
	int						return_format;
	unsigned long			return_count;
	unsigned long			return_rest;
	object_pointer* 		obj_ptr = 0;


	if ( handle == None )
		return 0;

	XSetErrorHandler( decode_window_object_error );

	if ( XGetWindowProperty( global.connection, handle,
				global.object_atom, 0, sizeof( object_pointer )/sizeof( int ), 0,
				global.object_atom, &return_type, &return_format, &return_count,
				&return_rest, (unsigned char**)&obj_ptr ) == Success )
	{
		// decode the object pointer
		result = obj_ptr->pointer;
		XFree( obj_ptr );
	}
	else
	{
		result = 0;
	}

	XSetErrorHandler( 0 );

	if ( global.flags & decode_error )*/
	{
		global.flags &= ~decode_error;

		// it wasn't able to decode the pointer, try a manual lookup
		result = global.window_head;

		while ( result )
		{
			if ( core::get_internal( *result ).handle == handle )
				return result;

			result = core::get_internal( *result ).next;
		}
	}

	return result;
}

#define MWM_HINTS_DECORATIONS   (1L << 1)
#define _XA_MOTIF_WM_HINTS		"_MOTIF_WM_HINTS"

void glsk::window::detail::set_decorated( bool enable )
{
	using namespace glsk::detail;

	if ( enable )
		this->flags |= is_decorated;
	else
		this->flags &= ~is_decorated;

	if ( !this->handle )
		return;
	
	
	motif_wm_hints			hints_object;
	motif_wm_hints*			hints_pointer  = 0;
	Atom					hints_atom = None;
	int						return_format = 0;
	Atom					return_type = None;
	unsigned long			return_count = 0;
	unsigned long			return_rest = 0;
	XSetWindowAttributes	attributes;

	attributes.override_redirect = false;

	XChangeWindowAttributes( global.connection, this->handle, CWOverrideRedirect, &attributes );

	if ( enable )
	{
		//XMapRaised( global.connection, window->handle );
		XUnmapWindow( global.connection, this->handle );
		XMapWindow( global.connection, this->handle );
	}
	XFlush( global.connection );


	//printf( "changing decoration %i\n", has_decoration );

	hints_object.flags = MWM_HINTS_DECORATIONS;
	hints_object.decorations = enable;

	// try getting the window-decoration(hints) extension atom
	hints_atom = XInternAtom( global.connection, _XA_MOTIF_WM_HINTS, false );


	XGetWindowProperty ( global.connection, this->handle,
		hints_atom, 0, sizeof (motif_wm_hints)/sizeof (int),
		false, AnyPropertyType, &return_type, &return_format, &return_count,
		&return_rest, (unsigned char **)&hints_pointer);

	if ( return_type != None )
	{
		hints_pointer->flags |= MWM_HINTS_DECORATIONS;
		hints_pointer->decorations = hints_object.decorations;
	}
	else
	{
		hints_pointer = &hints_object;
	}

	XChangeProperty ( global.connection, this->handle, hints_atom,
		hints_atom, 32, PropModeReplace, (unsigned char *)hints_pointer,
		sizeof (motif_wm_hints)/sizeof (int));

	if ( hints_pointer != &hints_object )
		XFree ( hints_pointer );

	XFlush( global.connection );
	
}

bool glsk::window::select_rc()
{
	using namespace glsk::detail;

	if ( (global.flags & glx_fallback) == 0 )
	{
		// GLX >= 1.3
		return glXMakeContextCurrent( global.connection, data->drawable,
					data->drawable, core::get_internal( data->rc ).handle ) != 0;
	}
	else
	{
		// GLX <= 1.2
		return glXMakeCurrent( global.connection, data->handle,
					core::get_internal( data->rc ).handle ) != 0;
	}
}

const glsk::config& glsk::window::get_rc() const
{
    return data->rc;
}

void glsk::window::swap_buffers()
{
	using namespace glsk::detail;

	if ( (global.flags & glx_fallback) == 0 )
	{
		// GLX >= 1.3
		glXSwapBuffers( global.connection, data->drawable );
	}
	else
	{
		// GLX <= 1.2
		glXSwapBuffers( global.connection, data->handle );
	}
}

void glsk::window::error_box( const std::string& text ) const
{
	throw failure( "function not implemented yet" );
}

#ifdef _GLSK_USE_SIGNALS_
/** Char signal.
	This is emitted whenever the window receives string input.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void, const std::string& >&
#else
boost::signal< void ( const std::string& ) >&
#endif
glsk::window::signal_char()
{ return data->signal_char; }

/** Redraw signal.
	This is emitted whenever the system requests for this window to be redrawn.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void >&
#else
boost::signal< void () >&
#endif
glsk::window::signal_redraw()
{ return data->signal_redraw; }

/** Configure signal.
	This is emitted whenever this window changes size.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void, int, int >&
#else
boost::signal< void ( int, int ) >&
#endif
glsk::window::signal_configure()
{ return data->signal_configure; }

/** Create signal.
	This is emitted right after the window is created.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void, int, int >&
#else
boost::signal< void ( int, int ) >&
#endif
glsk::window::signal_create()
{ return data->signal_create; }

/** Close signal.
	This is emitted when the user requests this window to be closed.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void >&
#else
boost::signal< void () >&
#endif
glsk::window::signal_close()
{ return data->signal_close; }

/** Destroy signal.
	This function is emitted when the window is destroyed.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void >&
#else
boost::signal< void () >&
#endif
glsk::window::signal_destroy()
{ return data->signal_destroy; }

/** Input event signal.
	This function is called whenever there is new event based input coming in.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void, const glsk::input_event& >&
#else
boost::signal< void ( const glsk::input_event& ) >&
#endif
glsk::window::signal_input()
{ return data->signal_input; }

/** Mouse event signal.
	This function is called whenever there is new event based input coming in.
	The parameters used are the x and y coordinates of the associated event, the type
	of the event (mouse_axis_move or mouse_button), the index of the button and the value of the button respectively.
*/
#ifdef _GLSK_USE_SIGNALS_
#  ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void, int, int, glsk::input_event::event_type, int, int >&
#  else
boost::signal< void (int, int, glsk::input_event::event_type, int, int) >&
#  endif
glsk::window::signal_mouse()
{ return data->signal_mouse; }
#endif

/** Activate signal.
*/
#ifdef _GLSK_USE_LIBSIGC_
sigc::signal< void, bool >&
#else
boost::signal< void ( bool ) >&
#endif
glsk::window::signal_activate()
{ return data->signal_activate; }

#endif


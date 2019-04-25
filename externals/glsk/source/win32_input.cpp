
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
**		filename:		win32_input.cpp
**
**		description:	input related functions
**		target-system:	microsoft windows
**		version-info:   $Id: win32_input.cpp 107 2010-01-16 16:27:26Z ltjax $
**
*/
#define INITGUID
#include <glsk/glsk.hpp>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <boost/utility.hpp>
#include "win32_input.hpp"
#include "win32_main.hpp"
#include "config.hpp"
#include "win32_window.hpp"
#include "win32_core.hpp"

namespace {

	enum {
		INPUT_BUFFER_SIZE = 128
	};

	// joystick device type
	struct joystick_type
	{
		joystick_type*			next;
		LPDIRECTINPUTDEVICE8	object;
		int						axis_count;
		int						button_count;
	};

	// mouse and keyboard device type
	struct device_type :
		public boost::noncopyable
	{
		
		LPDIRECTINPUTDEVICE8	object;
		DIDEVICEOBJECTDATA		buffer[ INPUT_BUFFER_SIZE ];
	};

	struct module :
		public boost::noncopyable
	{
		void					init();
		void					free();

		void					set_active( bool value );
		void					set_owner( glsk::window* o );
		void					capture_mouse( bool value );
		bool					is_mouse_captured() const;
		void					scan_joysticks();
		int						get_joystick_count() const { return joystick_count; }

		void					update();

		static module&			get();

	private:
		enum {
			is_enabled = 1 << 0,
			mouse_captured = 1 << 1
		};

		// Direct Input Stuff
		LPDIRECTINPUT8			direct_input;
		unsigned int			flags;

		device_type				keyboard;
		device_type				mouse;

		joystick_type*			joystick_list;
		int						joystick_count;

		glsk::window*			owner;

		void					update_mouse();
		void					update_keyboard();
		void					update_joysticks();

		void					free_joystick_list();
		
		static BOOL CALLBACK	scan_joystick_proc_router( const DIDEVICEINSTANCE* instance, VOID* context );
		bool					scan_joystick_proc( const DIDEVICEINSTANCE* instance );

								module();
	};

	// make sure the pointers have sane default values
	module::module()
	{
		this->direct_input = 0;
		this->flags = 0;

		this->mouse.object = 0;
		this->keyboard.object = 0;
		this->owner = 0;

		this->joystick_list = 0;
		this->joystick_count = 0;
	}

	// singleton get method
	module& module::get()
	{
		static module o;
		return o;
	}

	void module::init()
	{
		HRESULT result;

		// create a DirectInput interface
		DEBUG_PRINT0( " creating DirectInput interface.\n" );
		result = DirectInput8Create( glsk::detail::get_module_instance(),
			DIRECTINPUT_VERSION, IID_IDirectInput8,
			(void**)&(this->direct_input), NULL); 

		if ( FAILED( result ) )
			throw glsk::unsuccessful();

		// create a Keyboard device
		DEBUG_PRINT0( " adding keyboard..." );
		result = direct_input->CreateDevice(
			GUID_SysKeyboard, &keyboard.object, NULL );
		
		if ( FAILED( result ) )
			throw glsk::unsuccessful();

		// set the keyboard data format
		result = keyboard.object->SetDataFormat( &c_dfDIKeyboard ); 

		if ( FAILED( result ) )
			throw glsk::unsuccessful();

		DEBUG_PRINT0( "(ok)\n" );
		DEBUG_PRINT0( " adding mouse..." );

		// create a mouse device
		result = direct_input->CreateDevice(
			GUID_SysMouse, &mouse.object, NULL );

		if ( FAILED( result ) )
			throw glsk::unsuccessful();
	
		// set the mouse data format
		result = mouse.object->SetDataFormat( &c_dfDIMouse2 );

		if ( FAILED( result ) )
			throw glsk::unsuccessful();

		DEBUG_PRINT0( "(ok)\n" );


		this->scan_joysticks();


		// IMPORTANT STEP TO USE BUFFERED DEVICE DATA!
		//
		// DirectInput uses unbuffered I/O (buffer size = 0) by default.
		// If you want to read buffered data, you need to set a nonzero
		// buffer size.
		//
		// Set the buffer size to DINPUT_BUFFERSIZE (defined above) elements.
		//
		// The buffer size is a DWORD property associated with the device.
		DIPROPDWORD dipdw;

		dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj        = 0;
		dipdw.diph.dwHow        = DIPH_DEVICE;
		dipdw.dwData            = INPUT_BUFFER_SIZE;

		if( FAILED( keyboard.object->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
			throw glsk::unsuccessful();

		if( FAILED( mouse.object->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
			throw glsk::unsuccessful();
	}

	template < typename T > inline
	void safe_release( T*& x )
	{
		if ( x != 0 )
		{
			x->Release();
			x = 0;
		}
	}

	void module::free()
	{
		set_active( false );

		free_joystick_list();

		safe_release( keyboard.object );
		safe_release( mouse.object );
		safe_release( direct_input );
	}

	void module::set_active( bool value )
	{
		// check whether we're already in the right state
		if ( ((flags & is_enabled)!=0) == value )
			return;

		if ( value )
		{
			DEBUG_PRINT0( " aquiring devices.\n" );

			flags |= module::is_enabled;
			keyboard.object->Acquire();

			if ( flags & mouse_captured )
				mouse.object->Acquire();

			// aquire all joysticks
			for ( joystick_type* i = joystick_list; i; i = i->next )
				i->object->Acquire();

		}
		else
		{
			DEBUG_PRINT0( " unaquiring devices.\n" );

			flags &= ~is_enabled;
			keyboard.object->Unacquire();

			if ( flags & mouse_captured )
				mouse.object->Unacquire();

			for ( joystick_type* i = joystick_list; i; i = i->next )
				i->object->Unacquire();
		}
	}

	void module::set_owner( glsk::window* o )
	{
		// disable the input while switching
		set_active( false );

		try {

			HWND window = glsk::detail::core::get_handle( *o );

			// TODO: manage exclusive/non-exclusive mode somehow
			if ( FAILED( keyboard.object->SetCooperativeLevel( window, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND ) ) )
				throw glsk::unsuccessful();

			if ( FAILED( mouse.object->SetCooperativeLevel( window, DISCL_EXCLUSIVE | DISCL_FOREGROUND ) ) )
				throw glsk::unsuccessful();

			for ( joystick_type* i = joystick_list; i; i = i->next )
				if ( FAILED( i->object->SetCooperativeLevel( window, DISCL_EXCLUSIVE | DISCL_FOREGROUND ) ) )
					throw glsk::unsuccessful();

		}
		catch ( glsk::unsuccessful )
		{
			DEBUG_PRINT0( "glsk: unable to set cooperative level.\n" );
		}

		this->owner = o;

		set_active( true );
	}

	void module::update_mouse()
	{	
		using glsk::detail::core;
		using glsk::input_event;

		if ( owner == 0 || ( flags & mouse_captured ) == 0 )
			return;

		glsk::window&				window = *owner;
		LPDIRECTINPUTDEVICE8		mouse = this->mouse.object;
		DWORD						element_count = INPUT_BUFFER_SIZE;
		DIDEVICEOBJECTDATA*			data = this->mouse.buffer;
		HRESULT						result;

		result = mouse->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), data, &element_count, 0 );

		if ( result != DI_OK )
		{
			if ( result == DIERR_NOTACQUIRED )
			{
				mouse->Acquire();
				result = mouse->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), data, &element_count, 0 );
			}
			else
			{
				for ( unsigned int i = 0; i < 64; ++i )
				{
					if ( result == DIERR_INPUTLOST )
						result = mouse->Acquire();
				}

				if ( result != DI_OK )
				{

					DEBUG_PRINT0( "Error reading mouse.\n" );
					// TODO: handle errors
					return;
				}
			}
		}

		for ( unsigned int i = 0; i < element_count; ++i )
		{
			DIDEVICEOBJECTDATA* current = data + i;

			//TODO: improve on this
			if ( current->dwOfs == DIMOFS_BUTTON0 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 1, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_BUTTON1 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 2, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_BUTTON2 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 3, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_BUTTON3 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 4, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_BUTTON4 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 5, (current->dwData >> 7) & 1 ) );		
			else if ( current->dwOfs == DIMOFS_BUTTON5 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 6, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_BUTTON6 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 7, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_BUTTON7 )
				core::send_input_event( window, input_event( 0, glsk::mouse_button, 8, (current->dwData >> 7) & 1 ) );
			else if ( current->dwOfs == DIMOFS_X )
				core::send_input_event( window, input_event( 0, 
					glsk::mouse_move, 0, current->dwData ) );
			else if ( current->dwOfs == DIMOFS_Y )
				core::send_input_event( window, input_event( 0,
					glsk::mouse_move, 1, current->dwData ) );
			else if ( current->dwOfs == DIMOFS_Z )
				core::send_input_event( window, input_event( 0,
					glsk::mouse_move, 2, current->dwData ) );

			if ( (flags & is_enabled) == 0 )
				return;
		}
	}

	void module::update_keyboard()
	{
		using glsk::detail::core;
		using glsk::input_event;

		LPDIRECTINPUTDEVICE8		keyboard = this->keyboard.object;
		DWORD						element_count = INPUT_BUFFER_SIZE;
		DIDEVICEOBJECTDATA*			data = this->keyboard.buffer;
		HRESULT						result;

		if ( !owner || ( flags & is_enabled ) == 0 )
			return;

		result = keyboard->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), data, &element_count, 0 );

		if ( result != DI_OK )
		{
			if ( result == DIERR_NOTACQUIRED )
			{
				keyboard->Acquire();
				result = keyboard->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), data, &element_count, 0 );
			}
			else
			{
				for ( unsigned int i = 0; i < 64; ++i )
				{
					if ( result == DIERR_INPUTLOST )
						result = keyboard->Acquire();
				}

				if ( result != DI_OK )
				{

					DEBUG_PRINT0( "Error reading keyboard.\n" );
					// TODO: handle errors
					return;
				}
			}
		}

		for ( unsigned int i = 0; i < element_count; ++i )
		{
			core::send_input_event( *owner,
				input_event( 0, glsk::key, data[ i ].dwOfs, ( data[ i ].dwData >> 7 ) & 1 ) );

			if ( (flags & is_enabled) == 0 )
				return;
		}
	}

	void module::update_joysticks()
	{
		using glsk::detail::core;
		using glsk::input_event;

		HRESULT					result;
		DIDEVICEOBJECTDATA		buffer[ INPUT_BUFFER_SIZE ];
		DWORD					element_count = INPUT_BUFFER_SIZE;
		unsigned int			counter = 0;
		unsigned int			joystick_id = 0;

		for ( joystick_type* jst = joystick_list; jst; jst = jst->next, joystick_id++ )
		{
			element_count = INPUT_BUFFER_SIZE;

			result = jst->object->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), buffer, &element_count, 0 );

			while ( result != DI_OK )
			{
				if ( counter >= 64 )
				{
					DEBUG_PRINT0( "Error reading Joystick." );
					return;
				}

				if ( result == DIERR_NOTACQUIRED || result == DIERR_INPUTLOST )
				{
					jst->object->Acquire();
					++counter;
					continue;
				}
				else
				{
					DEBUG_PRINT0( "Error reading Joystick." );
					return;
				}

				result = jst->object->GetDeviceData( sizeof( DIDEVICEOBJECTDATA ), buffer, &element_count, 0 );
			}

			for ( unsigned int i = 0; i < element_count; ++i )
			{
				DIDEVICEOBJECTDATA& current = buffer[ i ];
				int DeviceObjectId = current.dwOfs / sizeof( DWORD );

				if ( DeviceObjectId < jst->axis_count )
				{
					core::send_input_event( *owner, input_event( joystick_id,
						glsk::joystick_axis_changed,
						DeviceObjectId, ( DeviceObjectId & 1 ) ? (1<<16)-1-current.dwData : current.dwData ) );
				}
				else
				{
					core::send_input_event( *owner, input_event( joystick_id,
						glsk::joystick_button,	DeviceObjectId - jst->axis_count,
						( current.dwData >> 7 )&1 ) );
				}

				if ( (flags & is_enabled) == 0 )
					return;
			}
		}
	}


	void module::update()
	{
		if ( ( flags & is_enabled ) == 0 )
			return;

		update_joysticks();
		update_mouse();
		update_keyboard();
	}

	void module::capture_mouse( bool value )
	{
		if ( value )
		{
			flags |= mouse_captured;
			ShowCursor( false );
		}
		else
		{
			flags &= ~mouse_captured;		
			ShowCursor( true );
		}

		if ( flags & is_enabled )
		{
			if ( value )
				mouse.object->Acquire();
			else
				mouse.object->Unacquire();
		}
	}

	inline bool module::is_mouse_captured() const
	{
		return ( flags & mouse_captured ) != 0;
	}

	void module::scan_joysticks()
	{
		DEBUG_PRINT0( " scanning attached joysticks:\n" );

		// clear previous joystick list
		free_joystick_list();

		// scan for joysticks
		if ( direct_input->EnumDevices( DI8DEVCLASS_GAMECTRL,
			&module::scan_joystick_proc_router, this, DIEDFL_ATTACHEDONLY ) != DI_OK )
		{
			DEBUG_PRINT0( " failed.\n" );
			return;
		}

		DEBUG_PRINT0( " done.\n" );
	}

	
	void module::free_joystick_list()
	{
		joystick_type* current = joystick_list;

		while ( current )
		{
			joystick_type* temp = current;
			
			if ( current->object )
				current->object->Release();

			current = current->next;
			delete temp;
		}

		joystick_list = 0;
		joystick_count = 0;
	}
	
	BOOL CALLBACK scan_joystick_axis_proc( const DIDEVICEOBJECTINSTANCE* doi, VOID* context )
	{
		LPDIOBJECTDATAFORMAT	current;
		DIDATAFORMAT*			format = (DIDATAFORMAT*)context;
		int						instance = DIDFT_GETINSTANCE(doi->dwType);
		int						index = format->dwNumObjs;

		// find the index to insert this axis (this is some kind of insertion sort)
		for ( int i = format->dwNumObjs-1; i >= 0 ; --i )
		{
			current = &(format->rgodf[ i ]);

			if ( instance < DIDFT_GETINSTANCE(current->dwType) )
			{
				format->rgodf[ i+1 ] = format->rgodf[ i ];
				index = i;
			}
			else
				break;
		}

		current = &(format->rgodf[ index ]);

		current->pguid = 0;
		current->dwOfs = 0;
		current->dwType = doi->dwType;
		current->dwFlags = 0;

		format->dwNumObjs++;
		
		return DIENUM_CONTINUE;
	}

	BOOL CALLBACK module::scan_joystick_proc_router( const DIDEVICEINSTANCE* instance, VOID* context )
	{
		return reinterpret_cast< module* >( context )->scan_joystick_proc( instance ) != 0;
	}

	bool module::scan_joystick_proc( const DIDEVICEINSTANCE* instance )
	{
		HRESULT					result;
		LPDIRECTINPUTDEVICE8	object = 0;
		DIDATAFORMAT			format;

		// Obtain an interface to the enumerated joystick.
		result = this->direct_input->CreateDevice( instance->guidInstance, &object, NULL);

		if( FAILED( result ) )
		{
			return DIENUM_CONTINUE;
		}

		// get this joysticks capabilities
		DIDEVCAPS caps;
		caps.dwSize = sizeof( DIDEVCAPS );

		object->GetCapabilities( &caps );


		// set the joysticks format
		format.dwSize = sizeof( DIDATAFORMAT );
		format.dwObjSize = sizeof( DIOBJECTDATAFORMAT );
		format.dwFlags = DIDF_ABSAXIS;
		format.dwDataSize = (caps.dwAxes + caps.dwButtons) * sizeof( DWORD );	// reserve a DWORD for each axis and each button
		format.dwNumObjs = /*axiscount +*/ 0;
		format.rgodf = new DIOBJECTDATAFORMAT[ caps.dwAxes + caps.dwButtons ];

		// FIXME: actually do something with the POVs
		DEBUG_PRINT3( "  adding joystick with %i axes, %i buttons and %i POVs...", caps.dwAxes, caps.dwButtons, caps.dwPOVs );

		// setup the axes
		object->EnumObjects( &scan_joystick_axis_proc, &format, DIDFT_AXIS );

		if ( caps.dwAxes != format.dwNumObjs )
		{
			DEBUG_PRINT0( "(error while scanning axes)\n" );
			delete[] format.rgodf;
			return DIENUM_CONTINUE;
		}

		// init the axis offsets
		for ( unsigned int i = 0; i < caps.dwAxes; ++i )
			format.rgodf[ i ].dwOfs = i * sizeof( DWORD );

		// setup the button data objects
		for ( unsigned int i = 0; i < caps.dwButtons; ++i )
		{
			LPDIOBJECTDATAFORMAT current = &(format.rgodf[ caps.dwAxes + i ]);
			current->pguid = 0;
			current->dwOfs = caps.dwAxes * sizeof( DWORD ) + i * sizeof( DWORD );
			current->dwType = DIDFT_BUTTON | DIDFT_MAKEINSTANCE(i);
			current->dwFlags = 0;
		}
		format.dwNumObjs += caps.dwButtons;

		result = object->SetDataFormat( &format );
		delete [] format.rgodf;

		if ( FAILED( result ) )
		{
			DEBUG_PRINT0( "(error while setting data format)\n" );
			IDirectInputDevice8_Release( object );
			return DIENUM_CONTINUE;
		}

		DIPROPDWORD dipdw;

		dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj        = 0;
		dipdw.diph.dwHow        = DIPH_DEVICE;
		dipdw.dwData            = INPUT_BUFFER_SIZE;

		if( FAILED( object->SetProperty( DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
			throw glsk::unsuccessful();

		joystick_type* jst = new joystick_type;
	    
		jst->object = object;
		jst->next = this->joystick_list;
		jst->axis_count = caps.dwAxes;
		jst->button_count = caps.dwButtons;

		joystick_list = jst;
		joystick_count++;

		DEBUG_PRINT0( "(ok)\n" );

		return DIENUM_CONTINUE;
	}
}

// initialize the input module
void glsk::detail::input::init()
{
	try {
		module::get().init();
	}
	catch ( glsk::unsuccessful )
	{
		DEBUG_PRINT0( " failed initialising input subsystem.\n" );
		glsk::detail::input::free();
		throw;
	}
}

// disable polling of input
void glsk::detail::input::enable()
{
	module::get().set_active( true );
}

// enable input polling
void glsk::detail::input::disable()
{
	module::get().set_active( false );
}

//	set the current owner window of the input
void glsk::detail::input::set_owner( glsk::window* o )
{
	module::get().set_owner( o );
}

// poll all active input devices and spread events
void glsk::detail::input::update()
{
	module::get().update();
}

void glsk::detail::input::free()
{
	module::get().free();
}

/** Sets the mouse's captured status.
	If the mouse is set captured, no cursor will be visible and only relative mouse events will be reported.
	\ingroup Input
*/

void glsk::set_mouse_captured( bool value )
{
	module::get().capture_mouse( value );
}

/** Gets the mouse's captured status.
	If the mouse is set captured, no cursor will be visible and only relative mouse events will be reported.
	\ingroup Input
*/

bool glsk::get_mouse_captured()
{
	return module::get().is_mouse_captured();
}

/** Rescan the computer for attached joysticks.
	\note there is no guarantee about the order of the joysticks after a scan.
	\ingroup Input
*/
void glsk::scan_joysticks()
{
	module::get().scan_joysticks();
}

/** Get the amount of joysticks currently attached.
	\ingroup Input
*/
int glsk::get_joystick_count()
{
	return module::get().get_joystick_count();
}




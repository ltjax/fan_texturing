
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
**    glsk - openGL Systems toolKit
**    for C++
**
**    version-info: $Id: glsk.hpp 113 2010-05-14 20:49:58Z ltjax $
**
*/

#ifndef GLSK_HEADER_INCLUDED
#define GLSK_HEADER_INCLUDED

#include <string>
#include <list>
#include <stdexcept>
#include <boost/utility.hpp>
#include <boost/tuple/tuple.hpp>

// figure out the platform
#if defined( _WIN32 )
#  define GLSK_PLATFORM_WIN32
#else
#  define GLSK_PLATFORM_LINUX
#endif

// define both if none is defined
#if !defined( GLSK_USE_SIGNALS ) && !defined( GLSK_USE_VIRTUAL_EVENTS )
#  define GLSK_USE_SIGNALS
#  define GLSK_USE_VIRTUAL_EVENTS
#endif

// get the right includes for optional signals
#ifdef GLSK_USE_SIGNALS
#  ifdef GLSK_USE_LIBSIGC
#    include <sigc++/signal.h>
#    include <sigc++/trackable.h>
#  else
#    include <boost/signal.hpp>
#    include <boost/signals/trackable.hpp>
#  endif
#endif

// setup auto-link for microsoft visual studio
#if defined(_MSC_VER)
#  ifndef GLSK_NO_AUTO_LINK
#    pragma comment( lib, "opengl32.lib" )
#    pragma comment( lib, "user32.lib" )
#    pragma comment( lib, "gdi32.lib" )
#    pragma comment( lib, "dinput8.lib" )
#  endif
#endif

/** the glsk namespace.
*/
namespace glsk
{
	/** Generic function pointer type.
		\ingroup Supportive
	*/
	typedef void (*proc)();

	/** TODO: this needs a more meaningful name.
	*/
	class object
	{
	public:
		object();
		object( const object& other );
		~object();

	private:
		static unsigned int& refcounter();
	};


    /**
        A convenience type containing three signed integers.
		\ingroup Supportive
    */
	typedef boost::tuple<int,int,int> int3;

	/**	Type of	an input event.
		\see get_type()
	*/
	enum input_event_type
	{
		/**	Mouse move.	*/
		mouse_move = 1,
		/**	Mouse button press/release.	*/
		mouse_button,
		/**	Keyboard button	press/release. */
		key,
		/**	Joystick button	press/release. */
		joystick_button,
		/**	Joystick axis change. */
		joystick_axis_changed
	};

	/**	An input event structure.
	\ingroup Input
	*/
	class input_event
	{
		public:

			int											get_device() const { return device; }
			input_event_type							get_type() const { return type; }
			int											get_identifier() const { return identifier; }
			int											get_value()	const { return value; }

														input_event( const int device, const input_event_type type,
															const int identifier, const int value )
														: device( device ), type( type ),
														identifier( identifier ), value( value ) {}

		private:
			int											device;
			input_event_type							type;
			int											identifier;
			int											value;
	};

	/** Mouse events.
	\ingroup Input
	*/
	class mouse_event
	{
		public:
			int											get_px() const { return px; }
			int											get_py() const { return py; }

			input_event_type							get_type() const { return type; }

			int											get_index() const { return index; }
			int											get_value() const { return value; }

			mouse_event( int px, int py, input_event_type type, int index, int value )
				: px(px), py(py), type(type), index(index), value(value) {}

		private:
			int											px;
			int											py;
			input_event_type							type;
			int											index;
			int											value;
	};

    class window;
    class pbuffer;

	namespace detail {
		class core;
	}

	/** Critical error that should usually result in program termination.
		glsk raises these if an internal error occured.
		\ingroup Exceptions
	*/
	struct failure :
		public std::runtime_error
	{
		/** create a new failure exception.
			\param text The text to be carried by this extension.
		*/
		explicit										failure( const std::string& text ) : std::runtime_error( text ) {}
	};

	/** non critical error that is risen when some call was for whatever reason unsuccessful.
		\ingroup Exceptions
	*/
	struct unsuccessful
	{
														unsuccessful() {}
	};

    /** An OpenGL rendering context.
        Represents a rendering context that takes OpenGL calls.
        Once this rendering context is selected for a drawable,
        all future OpenGL calls are directed to it.
		\note This is a reference type.
		\ingroup Main
    */
	class config :
		public object
    {
		private:
			struct detail;
			detail* data;

		public:
			enum flags
			{
				/** Support drawing to a window. */
				draw_to_window = 1,
				/** Support drawing to a pbuffer. */
				draw_to_pbuffer = 2,
				/** Support double buffering. */
				doublebuffer = 4,
				/** Support FSAA via multisampling. */
				multisample = 8,
				/** Indicate that the color bits are specified. */
				color_bits_set = 16,
				/** Indicate that the depth bits are specified. */
				depth_bits_set = 32,
				/** Indicate that the stencil bits are specified. */
				stencil_bits_set = 64
			};


														config( int flags, int color_bits, int depth_bits,
																int stencil_bits, int samples,
																int major_version, int minor_version, int core_profile  );
														config( const config& other );

														~config();

			config&										operator=( const config& other );

			int											get_extension_count() const;
			std::string									get_extension_name( int index ) const;
			proc										get_extension_proc( const char* name ) const;
			bool										is_extension_supported( const std::string& extension ) const;

			friend class glsk::window;
			friend class glsk::detail::core;
    };

    /** A drawable base type.
        This is a type that can be rendered to via OpenGL.
		\ingroup Main
    */
	class drawable :
#ifdef GLSK_USE_SIGNALS
#ifdef GLSK_USE_LIBSIGC
		public sigc::trackable,
#else
		public boost::signals::trackable,
#endif
#endif
		public boost::noncopyable,
		public object
    {
        protected:
            inline											drawable() {}
            virtual                                         ~drawable() {}
        public:

            virtual void									swap_buffers() = 0;
            virtual bool									select_rc() = 0;
    };

    /** A window with an opengl client area.
        This class represents window that can be used to draw into.
		\ingroup Main
    */
    class window :
		public drawable
    {
		public:
			struct detail;
		private:
			detail*           data;

		public:
			explicit /*ctor*/								window( const config& rc );
			virtual /*dtor*/								~window();

			void											open();
			void											destroy();
			bool											is_open();

			void											swap_buffers();
			bool											select_rc();
			const config&									get_rc() const;

			void											show();
			void											hide();
			bool											is_visible() const;

			// access functions
			const std::string&								get_title() const;
			int												get_width() const;
			int												get_height() const;
			boost::tuple<int,int>							get_size() const;
			boost::tuple<int,int>							get_position() const;
			bool											get_decorated() const;
			bool											get_fullscreen() const;

			void											set_title( const std::string& string );
			void											set_width( const int width );
			void											set_height( const int height );
			void											set_size( const int width, const int height );
			void											set_position( const int x, const int y );
			void											set_decorated( const bool value );
			void											set_fullscreen( const bool value );

#ifdef GLSK_USE_SIGNALS
#ifdef GLSK_USE_LIBSIGC
			sigc::signal< void, const std::string& >&		signal_char();
			sigc::signal< void >&							signal_redraw();
			sigc::signal< void, int, int >&					signal_configure();
			sigc::signal< void, int, int >&					signal_create();
			sigc::signal< void >&							signal_close();
			sigc::signal< void >&							signal_destroy();
			sigc::signal< void, const input_event& >&		signal_input();
			sigc::signal< void, const mouse_event& >&		signal_mouse();
			sigc::signal< void, bool >&						signal_activate();
#else
			boost::signal< void ( const std::string& ) >&	signal_char();
			boost::signal< void () >&						signal_redraw();
			boost::signal< void ( int, int ) >&				signal_configure();
			boost::signal< void ( int, int ) >&				signal_create();
			boost::signal< void () >&						signal_close();
			boost::signal< void () >&						signal_destroy();
			boost::signal< void ( const input_event& ) >&	signal_input();
			boost::signal< void ( const mouse_event& ) >&	signal_mouse();
			boost::signal< void ( bool ) >&					signal_activate();
#endif
#endif

#ifdef GLSK_USE_VIRTUAL_EVENTS
			virtual void									on_char( const std::string& x ) {}
			virtual void									on_redraw() {}
			virtual void									on_configure( int w, int h ) {}
			virtual void									on_create( int w, int h ) {}
			virtual void									on_close() {}
			virtual void									on_destroy() {}
			virtual void									on_input( const input_event& e ) {}
			virtual void									on_mouse( const mouse_event& e ) {}
			virtual void									on_activate( bool state ) {}
#endif

			void											error_box( const std::string& text ) const;

			friend struct glsk::window::detail;
			friend class glsk::detail::core;

    };

	/**	Pbuffers are offscreen drawables
		\note This is a	reference type.
		\ingroup Main
	*/
	class pbuffer :
		public drawable
	{
		private:
			struct detail;
			detail* data;
		public:
			explicit /*ctor*/								pbuffer( window& owner );
			/*ctor*/										pbuffer( window& owner, const config& rc );
			virtual											~pbuffer();

			void											open( int width, int height, bool get_largest );
			void											destroy();

			void											swap_buffers();
			bool											select_rc();
	};

	/**	Filesystem related functions.
	\ingroup Supportive
	*/
	namespace file_system
	{
		void											change_directory( const	std::string& path );
	}


	/**	The	program	main-loop.
	\ingroup Main
	*/
	namespace main
	{
		void									quit();
		bool									pump_messages();
	}

	/* misc functions
	*/
	void										scan_joysticks();
	int											get_joystick_count();

	void										error_box( const std::string& text );

	int											get_keycode( const std::string&	name );
	const char*									get_keyname( const int code	);

	void										set_resolution(	const int width, const int height, const int bpp );
	int											enumerate_resolutions( std::list<int3>& result, const int3& min );

	void										set_mouse_captured(	bool value );
	bool										get_mouse_captured();

	double										get_time();

	void										print( const char* text );
	void										print( const std::string& text );

#ifdef GLSK_PLATFORM_WIN32
	/** Interface specific to the Microsoft Windows implementation.
	*/
	namespace platform_win32
	{
		long									get_native_handle( glsk::window& wnd );
	};
#endif
}

#endif // GLSK_HEADER_INCLUDED


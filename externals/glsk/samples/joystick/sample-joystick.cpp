
#include <GL/gl.h>
#include <glsk/glsk.hpp>
#include <cmath>
#include <iostream>
#include <boost/bind.hpp>

// inherit our application from glsk::window
// note that this is not needed, since events are bound via signals, and not via virtual functions
// if desired, virtual function event handlers can easiely be implement on top of those signals
class application : public glsk::window
{
public:

	float pi;
	float Position[ 4 ];

	application( glsk::config& rc )
	: window( rc )
	{
		// set this window's title
		set_title( "GLSK Joystick Test Application" );

		set_size( 512, 512 );

		// enable window decorations
		set_decorated( true );

		// make this window visible
		show();

		pi = std::acos( 0.f ) * 2.f;

		Position[ 0 ] = Position[ 1 ] = Position[ 2 ] = Position[ 3 ] = 0.f;

		// open the window handlers
		try {
            open();
		}
		catch( glsk::unsuccessful )
		{
		    std::cout << "unable to open window.";
		}
	}

	void on_close()
	{
		glsk::main::quit();
	}

	
	void on_create( int w, int h )
	{
		// Make this window's OpenGL context active.
		glsk::window::select_rc();

		// Setup the initial viewport.
		on_configure( w, h );
	}

	// resize handler
	void on_configure( int w, int h )
	{
		glViewport( 0, 0, w, h );

		float p = static_cast< float >( w ) / h;

		// maintain the aspect ratio
		const float matrix[ 16 ] = {
		  1.f/p, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f };

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glLoadMatrixf( matrix );

		glMatrixMode( GL_MODELVIEW );

		glLineWidth( 3.f );
		glPointSize( 5.f );
		glClearColor( 1.f, 1.f, 1.f, 1.f );
	}

	// redraw our scene
	void on_redraw()
	{
		float delta = pi / 32.f;

		glClear( GL_COLOR_BUFFER_BIT );
		glLoadIdentity();

		glColor3ub( 0, 0, 0 );
		// draw a simple triangle using 3 colors
		glBegin( GL_LINE_LOOP );

		for ( float a = 0.f; a < 2.f*pi; a += delta )
		{
			glVertex2f( std::cos( a ), std::sin( a ) );
		}

		glEnd();

		glBegin( GL_POINTS );
			glColor3ub( 255, 0, 0 );
			glVertex2fv( Position );
			glColor3ub( 0, 255, 0 );
			glVertex2fv( Position + 2 );
		glEnd();

		this->swap_buffers();
	}

	// input event handler
	void on_input( const glsk::input_event& input )
	{
		// glsk uses strings to identify keycodes, which makes it portable and extensible
		if ( input.get_type() == glsk::key && input.get_value() && 
			input.get_identifier() == glsk::get_keycode( "Escape" ) )
			glsk::main::quit();

		if ( input.get_type() == glsk::joystick_button && input.get_value() && 
			input.get_identifier() == 0 )
			glsk::main::quit();

		if ( input.get_type() == glsk::joystick_axis_changed )
		{
			float Offset = float( 1 << 15 );

			if ( 0 <= input.get_identifier() && input.get_identifier() < 4 )
				Position[ input.get_identifier() ] = (input.get_value() - Offset)/Offset;
		}
	}
};

#undef APIENTRY
#undef WINGDIAPI
#include <windows.h>

// glsk uses a unified program entry point
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
    try {
        // select our window's config
        glsk::config cfg( glsk::config::draw_to_window | glsk::config::doublebuffer |
			glsk::config::color_bits_set | glsk::config::depth_bits_set |
			glsk::config::multisample, 32, 24, 0, 4, 0, 0, 0 );

		std::cout << "found " << glsk::get_joystick_count() << " joysticks." << std::endl;

        // create our window object using that config
        application wnd( cfg );

        // run the application
		while ( glsk::main::pump_messages() )
			wnd.on_redraw();

        return 0;
    }
    catch( glsk::failure& x )
    {
        std::cout << "glsk critical exception caught: " << x.what() << std::endl;
    }

    return 0;
}

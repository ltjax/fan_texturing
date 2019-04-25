#include <glsk/glsk.hpp>

#ifdef MSVC_OPENGL_HEADER
#  include <windows.h>
#endif



#include <GL/gl.h>

#include <cmath>
#include <iostream>
#include <boost/bind.hpp>


template < int l, int r >
int clamp( int v ) { if ( v < l ) return l; else if ( v > r ) return r; else return v; }



// inherit our application from glsk::window
// note that this is not needed, since events are bound via signals, and not via virtual functions
// if desired, virtual function event handlers can easiely be implement on top of those signals
class application :
	public glsk::window
{
public:

	float last_update;
	float angle;

	application( glsk::config& rc )
	: window( rc ), angle( 0.f )
	{
		// set this window's title
		set_title( "GLSK Test Application" );

		// enable window decorations
		set_decorated( true );

		// make this window visible
		show();

		// put mouse in captured mode - that means relative device coordinates and no visible cursor
		//glsk::set_mouse_captured( true );

		// connect the update and redraw methods to the idle signal, so they are called whenever we
		// don't need to handle any messages
		//glsk::main::signal_idle().connect( boost::bind( &application::update, this ) );
		//glsk::main::signal_idle().connect( boost::bind( &application::redraw, this ) );
		//glsk::main::set_idle_callback( &application::idle_handler, this );

		// connect all the necesarry handlers
		/*this->signal_create().connect( boost::bind( &application::on_create, this, _1, _2 ) );
		this->signal_configure().connect( boost::bind( &application::on_configure, this, _1, _2 ) );
		this->signal_redraw().connect( boost::bind( &application::redraw, this ) );
		this->signal_input().connect( boost::bind( &application::on_input, this, _1 ) );
		this->signal_close().connect( &glsk::main::quit );
*/


        open();


	}

	// initialization handler
	void on_create( int x, int y )
	{
		last_update = static_cast< float >( glsk::get_time() );

		select_rc();

		glClearColor( 1.f, 1.f, 1.f, 1.f );
		glPointSize( 4.f );
		on_configure( x, y );
	}

	// resize handler
	void on_configure( int w, int h )
	{
		select_rc();


		glViewport( 0, 0, w, h );

		const float fov = 75.f;
		const float near = 0.1f;
		const float far = 10000.f;
		float f = 1.f / std::tan( fov / 180.f * 1.570796327f );
		float a = near + far;
		float b = near * far * 2.f;
		float d = near - far;
		float p = static_cast< float >( w ) / h;

		// setup a perspective matrix
		const float matrix[ 16 ] = {
			f/p, 0.f, 0.f, 0.f,
			0.f,   f, 0.f, 0.f,
			0.f, 0.f, a/d,-1.f,
			0.f, 0.f, b/d, 0.f };

		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glLoadMatrixf( matrix );

		glMatrixMode( GL_MODELVIEW );
	}

	// update our scene
	void update()
	{
		float time = static_cast< float >( glsk::get_time() );

		// 1/3 rotation per second
		angle += std::fmod( 120.f * ( time - last_update ), 360.f );

		last_update = time;
	}

	// redraw our scene
	void on_redraw()
	{
		float radius = 5.f;
		float depth = -10.f;
		float sy = 0.5f * radius;
		float sx = sqrt( 3.f ) / 2.f * radius;

		glClear( GL_COLOR_BUFFER_BIT );
		glLoadIdentity();
		glRotatef( angle, 0.f, 0.f, 1.f );

		// draw a simple triangle using 3 colors
		glBegin( GL_TRIANGLES );
			glColor3ub( 255, 0, 0 );
			glVertex3f( 0.f, radius, depth );
			glColor3ub( 0, 255, 0 );
			glVertex3f( -sx, -sy, depth );
			glColor3ub( 0, 0, 255 );
			glVertex3f( sx, -sy, depth );
		glEnd();
		glLoadIdentity();


		// swap buffers
		swap_buffers();
	}

	// input event handler
	void on_input( const glsk::input_event& input )
	{
		// glsk uses strings to identify keycodes, which makes it portable and extensible
		if ( input.get_type() == glsk::key && input.get_value() &&
			input.get_identifier() == glsk::get_keycode( "Escape" ) )
			glsk::main::quit();
	}

	void on_close()
	{
		glsk::main::quit();
	}
};

#ifdef _WIN32
// undefine tags previously defined for header compatibilty
#undef APIENTRY
#undef WINGDIAPI

// include a lean and mean version of the windows header
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain( HINSTANCE hinstance, HINSTANCE hlastinstance, PSTR cmdline, int iCmdShow )
#else
int main( int argc, char **argv )
#endif
{
    try {
        // select our window's config
        glsk::config cfg( glsk::config::draw_to_window | glsk::config::doublebuffer |
            glsk::config::color_bits_set | glsk::config::depth_bits_set, 32, 24, 0, 0, 0, 0, 0 );
		
		// open the window handlers

		// create our window object using that config
		application wnd( cfg );

		while ( glsk::main::pump_messages() )
		{
			wnd.update();
			wnd.on_redraw();
		}

		// run the application
		return 0;
    }
	catch( glsk::unsuccessful )
	{
	    std::cout << "unable to run app." << std::endl;
	}
    catch( glsk::failure& x )
    {
        std::cout << "glsk critical exception caught: " << x.what() << std::endl;
    }

    return 1;
}



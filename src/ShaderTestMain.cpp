

#include "RenderWindow.hpp"
#include <glsk/glsk.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <replay/vector_math.hpp>
#include <replay/affinity.hpp>
#include <replay/byte_color.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign.hpp>
#include <boost/filesystem/fstream.hpp>
#include <replay/buffer.hpp>
#include <replay/bstream.hpp>
#include "GLmm/Program.hpp"
#include "GLmm/GL.hpp"
#include "ShaderTestKernel.hpp"
								

#include <boost/filesystem/operations.hpp>

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
	boost::filesystem::initial_path();


    try {
        // select our window's config
        glsk::config cfg( glsk::config::draw_to_window | glsk::config::doublebuffer |
			glsk::config::color_bits_set | glsk::config::depth_bits_set | glsk::config::multisample, 32, 24, 0, 8,
			0,2,0);
		
		// open the window handlers

		// create our window object using that config
		CRenderWindow<CShaderTestKernel> RenderWindow( cfg );

		RenderWindow.Run( "Shader Test" );

		// run the application
		return 0;
    }
	catch( glsk::unsuccessful )
	{
		// Just ignore this, should be handled earlier
	}
	catch( std::runtime_error& x )
	{
		glsk::error_box( x.what() );
	}

    return 1;
}

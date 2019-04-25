

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
#include <boost/program_options.hpp>
#include <replay/buffer.hpp>
#include <replay/bstream.hpp>
#include "GLmm/Program.hpp"
#include "GLmm/GL.hpp"
#include "ApplicationKernel.hpp"
								

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
	
	std::cout << "Parsing options..." << std::endl;
	boost::filesystem::initial_path();


	if ( AttachConsole( ATTACH_PARENT_PROCESS ) == 0 )
		AllocConsole();

	namespace po = boost::program_options;
	std::string ModelName;

	// Parse the command line parameters
	{
		po::options_description Desc("Allowed options");
		Desc.add_options()
			("model", po::value<std::string>(&ModelName),
				"the model to load" )
		;
		po::variables_map VariableMap;

#ifdef GLSK_PLATFORM_WIN32
		// Parsing on Win32 is a little more complicated, since we first have to split the args up
		std::vector<std::string> Args = po::split_winmain(cmdline);
		po::store(po::command_line_parser(Args).options(Desc).run(), VariableMap);
#elif GLSK_PLATFORM_LINUX
		po::store(po::parse_command_line( Argc, Argv, Desc), VariableMap );
#else
#error Platform not supported.
#endif

		// process the program options
		po::notify(VariableMap);
	}

	freopen( "CONOUT$", "w", stdout );

	std::cout << "Initializing window..." << std::endl;

    try {
        // select our window's config
        glsk::config cfg( glsk::config::draw_to_window | glsk::config::doublebuffer |
			glsk::config::color_bits_set | glsk::config::depth_bits_set | glsk::config::multisample, 32, 24, 0, 8,
			3,2,0);
		
		// open the window handlers

		// create our window object using that config
		CRenderWindow<CApplicationKernel> RenderWindow( cfg, boost::filesystem::path(ModelName) );
		RenderWindow.set_size(1280,1024);

		RenderWindow.Run( "Fan Texturing Demo" );

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

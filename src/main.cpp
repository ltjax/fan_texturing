
#include "RenderWindow.hpp"
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


// figure out the platform
#if defined( _WIN32 )
#  define PLATFORM_WIN32
#else
#  define PLATFORM_LINUX
#endif

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

#ifdef PLATFORM_WIN32
		// Parsing on Win32 is a little more complicated, since we first have to split the args up
		std::vector<std::string> Args = po::split_winmain(cmdline);
		po::store(po::command_line_parser(Args).options(Desc).run(), VariableMap);
#elif PLATFORM_LINUX
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
        CRenderWindow<CApplicationKernel> RenderWindow{ boost::filesystem::path(ModelName) };

        // run the application
        RenderWindow.Run(1600, 900, "Fan Texturing Demo");

    }
	catch( std::runtime_error& x )
	{
        return 1;
	}
    return 0;

}

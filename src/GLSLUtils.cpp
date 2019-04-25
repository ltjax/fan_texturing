#include "GLSLUtils.hpp"

#include <sstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include "GLmm/GL.hpp"

using namespace GLmm;

/** Macro to easiely throw errors.
*/
#define THROW_ERROR( ARGS ) \
	do { \
	std::ostringstream _ErrorStr; \
	_ErrorStr << ARGS; \
	throw std::runtime_error( _ErrorStr.str() ); \
	} while ( false )


namespace {
void LoadComboShaderSourceInternal( const boost::filesystem::path& Filename,
						    std::vector<boost::tuple<uint,std::string>>& Sections,
							const std::vector<std::string>* Defines )
{
	namespace ba = boost::algorithm;
	
	boost::filesystem::ifstream File( Filename, std::ios::in | std::ios::binary );

	if ( !File.good() )
		THROW_ERROR( "Unable to open file " << Filename.string() );

	std::string DefineString("");

	if ( Defines )
	{
		BOOST_FOREACH( const std::string& x, *Defines )
		{
			DefineString += "#define " + x + '\n';
		}
	}

	boost::tuple<uint,std::string>* Section=0;
	std::string CurrentLine;
	unsigned int Counter = 0;

	// Scan all lines
	while ( std::getline( File, CurrentLine ) )
	{
		// Count and cleanup the line!
		++Counter;
		ba::trim_right_if( CurrentLine, ba::is_cntrl() );

		// Scan for tags
		if ( CurrentLine == "[Vertex Shader]" )
		{
			Sections.push_back( boost::make_tuple( uint(GL_VERTEX_SHADER), std::string() ) );
			Section = &Sections.back();
		}
		else if ( CurrentLine == "[Fragment Shader]" )
		{
			Sections.push_back( boost::make_tuple( uint(GL_FRAGMENT_SHADER), std::string() ) );
			Section = &Sections.back();
		}
		else if ( CurrentLine == "[Geometry Shader]" )
		{
			Sections.push_back( boost::make_tuple( uint(GL_GEOMETRY_SHADER_EXT), std::string() ) );
			Section = &Sections.back();
		}
		// Do we have an active section?
		else if ( Section != 0 )
		{
			std::string& Result = Section->get<1>();

			if ( Result.empty() || Result.length() == 0 )
			{
				if ( ba::starts_with( CurrentLine, "#version" ) )
					CurrentLine += '\n' + DefineString; // append - version needs to be first
				else
					CurrentLine = DefineString+CurrentLine; // prepend otherwise
			}

			Result += CurrentLine;
			Result += '\n';
		}
	}
}

}

void CompileShader( const boost::filesystem::path& Filename, GLmm::Program& Result )
{
	typedef boost::tuple<uint,std::string> ShaderTag;
	std::vector<ShaderTag> ShaderSections;

	LoadComboShaderSource( Filename, ShaderSections );

	// Compile all the translation units
	try {

		BOOST_FOREACH( const ShaderTag& x, ShaderSections )
		{
			GLmm::Shader Temp( x.get<0>() );
			Temp.SetSource( x.get<1>() );
			Temp.Compile();
			Result.Attach( Temp );
		}

	}
	catch ( GLmm::Shader::CompileError& x )
	{
		std::string Msg = std::string("Compile error in file ") + Filename.string() + ':' + x.what();
		throw std::runtime_error( Msg );
	}
}

void CompileShader( const boost::filesystem::path& Filename,
	const std::vector<std::string>& Defines, GLmm::Program& Result )
{
	typedef boost::tuple<uint,std::string> ShaderTag;
	std::vector<ShaderTag> ShaderSections;

	LoadComboShaderSourceInternal( Filename, ShaderSections, &Defines );

	// Compile all the translation units
	try {

		BOOST_FOREACH( const ShaderTag& x, ShaderSections )
		{
			GLmm::Shader Temp( x.get<0>() );
			Temp.SetSource( x.get<1>() );
			Temp.Compile();
			Result.Attach( Temp );
		}

	}
	catch ( GLmm::Shader::CompileError& x )
	{
		std::string Msg = std::string("Compile error in file ") + Filename.string() + ':' + x.what();
		throw std::runtime_error( Msg );
	}
}

void LoadComboShaderSource( const boost::filesystem::path& Filename,
						    std::vector<boost::tuple<uint,std::string>>& Sections )
{
	LoadComboShaderSourceInternal( Filename, Sections, 0 );
}

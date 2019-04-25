/*  Cataclysm-Software Disaster Engine

	$Id: Shader.cpp 595 2008-05-22 16:41:18Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#include "Shader.hpp"
#include <fstream>
#include <boost/filesystem/fstream.hpp>
#include <boost/scoped_array.hpp>
#include "GL.hpp"

using namespace GLmm;

GLmm::Shader::Shader()
: GLObject(0)
{
}

GLmm::Shader::Shader( GLuint Type )
: GLObject(0)
{
	Create( Type );
}

GLmm::Shader::~Shader()
{
	glDeleteShader( GLObject );
}

void GLmm::Shader::Create( GLuint Type )
{
	GLObject = glCreateShader( Type );
	GLMM_CHECK();

	if ( !GLObject )
		GLMM_THROW_ERROR( "Unable to create shader object (type:" << Type << ")" );
}

void GLmm::Shader::SetSource( const String& Source )
{
	const int Length = (const int)Source.length();
	const char* String[ 1 ] = { Source.data() };

	glShaderSource( GLObject, 1, String, &Length );
}

bool GLmm::Shader::LoadSourceFromFile( const Path& Filename )
{
	boost::filesystem::ifstream File( Filename, std::ios::binary );

	if ( !File.good() )
		return false;

	File.seekg( 0, std::ios::end );
	GLint Length = File.tellg();
	File.seekg( 0, std::ios::beg );

	boost::scoped_array< char > String( new char[ Length + 1 ] );

	File.read( String.get(), Length );
	String[ Length ] = 0; // null terminate it

	const char* Temp[ 1 ] = { String.get() };

	glShaderSource( GLObject, 1, Temp, &Length );

	return true;
}

void GLmm::Shader::Compile()
{
	int Status = 0;

	glCompileShader( GLObject );
	glGetShaderiv( GLObject, GL_COMPILE_STATUS, &Status );

	// Have there been errors?
	if ( !Status )
	{
		// Get the length of the compile log
		int InfoLength = 0;
		glGetShaderiv( GLObject, GL_INFO_LOG_LENGTH, &InfoLength );

		// Get memory and download the log
		boost::scoped_array< char > Buffer( new char[ InfoLength + 1 ] );
		glGetShaderInfoLog( GLObject, GLsizei( InfoLength + 1 ), 0, Buffer.get() );

		// Throw it!
		throw CompileError( Buffer.get() );
	}
}


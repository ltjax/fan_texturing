/*  Cataclysm-Software Disaster Engine

	$Id: Program.cpp 608 2008-06-04 16:09:32Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/


#include <boost/scoped_array.hpp>
#include "Program.hpp"
#include "GL.hpp"

using namespace GLmm;

GLmm::Program::Program()
: GLObject( glCreateProgram() )
{
    if ( !GLObject )
		GLMM_THROW_ERROR( "Unable to create program" );
}

GLmm::Program::~Program()
{
	/*GLint ShaderCount = 0;
	glGetProgramiv( GLObject, GL_ATTACHED_SHADERS, &ShaderCount );

	boost::scoped_array< GLuint > Shaders( new GLuint[ ShaderCount ] );

	glGetAttachedShaders( GLObject, ShaderCount, 0, Shaders.get() );

	for ( GLint i = 0; i < ShaderCount; ++i )
		glDetachShader( GLObject, Shaders[ i ] );
	_GLMM_CHECK_ERRORS();
*/
	glDeleteProgram( GLObject );
}

void GLmm::Program::Link()
{
	GLint Status = 0;

	glLinkProgram( GLObject );	GLMM_CHECK();
	glGetProgramiv( GLObject, GL_LINK_STATUS, &Status ); GLMM_CHECK();

	// check for eventual link errors.
	if ( !Status )
	{
		GLint InfoLength = 0;
		glGetProgramiv( GLObject, GL_INFO_LOG_LENGTH, &InfoLength ); GLMM_CHECK();

		boost::scoped_array< char > Buffer( new char[ InfoLength + 1 ] );
		glGetProgramInfoLog( GLObject, GLsizei( InfoLength + 1 ), 0, Buffer.get() );
		GLMM_CHECK();

		throw LinkError( Buffer.get() );
	}
}

int GLmm::Program::GetUniformLocation( const char* Name ) const
{
	return glGetUniformLocation( GLObject, Name );
}

int GLmm::Program::GetAttribLocation( const char* Name ) const
{
	return glGetAttribLocation( GLObject, Name );
}

void GLmm::Program::Use() const
{
	glUseProgram( GLObject ); GLMM_CHECK();
}

void GLmm::Program::Disable()
{
	glUseProgram( 0 ); GLMM_CHECK();
}

void GLmm::Program::Attach( const Shader& Rhs )
{
	glAttachShader( GLObject, Rhs.GetGLObject() ); GLMM_CHECK();
}



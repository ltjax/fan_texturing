/*  Cataclysm-Software Disaster Engine

	$Id: Object.cpp 595 2008-05-22 16:41:18Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#include <sstream>
#include <stdexcept>
#include "GL.hpp"


namespace {
/** Translate the error code into a human readable string.
*/
const char* GetErrorString( unsigned int Code )
{
	switch ( Code )
	{
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		default: return "UNKNOWN GL ERRORCODE";
	}
}
}

void GLmm::CheckGLErrorsAt( const char* File, const unsigned int Line )
{
	const unsigned int MaxErrorCodes = 8;
	GLenum ErrorCode = glGetError();

	if ( ErrorCode != GL_NO_ERROR )
	{
		std::ostringstream Str;
		GLenum LastError = ErrorCode;
		Str << '(' << File << ':' << Line << ')' << "OpenGL error: " << GetErrorString( ErrorCode );

		// the counter is needed or else this could potentially not terminate
		for ( int i = 0; i < MaxErrorCodes ; ++i )
		{
			ErrorCode = glGetError();
			
			if ( ErrorCode == GL_NO_ERROR )
				break;

			if ( ErrorCode == LastError )
				break;

			Str << "," << GetErrorString( ErrorCode );
			LastError = ErrorCode;
		}

		throw std::runtime_error( Str.str() );
	}
}


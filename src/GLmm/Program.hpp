/*  Cataclysm-Software Disaster Engine

	$Id: Program.hpp 684 2008-09-06 23:00:06Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#ifndef _GLMM_PROGRAM_HPP_
#define _GLMM_PROGRAM_HPP_

#include "Shader.hpp"

namespace GLmm {

/** An executable for the programmable GL pipeline.
*/
class Program :
	public boost::noncopyable,
	public GLmm::Object
{
public:

	/** Error that is thrown when Program::Link() fails.
	*/
	struct LinkError :
		public std::runtime_error
	{
		LinkError( const String& Message ) :
			std::runtime_error( Message )
		{
		}
	};

										Program();
										~Program();

	void								Attach( const Shader& Rhs );

	int									GetUniformLocation( const char* Name ) const;
	int									GetAttribLocation( const char* Name ) const;

	void								Link();
	
	void								Use() const;
	static void							Disable();

	GLuint								GetGLObject() const { return GLObject; }

	void								swap( Program& Rhs ) { std::swap(Rhs.GLObject,GLObject); }

private:
	GLuint								GLObject;
};

inline void swap( GLmm::Program& Lhs, GLmm::Program& Rhs )
{
	Lhs.swap( Rhs );
}

}

#endif

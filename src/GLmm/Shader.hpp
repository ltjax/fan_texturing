/*  Cataclysm-Software Disaster Engine

	$Id: Shader.hpp 595 2008-05-22 16:41:18Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#ifndef _GLMM_SHADER_HPP_
#define _GLMM_SHADER_HPP_

#include <iostream>
#include <stdexcept>
#include <boost/utility.hpp>
#include <boost/filesystem/path.hpp>
#include "Object.hpp"

namespace GLmm {

typedef std::string String;
typedef boost::filesystem::path Path;

/** A GL programmable pipeline compilation unit.
*/
class Shader :
	public boost::noncopyable,
	public GLmm::Object
{
public:

	/** Error that is thrown when Shader::Compile() fails.
	*/
	struct CompileError :
		public std::runtime_error
	{
		CompileError( const String& Message ) :
			std::runtime_error( Message )
		{
		}
	};

							Shader();
	explicit				Shader( GLenum Type );
							~Shader();

	void					Create( GLenum Type );

	void					SetSource( const String& Source );
	bool					LoadSourceFromFile( const Path& Filename );

	/** Compile the shader.
		Throws CompileError on a compile-time error.
	*/
	void					Compile();

	GLuint					GetGLObject() const { return GLObject; }

private:
	GLuint					GLObject;
};

}

#endif


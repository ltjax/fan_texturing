/*  Cataclysm-Software Disaster Engine

	$Id: BufferObject.cpp 729 2008-11-13 14:16:28Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/


#include <boost/scoped_array.hpp>
#include "BufferObject.hpp"

#include "GL.hpp"
using namespace GLmm;

GLmm::BufferObject::BufferObject()
{
	glGenBuffers( 1, &GLObject );
}

GLmm::BufferObject::~BufferObject()
{
	glDeleteBuffers( 1, &GLObject );
}

void GLmm::BufferObject::SetData(
		GLenum Target, std::size_t Size,
		const GLubyte* Data, GLenum Usage )
{
	glBindBuffer( Target, GLObject );
	glBufferData( Target, Size, Data, Usage );
	GLMM_CHECK();
}

void GLmm::BufferObject::SetSubData( GLenum Target, std::size_t Offset,
									std::size_t Size, const GLubyte* Data )
{
	glBindBuffer( Target, GLObject );
	glBufferSubData( Target, Offset, Size, Data );
	GLMM_CHECK();
}

void GLmm::BufferObject::GetSubData( GLenum Target, std::size_t Offset,
								std::size_t Size, GLvoid* Data )
{
	glBindBuffer( Target, GLObject );
	glGetBufferSubData( Target, Offset, Size, Data );
}

GLubyte* GLmm::BufferObject::Map( GLenum Target, GLenum Access )
{
	glBindBuffer( Target, GLObject );
	return reinterpret_cast< GLubyte* >( glMapBuffer( Target, Access ) );
}

bool GLmm::BufferObject::Unmap( GLenum Target )
{
	glBindBuffer( Target, GLObject );
	return glUnmapBuffer( Target ) != 0;
}


void GLmm::BufferObject::Bind( GLenum Target ) const
{
	glBindBuffer( Target, GLObject );
}

void GLmm::BufferObject::Unbind( GLenum Target )
{
	glBindBuffer( Target, 0 );
}

void GLmm::BufferObject::Swap( BufferObject& Rhs )
{
	std::swap( this->GLObject, Rhs.GLObject );
}


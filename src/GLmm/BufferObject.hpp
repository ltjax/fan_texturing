/*  Cataclysm-Software Disaster Engine

	$Id: BufferObject.hpp 729 2008-11-13 14:16:28Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#ifndef GLMM_BUFFER_OBJECT_HPP
#define GLMM_BUFFER_OBJECT_HPP

#include <vector>
#include <boost/scoped_array.hpp>
#include "Shader.hpp"

namespace GLmm {

/** An executable for the programmable GL pipeline.
*/
class BufferObject :
	public boost::noncopyable,
	public GLmm::Object
{
public:

							BufferObject();
							~BufferObject();

	/** Transfer the data to the buffer object.
		Implies a bind.
		\param Target the buffer target type this is to be bound too
		\param Size The number of elements of the the specified data type.
		\param Data Pointer to an array of elements.
		\param Usage Which policy to use for storing.
	*/
	void					SetData( GLenum Target, std::size_t Size,
									 const GLubyte* Data, GLenum Usage );

	/** Transfer the data to the buffer object.
		Implies a bind.
		\param Target the buffer target type this is to be bound too
		\param Data A vector of elements.
		\param Usage Which policy to use for storing.
	*/
	template < class T >
	void					SetData( GLenum Target, const std::vector<T>& Data, GLenum Usage );


	/** Transfer the data to the buffer object from a generator function.
		This is a convenience function that internally tries to use Map to
		avoid unnecessary data copies. However, if the map or the unmap fail,
		it will use a temporary copy for to update the data.
		\param Target the buffer target type this is to be bound too
		\param Size The number of elements of the the specified data type.
		\param Generator Function object to generate the data. This is called at most two times.
		\param Usage Which policy to use for storing.
	*/
	template < class T >
	void					SetGeneratedData( GLenum Target, std::size_t Size,
								T Generator, GLenum Usage );

	/** Transfer a part of the data to the buffer object.
		Does not reallocate.
	*/
	void					SetSubData( GLenum Target, std::size_t Offset,
								std::size_t Size, const GLubyte* Data );

	/** Transfer a part or all of the data in the buffer to the given memory.
	*/
	void					GetSubData( GLenum Target, std::size_t Offset,
								std::size_t Size, GLvoid* Data );

	/** Transfer generated data to a section of an already existing buffer.
		This is a convenience function that internally tries to use Map to 
		avoid unnecessary data copies.
		It does not reallocate.
	*/
	template < class T >
	void					SetGeneratedSubData( GLenum Target,	std::size_t Offset,
								std::size_t Size, T Generator );

	/** Map the buffer into local address space.
	*/
	GLubyte*				Map( GLenum Target, GLenum Access );

	bool					Unmap( GLenum Target );
	void					Bind( GLenum Target ) const;

	static void				Unbind( GLenum Target );

	void					Swap( BufferObject& Rhs );


private:
	
	GLuint					GLObject;
};

}

template <class T> inline
void GLmm::BufferObject::SetData( GLenum Target, const std::vector<T>& Data, GLenum Usage )
{
	SetData( Target, Data.size()*sizeof(T), reinterpret_cast<const GLubyte*>(&Data[0]), Usage );
}

template <class T> inline
void GLmm::BufferObject::SetGeneratedSubData( GLenum Target, std::size_t Offset,
											 std::size_t Size, T Generator )
{
	// Attempt to write to the mapped buffer at offset
	GLubyte* Data = Map(Target,GL_WRITE_ONLY);

	if ( Data )
		Generator( (Data + Offset) );

	// Use SetSubData as a fallback (and invoke the generator again)
	if ( !Data || !Unmap(Target) )
	{	
		boost::scoped_array<GLubyte> Temp(new GLubyte[Size]);
		Generator(Temp.get());

		SetSubData( Target, Offset, Size, Temp.get() );
	}
}

template <class T> inline
void GLmm::BufferObject::SetGeneratedData( GLenum Target, std::size_t Size,
								  T Generator,  GLenum Usage )
{
	// Init a buffer of the given size
	SetData(Target,std::max<std::size_t>(Size,1<<11),0,Usage);

	SetGeneratedSubData( Target, 0, Size, Generator );
}


#endif

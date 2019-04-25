/*  Cataclysm-Software Disaster Engine

	$Id: Framebuffer.hpp 596 2008-05-22 21:02:14Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#ifndef GLMM_FRAMEBUFFER_HPP
#define GLMM_FRAMEBUFFER_HPP

#include <boost/utility.hpp>
#include "Object.hpp"
#include "Texture.hpp"

namespace GLmm {

/** A possible framebuffer component.
*/
class Renderbuffer :
	public boost::noncopyable,
	public GLmm::Object	
{
public:
	void				SetStorage( GLenum InternalFormat, GLsizei w, GLsizei h );

						Renderbuffer();
						~Renderbuffer();

private:
	GLuint				GLObject;

	friend class Framebuffer;
};

/** Additional rendering target.
*/
class Framebuffer :
	public boost::noncopyable,
	public GLmm::Object
{
public:

						Framebuffer();
						~Framebuffer();

	void				Bind() const;
	static void			Unbind();

	void				TestCompleteness();

    void				AttachColorbuffer( GLuint Index, const Texture2D& Texture, GLint Level = 0 );
	void				AttachColorbuffer( GLuint Index, const TextureRect& Texture );
	void				DetachColorbuffer( GLuint Index );

	void				Attach( GLenum Attachment, const Texture2DArray& Texture, GLint Layer, GLint Level=0 );
	void				Attach( GLenum Attachment, const Texture2D& Texture, GLint Level = 0 );
	void				Attach( GLenum Attachment, const Renderbuffer& Renderbuffer );
	void				Detach( GLenum Attachment );

private:
	GLuint				GLObject;

};

}

#endif //GLMM_FRAMEBUFFER_HPP


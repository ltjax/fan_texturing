/*  Cataclysm-Software Disaster Engine

	$Id: Texture.hpp 806 2009-06-01 16:09:32Z ltjax $

	Copyright 2006-2007 Marius Elvert
*/

#ifndef GLMM_TEXTURE_HPP
#define GLMM_TEXTURE_HPP

#include <replay/pixbuf.hpp>
#include <boost/utility.hpp>
#include <boost/filesystem/path.hpp>

#include "Object.hpp"

namespace GLmm {

typedef boost::filesystem::path Path;

/** bind point for textures.
*/
class TextureSlot
{
public:
	TextureSlot() : Stage( 0 ) {}
	void				SetStage( unsigned int i ) { Stage = i; }
	unsigned int		GetStage() const { return Stage; }

private:
	unsigned int		Stage;
};

/** generic texture object.
*/
class Texture :
	public boost::noncopyable,
	public GLmm::Object
{
public:
	virtual void						Bind() const = 0;
	void								BindTo( const TextureSlot& Slot ) const;
	virtual								~Texture() {}
};

/** texture 3d object.
*/
class Texture3D :
	public Texture
{

public:
										Texture3D();
										~Texture3D();

    /** bind this texture object.
    */
    void                                Bind() const;
	void								Unbind() const;

	/** set texture parameters.
	*/
	void								SetFilter( GLint MinFilter, GLint MagFilter );
	void								SetWrap( GLint SWrap, GLint TWrap, GLint RWrap );
	inline void							SetWrap( GLint Wrap ) { SetWrap( Wrap, Wrap, Wrap ); }

	/** get the object id as used by OpenGL.
	*/
	unsigned int						GetGLObject() const { return GLObject; }

	/** Load a volume from a tiled picture.
	*/
	void								LoadFromFileTiled( const Path& Filename,
												unsigned int x, unsigned int y );
private:
	GLuint								GLObject;
};

/** texture 2d array object. this is somewhat similar to a 3d texture,
    with the difference that there's no filtering between layers.
*/
class Texture2DArray :
	public Texture
{
public:
										Texture2DArray();
										~Texture2DArray();

	/** bind this texture object.
	*/
	void								Bind() const;
	void								Unbind() const;

	/** set texture parameters.
	*/
	void								SetFilter( GLint MinFilter, GLint MagFilter );
	void								SetWrap( GLint SWrap, GLint TWrap );
	inline void							SetWrap( GLint Wrap ) { SetWrap( Wrap, Wrap ); }
	void								SetCompareMode( GLint Mode );
	void								SetCompareFunc( GLint Function );

	/** set the image data.
	*/
	void								SetImage( GLint Level, GLint InternalFormat,
											GLsizei Width, GLsizei Height, GLsizei Depth,
											GLenum Format, GLenum Type, GLvoid* Data );

	/** Get the object id as used by OpenGL.
	*/
	unsigned int						GetGLObject() const { return GLObject; }

private:
	GLuint								GLObject;

};

/** texture 2d object.
*/
class Texture2D :
	public Texture
{
public:
										Texture2D();
										~Texture2D();
	/** bind this texture object.
	*/
	void								Bind() const;
	void								Unbind() const;

	/** set texture parameters.
	*/
	void								SetFilter( GLint MinFilter, GLint MagFilter );
	void								SetWrap( GLint SWrap, GLint TWrap );
	inline void							SetWrap( GLint Wrap ) { SetWrap( Wrap, Wrap ); }
	void								SetCompareMode( GLint Mode );
	void								SetCompareFunc( GLint Function );
	void								SetGenerateMipmap( bool Value );
	void								GenerateMipmap();

	/** Image upload.
	*/
	void								SetImage( GLint Level, GLint InternalFormat, GLsizei Width, GLsizei Height,
												  GLenum Format, GLenum Type, const GLvoid* Data );

	void								SetImage( GLint Level, GLint InternalFormat, GLsizei Width, GLsizei Height );

	void								SetSubImage( GLint Level, GLint x, GLint y, GLsizei Width, GLsizei Height, 
												GLenum Format, GLenum Type, const GLvoid* Data );

	void								CopyImage( GLint Level, GLint InternalFormat,
												GLint x, GLint y, GLsizei Width, GLsizei Height );

	void								LoadFromFile( const Path& Filename );

	/** Image download.
	*/
	replay::shared_pixbuf				GetImage( GLint Level ) const;

	/** get the object id as used by OpenGL.
	*/
	GLuint								GetGLObject() const { return GLObject; }

private:
	GLuint								GLObject;
};

/** texture rectangle object.
*/
class TextureRect :
	public Texture
{
public:
                                        TextureRect();
                                        ~TextureRect();
    /** bind this texture object.
    */
    void                                Bind() const;
	void								Unbind() const;

	/** set texture parameters.
	*/
	void								SetFilter( GLint MinFilter, GLint MagFilter );
	void								SetWrap( GLint SWrap, GLint TWrap );
	inline void							SetWrap( GLint Wrap ) { SetWrap( Wrap, Wrap ); }

	/** Image upload.
	*/
	void								SetImage( GLint InternalFormat, GLsizei Width, GLsizei Height,
												  GLenum Format, GLenum Type, const GLvoid* Data );

	void								SetImage( GLint InternalFormat, GLsizei Width, GLsizei Height );

	void								SetSubImage( GLint x, GLint y, GLsizei Width, GLsizei Height, 
												GLenum Format, GLenum Type, const GLvoid* Data );

	void								LoadFromFile( const Path& Filename );

	/** Image download.
	*/
	replay::shared_pixbuf				GetImage() const;

	/** get the object id as used by OpenGL.
	*/
	GLuint								GetGLObject() const { return GLObject; }

private:
	GLuint				                GLObject;
};

}

#endif


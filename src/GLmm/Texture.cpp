
#include "Texture.hpp"
#include "GL.hpp"
#include <replay/pixbuf_io.hpp>

using namespace GLmm;

void Texture::BindTo( const TextureSlot& Slot ) const
{
	glActiveTexture( GL_TEXTURE0 + Slot.GetStage() );
	this->Bind();
}

void Texture3D::LoadFromFileTiled( const Path& Filename, unsigned int x, unsigned int y )
{
	replay::shared_pixbuf SourceImage = replay::pixbuf_io::load_from_file( Filename );
	replay::shared_pixbuf Image;

	unsigned int wi = SourceImage->get_width() / x;
	unsigned int hi = SourceImage->get_height() / y;

	glBindTexture( GL_TEXTURE_3D, GLObject );
	glTexImage3D( GL_TEXTURE_3D, 0, GL_RGBA,
		wi,	hi, x*y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
	GLMM_CHECK();

	SetFilter( GL_NEAREST, GL_LINEAR );
	SetWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

	for ( unsigned int j = 0; j < y; ++j )
	{
		for ( unsigned int i = 0; i < x; ++i )
		{
			// TODO: do this totally in GL
			Image = SourceImage->get_sub_image( i*wi, j*hi, wi, hi );

			glTexSubImage3D( GL_TEXTURE_3D, 0, 0, 0, j*x+i, wi, hi,
				1, GL_RGBA, GL_UNSIGNED_BYTE, Image->get_data() );
			GLMM_CHECK();
		}
	}
}

void Texture3D::SetFilter( GLint MinFilter, GLint MagFilter )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, MinFilter ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, MagFilter ); GLMM_CHECK();
}

void Texture3D::SetWrap( GLint SWrap, GLint TWrap, GLint RWrap )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, SWrap ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, TWrap ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, RWrap ); GLMM_CHECK();
}

Texture3D::Texture3D()
{
    glGenTextures( 1, &GLObject ); GLMM_CHECK();
}

Texture3D::~Texture3D()
{
    glDeleteTextures( 1, &GLObject ); GLMM_CHECK();
}

void Texture3D::Bind() const
{
    glBindTexture( GL_TEXTURE_3D, GLObject ); GLMM_CHECK();
}

void Texture3D::Unbind() const
{
    glBindTexture( GL_TEXTURE_3D, 0 ); GLMM_CHECK();
}
void Texture2DArray::SetFilter( GLint MinFilter, GLint MagFilter )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MIN_FILTER, MinFilter ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_MAG_FILTER, MagFilter ); GLMM_CHECK();
}

void Texture2DArray::SetWrap( GLint SWrap, GLint TWrap )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_S, SWrap ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_WRAP_T, TWrap ); GLMM_CHECK();
}

Texture2DArray::Texture2DArray()
{
    glGenTextures( 1, &GLObject ); GLMM_CHECK();
}

Texture2DArray::~Texture2DArray()
{
    glDeleteTextures( 1, &GLObject ); GLMM_CHECK();
}

void Texture2DArray::SetImage( GLint Level, GLint InternalFormat,
							   GLsizei Width, GLsizei Height, GLsizei Depth,
							   GLenum Format, GLenum Type, GLvoid* Data )
{
	Bind();
	glTexImage3D( GL_TEXTURE_2D_ARRAY_EXT, Level, InternalFormat, Width, Height, Depth, 0, Format, Type, Data ); GLMM_CHECK();
}

void Texture2DArray::SetCompareMode( GLint Mode )
{
	this->Bind();
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_COMPARE_MODE, Mode );GLMM_CHECK();
}

void Texture2DArray::SetCompareFunc( GLint Function )
{
	this->Bind();
	glTexParameteri( GL_TEXTURE_2D_ARRAY_EXT, GL_TEXTURE_COMPARE_FUNC, Function );GLMM_CHECK();
}


void Texture2DArray::Bind() const
{
    glBindTexture( GL_TEXTURE_2D_ARRAY_EXT, GLObject ); GLMM_CHECK();
}

void Texture2DArray::Unbind() const
{
    glBindTexture( GL_TEXTURE_2D_ARRAY_EXT, 0 ); GLMM_CHECK();
}

Texture2D::Texture2D()
{
    glGenTextures( 1, &GLObject ); GLMM_CHECK();
}

Texture2D::~Texture2D()
{
    glDeleteTextures( 1, &GLObject ); GLMM_CHECK();
}

void Texture2D::Bind() const
{
    glBindTexture( GL_TEXTURE_2D, GLObject );GLMM_CHECK();
}

void Texture2D::Unbind() const
{
    glBindTexture( GL_TEXTURE_2D, 0 );GLMM_CHECK();
}

void Texture2D::SetFilter( GLint MinFilter, GLint MagFilter )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MinFilter ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MagFilter ); GLMM_CHECK();
}

void Texture2D::SetWrap( GLint SWrap, GLint TWrap )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, SWrap );GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TWrap );GLMM_CHECK();
}

void Texture2D::SetCompareMode( GLint Mode )
{
	this->Bind();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, Mode );GLMM_CHECK();
}

void Texture2D::SetCompareFunc( GLint Function )
{
	this->Bind();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, Function );GLMM_CHECK();
}

void Texture2D::GenerateMipmap()
{
	this->Bind();
	glGenerateMipmap( GL_TEXTURE_2D );
	GLMM_CHECK();
}

void Texture2D::SetImage( GLint Level, GLint InternalFormat, GLsizei Width, GLsizei Height,
						  GLenum Format, GLenum Type, const GLvoid* Data )
{
	this->Bind();

	glTexImage2D( GL_TEXTURE_2D, Level, InternalFormat,
			Width, Height, 0, Format, Type, Data );
	GLMM_CHECK();
}

void Texture2D::SetSubImage( GLint Level, GLint x, GLint y, GLsizei Width, GLsizei Height, 
								GLenum Format, GLenum Type, const GLvoid* Data )
{
	this->Bind();

	glTexSubImage2D( GL_TEXTURE_2D, Level, x, y,
		Width, Height, Format, Type, Data );

	GLMM_CHECK();

}

void Texture2D::SetImage( GLint Level, GLint InternalFormat, GLsizei Width, GLsizei Height )
{
	GLenum Format = GL_RGBA;
	
	switch ( InternalFormat )
	{
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_COMPONENT32F_NV:
		Format = GL_DEPTH_COMPONENT;
		break;
	default:
		break;
	};

	this->SetImage( Level, InternalFormat, Width, Height, Format, GL_UNSIGNED_BYTE, 0 );
}


void Texture2D::CopyImage( GLint Level, GLint InternalFormat,
					GLint x, GLint y, GLsizei Width, GLsizei Height )
{
	this->Bind();
	glCopyTexImage2D( GL_TEXTURE_2D, Level, InternalFormat, x, y, Width, Height, 0 );
	GLMM_CHECK();
}

void Texture2D::SetGenerateMipmap( bool Value )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, Value ? GL_TRUE : GL_FALSE );
}

void Texture2D::LoadFromFile( const Path& Filename )
{
	// load the texture
	replay::shared_pixbuf	Source = replay::pixbuf_io::load_from_file( Filename );

	if ( !Source )
		GLMM_THROW_ERROR( "Unable to load texture: " 
			<< Filename.string() << " (unable to load image file)" );

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	unsigned int Format;

	switch( Source->get_channels() )
    {
        case 1:		Format = GL_LUMINANCE; break;
        case 3:		Format = GL_RGB; break;
        case 4:		Format = GL_RGBA; break;
        default:
			GLMM_THROW_ERROR( "Unable to load texture: " <<
				Filename.string() <<
				" - unsupported number of channels" );
    }

	this->SetImage( 0, Source->get_channels(), Source->get_width(), Source->get_height(),
					Format, GL_UNSIGNED_BYTE, Source->get_data() );
}

replay::shared_pixbuf Texture2D::GetImage( GLint Level ) const
{
	GLint Width = 0;
	GLint Height = 0;
	this->Bind();
	glGetTexLevelParameteriv( GL_TEXTURE_2D, Level, GL_TEXTURE_WIDTH, &Width );GLMM_CHECK();
	glGetTexLevelParameteriv( GL_TEXTURE_2D, Level, GL_TEXTURE_HEIGHT, &Height );GLMM_CHECK();

	replay::shared_pixbuf Result = replay::pixbuf::create( Width, Height, replay::pixbuf::rgba );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );GLMM_CHECK();
	glGetTexImage( GL_TEXTURE_2D, Level, GL_RGBA, GL_UNSIGNED_BYTE, Result->get_data() );GLMM_CHECK();

	return Result;
}

TextureRect::TextureRect()
{
    glGenTextures( 1, &GLObject );
}

TextureRect::~TextureRect()
{
    glDeleteTextures( 1, &GLObject );
}

void TextureRect::Bind() const
{
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, GLObject );
}

void TextureRect::Unbind() const
{
	glBindTexture( GL_TEXTURE_RECTANGLE_ARB, 0 );
}

void TextureRect::SetFilter( GLint MinFilter, GLint MagFilter )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, MinFilter ); GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, MagFilter ); GLMM_CHECK();
}

void TextureRect::SetWrap( GLint SWrap, GLint TWrap )
{
	this->Bind();

	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, SWrap );GLMM_CHECK();
	glTexParameteri( GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, TWrap );GLMM_CHECK();
}

void TextureRect::SetImage( GLint InternalFormat, GLsizei Width, GLsizei Height,
							 GLenum Format, GLenum Type, const GLvoid* Data )
{
	this->Bind();

	glTexImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, InternalFormat,
			Width, Height, 0, Format, Type, Data );
	GLMM_CHECK();
}

void TextureRect::SetSubImage( GLint x, GLint y, GLsizei Width, GLsizei Height, 
								GLenum Format, GLenum Type, const GLvoid* Data )
{
	this->Bind();

	glTexSubImage2D( GL_TEXTURE_RECTANGLE_ARB, 0, x, y,
		Width, Height, Format, Type, Data );

	GLMM_CHECK();

}

void TextureRect::SetImage( GLint InternalFormat, GLsizei Width, GLsizei Height )
{
	this->SetImage( InternalFormat, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, 0 );
}

replay::shared_pixbuf TextureRect::GetImage() const
{
	GLint Width = 0;
	GLint Height = 0;
	this->Bind();
	glGetTexLevelParameteriv( GL_TEXTURE_RECTANGLE_ARB, 0, GL_TEXTURE_WIDTH, &Width );GLMM_CHECK();
	glGetTexLevelParameteriv( GL_TEXTURE_RECTANGLE_ARB, 0, GL_TEXTURE_HEIGHT, &Height );GLMM_CHECK();

	replay::shared_pixbuf Result = replay::pixbuf::create( Width, Height, replay::pixbuf::rgba );

	glPixelStorei( GL_PACK_ALIGNMENT, 1 );GLMM_CHECK();
	glGetTexImage( GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, GL_UNSIGNED_BYTE, Result->get_data() );GLMM_CHECK();

	return Result;
}


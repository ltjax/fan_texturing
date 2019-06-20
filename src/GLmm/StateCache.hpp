/*

Cataclysm-Software Disaster Engine
----------------------------------

Repository-Info:
$Id: StateCache.hpp 810 2009-07-01 17:32:07Z ltjax $

Copyright:
Marius Elvert (marius.elvert@cataclysm-software.com) 2006-2007

*/

#ifndef _GLMM_STATE_CACHE_HPP_
#define _GLMM_STATE_CACHE_HPP_

#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>
#include "Object.hpp"

#include <replay/matrix4.hpp>
#include <replay/transformation.hpp>
#include <replay/pixbuf.hpp>
#include <replay/byte_color.hpp>

namespace GLmm {

namespace rpl = replay;

enum BlendFunc {
	BLEND_NONE = 0,
	BLEND_GLOWING = 1,				// not directly supported
	BLEND_ALPHA_INTERPOLATE = 2,	// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
	BLEND_ALPHA_ADDITIVE = 3,		// GL_SRC_ALPHA, GL_ONE
	BLEND_ALPHA_PREMULT = 4			// GL_ONE, GL_ONE_MINUS_SRC_ALPHA
};


class StateCache :
	public boost::noncopyable,
	public GLmm::Object
{
public:	
	class InternalState;

	class Matrix :
		public GLmm::Object
	{
	private:
		InternalState&				State;

		const GLuint				Name;

		void						Select() const;

									Matrix( InternalState& State, const GLuint Name );

	public:
		void						Push();
		void						Push( const rpl::matrix4& other );
		void						Push( const rpl::affinity& other );

		void						Pop();

		void						ExtractTo( rpl::matrix4& other ) const;
		void						Set( const rpl::matrix4& other );

		Matrix&						Multiply( const rpl::matrix4& other );
		Matrix&						operator*=( const rpl::matrix4& other ) { return Multiply( other ); }
		Matrix&						operator*=( const rpl::affinity& other );
		Matrix&						operator*=( const rpl::transformation& other );

									operator rpl::matrix4() const;
		Matrix&						operator=( const rpl::matrix4& other );
		Matrix&						operator=( const rpl::affinity& x );
		Matrix&						operator=( const rpl::transformation& x );

		Matrix&						SetIdentity();
		Matrix&						Scale( const float factor );
		Matrix&						Scale( const rpl::vector3f& s );
		Matrix&						ScaleBySign( const float sign );
		Matrix&						Rotate( const rpl::quaternion& q );
		Matrix&						Rotate( const float degrees, const rpl::vector3f& axis );
		Matrix&						Translate( const rpl::vector3f& v );
		Matrix&						Translate( const rpl::vector2f& v );
		Matrix&						InverseRotate( const rpl::quaternion& q );
		Matrix&						InverseTranslate( const rpl::vector3f& s );

		friend class StateCache;
	};

	rpl::shared_pixbuf				GetViewportScreenshot();

	void							SetFaceCulling( bool Status );
	void							SetColor( const rpl::vector4f& Color );
	void							SetColor( const rpl::byte_color4& Color );
	void							SetClipPlane( GLenum Plane, const rpl::plane3& x );
	void							SetBlendFunction( BlendFunc Rhs );

									StateCache();
									~StateCache();

	void							Sync();

	// FIXME this needs a new name
	static const rpl::matrix4		ClipspaceToTexturespaceMatrix;

private:
	boost::scoped_ptr< InternalState > State;

public:
	// need to be after the state so they can be initialized with it
	Matrix							Modelview;
	Matrix							Projection;

};

inline
StateCache::Matrix::operator rpl::matrix4() const
{
    rpl::matrix4 Result{ 0 };
	ExtractTo( Result );
	return Result;
}

inline
StateCache::Matrix& StateCache::Matrix::operator=( const rpl::matrix4& Rhs )
{
	Set( Rhs );
	return *this;
}

inline
StateCache::Matrix& StateCache::Matrix::operator=( const rpl::affinity& x )
{
	SetIdentity();
	Translate( x.position );
	Rotate( x.orientation );
	return *this;
}

inline
StateCache::Matrix& StateCache::Matrix::operator=( const rpl::transformation& x )
{
	SetIdentity();
	Translate( x.position );
	ScaleBySign( x.sign );
	Rotate( x.orientation );
	return *this;
}

inline 
void StateCache::Matrix::Push( const rpl::matrix4& Rhs )
{
	Push();
	Set( Rhs );
}

inline
void StateCache::Matrix::Push( const rpl::affinity& Rhs )
{
	Push(); *this = Rhs;
}

}


#endif


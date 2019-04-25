/*

Cataclysm-Software Disaster Engine
----------------------------------

Repository-Info:
$Id: StateCache.cpp 811 2009-08-23 16:32:44Z ltjax $

Copyright:
Marius Elvert (marius.elvert@cataclysm-software.com) 2006-2007

*/

#include <cmath>
#include "StateCache.hpp"
#include <replay/math.hpp>
#include "GL.hpp"

using namespace GLmm;

// init the static
const rpl::matrix4 StateCache::ClipspaceToTexturespaceMatrix(
			0.5f, 0.0f, 0.0f, 0.5f,
			0.0f, 0.5f, 0.0f, 0.5f,
			0.0f, 0.0f, 0.5f, 0.5f,
			0.0f, 0.0f, 0.0f, 1.0f );

class StateCache::InternalState
{
public:
	GLuint					SelectedMatrix;

	InternalState()
	: SelectedMatrix( 0 )
	{
	}
};


void StateCache::Matrix::Select() const
{
	if ( Name != State.SelectedMatrix )
		glMatrixMode( State.SelectedMatrix = Name );
}

StateCache::Matrix::Matrix( InternalState& State, const GLuint Name )
: State( State ), Name( Name )
{
}

void StateCache::Matrix::Push() { Select(); glPushMatrix(); }
void StateCache::Matrix::Pop() { Select(); glPopMatrix(); }
void StateCache::Matrix::Set( const rpl::matrix4& Rhs ) { Select(); glLoadMatrixf( Rhs.ptr() ); }

void StateCache::Matrix::ExtractTo( rpl::matrix4& other ) const
{
	unsigned int GetName = 0;

	switch ( this->Name )
	{
		case GL_MODELVIEW:	GetName = GL_MODELVIEW_MATRIX; break;
		case GL_PROJECTION:	GetName = GL_PROJECTION_MATRIX; break;
		case GL_TEXTURE:	GetName = GL_TEXTURE_MATRIX; break;
		default:
			GLMM_THROW_ERROR( "Unknown matrix name." );
			return;
	};

	glGetFloatv( GetName, other.ptr() );
}


StateCache::Matrix& StateCache::Matrix::Multiply( const rpl::matrix4& other )
{
	Select(); glMultMatrixf( other.ptr() );
	return *this;
}

StateCache::Matrix& StateCache::Matrix::SetIdentity()
{
	Select(); glLoadIdentity();
    return *this;
}

StateCache::Matrix& StateCache::Matrix::Rotate( const float Degrees, const rpl::vector3f& Axis )
{
	Select(); glRotatef( Degrees, Axis[ 0 ], Axis[ 1 ], Axis[ 2 ] );
	return *this;
}

StateCache::Matrix& StateCache::Matrix::Rotate( const rpl::quaternion& q )
{
	float 			Angle = 0.f;
	rpl::vector3f 	Axis;

	Angle = std::acos( rpl::math::clamp( q.w, -1.f, 1.f ) ) * 2.f;

	float Sine = std::sqrt( rpl::math::abs( 1.f - ( q.w * q.w ) ) );

	if ( rpl::math::near_zero( Sine ) )
		Sine = 1.f;
	else
		Sine = 1.f / Sine;

	Axis[ 0 ] = q.x * Sine;
	Axis[ 1 ] = q.y * Sine;
	Axis[ 2 ] = q.z * Sine;

	return Rotate( rpl::math::convert_to_degrees( Angle ), Axis );
}

StateCache::Matrix& StateCache::Matrix::InverseRotate( const rpl::quaternion& q )
{
	float 			Angle = 0.f;
	rpl::vector3f 	Axis;

	Angle = std::acos( q.w ) * 2.f;

	float Sine = std::sqrt( 1.f - ( q.w * q.w ) );

	if ( rpl::math::near_zero( Sine ) )
		Sine = 1.f;
	else
		Sine = 1.f / Sine;

	Axis[ 0 ] = q.x * Sine;
	Axis[ 1 ] = q.y * Sine;
	Axis[ 2 ] = q.z * Sine;

	return Rotate( -rpl::math::convert_to_degrees( Angle ), Axis );
}

StateCache::Matrix& StateCache::Matrix::Scale( const float factor )
{
	Select(); glScalef( factor, factor, factor );
	return *this;
}


StateCache::Matrix& StateCache::Matrix::Scale( const rpl::vector3f& s )
{
	Select(); glScalef( s[ 0 ], s[ 1 ], s[ 2 ] );
	return *this;
}


StateCache::Matrix& StateCache::Matrix::Translate( const rpl::vector3f& v )
{
	Select(); glTranslatef( v[ 0 ], v[ 1 ], v[ 2 ] );
	return *this;
}


StateCache::Matrix& StateCache::Matrix::Translate( const rpl::vector2f& v )
{
	Select(); glTranslatef( v[ 0 ], v[ 1 ], 0.f );
	return *this;
}

StateCache::Matrix& StateCache::Matrix::ScaleBySign( const float sign )
{
	Select();
	float x = 1.f;
	rpl::math::mult_ref_by_sign( x, sign );
	glScalef( x, x, x );
	return *this;
}

StateCache::Matrix& StateCache::Matrix::operator *=( const rpl::affinity& Rhs )
{
	Translate( Rhs.position );
	return Rotate( Rhs.orientation );
}

StateCache::Matrix& StateCache::Matrix::operator *=( const rpl::transformation& Rhs )
{
	Translate( Rhs.position );
	ScaleBySign( Rhs.sign );
	return Rotate( Rhs.orientation );
}


StateCache::Matrix& StateCache::Matrix::InverseTranslate( const rpl::vector3f& v )
{
	Select(); glTranslatef( -v[ 0 ], -v[ 1 ], -v[ 2 ] );
	return *this;
}

StateCache::StateCache()
:   State( new InternalState() ),
	Modelview( *State, GL_MODELVIEW ),
	Projection( *State, GL_PROJECTION )
{
}

StateCache::~StateCache()
{
}

void
StateCache::SetBlendFunction( BlendFunc Rhs )
{
	switch( Rhs )
	{
	case GLmm::BLEND_ALPHA_ADDITIVE:
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		break;
	case GLmm::BLEND_ALPHA_INTERPOLATE:
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		break;
	case GLmm::BLEND_ALPHA_PREMULT:
		glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
		break;
	case GLmm::BLEND_NONE:
		glBlendFunc( GL_ONE, GL_ZERO );
		break;
	};
}

/** Set the current color.
*/
void
StateCache::SetColor( const rpl::vector4f& Color )
{
	glColor4fv( Color.ptr() );
}

/** Set the current color.
*/
void
StateCache::SetColor( const rpl::byte_color4& Color )
{
	glColor4ubv( Color.ptr() );
}

/** Enable/Disable face culling.
*/
void
StateCache::SetFaceCulling( bool Status )
{
	( Status ? glEnable : glDisable )( GL_CULL_FACE );
}

/** Set a clip plane.
*/
void
StateCache::SetClipPlane( GLenum Plane, const rpl::plane3& x )
{
	const double Equation[ 4 ] = { 
		x.normal[ 0 ], x.normal[ 1 ],
		x.normal[ 2 ], x.d };	

	glClipPlane( Plane, Equation );
}



/** Creates a screenshot of the current viewport.
*/
rpl::shared_pixbuf StateCache::GetViewportScreenshot()
{
	int Viewport[ 4 ];

	glGetIntegerv( GL_VIEWPORT, Viewport );

	replay::shared_pixbuf Result = replay::pixbuf::create( 
		Viewport[ 2 ], Viewport[ 3 ], replay::pixbuf::rgb );

	// make sure everything is drawn
	glFinish();

	// pixbuf is tightly packed
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glPixelStorei( GL_PACK_ROW_LENGTH, 0 );
	glPixelStorei( GL_PACK_SKIP_ROWS, 0 );
	glPixelStorei( GL_PACK_SKIP_PIXELS, 0 );

	// Read image data from viewport
	// NOTE: this is slow due to AGP/bus problems
	glReadPixels( Viewport[ 0 ], Viewport[ 1 ], Viewport[ 2 ], Viewport[ 3 ],
					GL_RGB, GL_UNSIGNED_BYTE, Result->get_data() );

	GLMM_CHECK();

	return Result;
}

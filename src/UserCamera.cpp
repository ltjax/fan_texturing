
#include "UserCamera.hpp"
#include <replay/math.hpp>
#include <boost/math/constants/constants.hpp>

CUserCamera::CUserCamera()
: MoveIndex(0)
{
}

CUserCamera::~CUserCamera()
{
}

void
CUserCamera::SetAffinity( const affinity& Rhs ) 
{
	Affinity = Rhs;
}

const affinity&
CUserCamera::GetAffinity() const
{
	return Affinity;
}

void
CUserCamera::OnMouseButton( int px, int py, int Index, int Value )
{
	int Action = 0;

	if ( replay::math::in_range( Index, 2,3 ) )
		Action = Index;

	if ( Value && !MoveIndex )
		MoveIndex = Action;
	else if ( !Value && MoveIndex==Action ) // only disable the one that is active
		MoveIndex = 0;

    LastMouse = { px,py };
}

void
CUserCamera::OnMouseMove( int px, int py )
{
	vector2i CurrentMouse( px, py );
	vector2i Delta = CurrentMouse-LastMouse;
	LastMouse = CurrentMouse;

	if ( MoveIndex == 2 )
	{
		const float MoveSpeed = 1.5f;
		Affinity.position += Affinity.orientation.get_transformed_x()*float(Delta[0])*MoveSpeed;
		Affinity.position -= Affinity.orientation.get_transformed_z()*float(Delta[1])*MoveSpeed;
	}
	else if ( MoveIndex == 3 )
	{
		const quaternion Prefix( boost::math::constants::half_pi<float>(), vec3( 1.f, 0.f, 0.f ) );
		const quaternion InvPrefix = inverse( Prefix );

		const float RotateSpeed = 0.005f;
		
		replay::quaternion YawDelta( -Delta[0]*RotateSpeed, vector3f( 0.f, 1.f, 0.f ) );
		replay::quaternion PitchDelta( Delta[1]*RotateSpeed, vector3f( 1.f, 0.f, 0.f ) );
		Affinity.orientation = Prefix * YawDelta * InvPrefix * Affinity.orientation * PitchDelta;		
	}
}

/*

Cataclysm-Software Disaster Engine
----------------------------------

Repository-Info:
$Id: TSM.cpp 818 2010-01-27 00:19:35Z ltjax $

Copyright:
Marius Elvert (marius.elvert@cataclysm-software.com) 2006-2007

*/

#include "TSM.hpp"

using namespace replay;

namespace {

inline vec2 Intersect( const vec2& g0, const vec2& g1,
		   const vec2& h0, const vec2& h1 )
{
	vec2 i;
	using namespace replay;

	float a, b;

	i[0] = i[1] = 1.0f / math::det(g0 - g1, h0 - h1 );

	a = math::det( g0, g1 );
	b = math::det( h0, h1 );

	i[0] *=	math::det(a, g0[0] - g1[0], b, h0[0] - h1[0]);
	i[1] *=	math::det(a, g0[1] - g1[1], b, h0[1] - h1[1]);
	return i;
}

template < class IteratorType >
inline fcouple ProjectedRange( IteratorType begin, IteratorType end, vec2 n )
{
	float Value = dot( n, *begin );
	fcouple Result( Value, Value );

	for ( ++begin; begin != end; ++begin )
	{
		Value = dot( n, *begin );

		if ( Value < Result[0] )
			Result[0] = Value;
		else if ( Value > Result[1] )
			Result[1] = Value;
	}

	return Result;
}

}


void
TSM::MapTrapezoidToSquare( matrix3& m, const vector2f& t0, const vector2f& t1,
						  const vector2f& t2, const vector2f& t3 )
{
	float a, b, c, d;
	vector2f i;

	//M1 = R * T1
	a = 0.5f * (t2[0] - t3[0]);
	b = 0.5f * (t2[1] - t3[1]);

	m.set( 	 a,   b, a * a + b * b,
	         b,  -a, a * b - b * a,
	       0.f, 0.f, 1.f );

	//M2 = T2 * M1 = T2 * R * T1
	i = Intersect(t0, t3, t1, t2);

	m( 0, 2 ) = -dot( vector2f( m( 0, 0 ), m( 0, 1 ) ), i );
	m( 1, 2 ) = -dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), i );

	//M1 = H * M2 = H * T2 * R * T1
	a = dot( vector2f( m( 0, 0 ), m( 0, 1 ) ), t2 ) + m( 0, 2 );
	b = dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), t2 ) + m( 1, 2 );
	c = dot( vector2f( m( 0, 0 ), m( 0, 1 ) ), t3 ) + m( 0, 2 );
	d = dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), t3 ) + m( 1, 2 );

	a = -(a + c) / (b + d);

	m( 0, 0 ) += m( 1, 0 ) * a;
	m( 0, 1 ) += m( 1, 1 ) * a;
	m( 0, 2 ) += m( 1, 2 ) * a;

	//M2 = S1 * M1 = S1 * H * T2 * R * T1
	a = 1.0f / (dot( vector2f( m( 0, 0 ), m( 0, 1 ) ), t2 ) + m( 0, 2 ) );
	b = 1.0f / (dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), t2 ) + m( 1, 2 ) );

	m.scale_row( 0, a );
	m.scale_row( 1, b );


	//M1 = N * M2 = N * S1 * H * T2 * R * T1
	m( 2, 0 ) = m( 1, 0 ); m( 2, 1 ) = m( 1, 1 ); m( 2, 2 ) = m( 1, 2 );
	m( 1, 2 ) += 1.0f;

	//M2 = T3 * M1 = T3 * N * S1 * H * T2 * R * T1
	a = dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), t0 ) + m( 1, 2 );
	b = dot( vector2f( m( 2, 0 ), m( 2, 1 ) ), t0 ) + m( 2, 2 );
	c = dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), t2 ) + m( 1, 2 );
	d = dot( vector2f( m( 2, 0 ), m( 2, 1 ) ), t2 ) + m( 2, 2 );

	a = -0.5f * (a / b + c / d);

	m.add_scaled_row( 2, a, 1 );


	//M1 = S2 * M2 = S2 * T3 * N * S1 * H * T2 * R * T1
	a = dot( vector2f( m( 1, 0 ), m( 1, 1 ) ), t0 ) + m( 1, 2 );
	b = dot( vector2f( m( 2, 0 ), m( 2, 1 ) ), t0 ) + m( 2, 2 );

	c = -b / a;

	m.scale_row( 1, c );
}

void
TSM::MapTrapezoidToSquare( matrix4& m, const replay::vector2f Trapezoid[], const fcouple& DepthRange )
{
    matrix3 t{ 0.f };

	// do the mapping for the 3x3 matrix
	MapTrapezoidToSquare( t, Trapezoid );

	const float DepthScale = -2.f / ( DepthRange[ 1 ] - DepthRange[ 0 ] );
	const float DepthBias = -( DepthRange[ 1 ] + DepthRange[ 0 ] )
							/( DepthRange[ 1 ] - DepthRange[ 0 ] );

	// embedd into the 4x4 matrix
	m.set( t( 0, 0 ), t( 0, 1 ), 0.f, t( 0, 2 ),
		   t( 1, 0 ), t( 1, 1 ), 0.f, t( 1, 2 ),
				 0.f,		0.f, DepthScale, DepthBias,
		   t( 2, 0 ), t( 2, 1 ), 0.f, t( 2, 2 ) );
}


matrix4 TSM::ComputeWarpingProjection( 
	const plane3* FrustumPlanes, 
	const matrix4& LightSceneMatrix,
	float Focus, float FocusPerc, vec2* TrapezoidResult )
{
	// Compute the frustum in worldspace
	boost::array< vec3, 8 > Frustum;
	vec2					Trapezoid[4];

	math::compute_frustum_corners( FrustumPlanes, Frustum.data() );

	// Move it to lightspace
	for ( unsigned int i = 0; i < 8 ; ++i )
		Frustum[ i ] = LightSceneMatrix * Frustum[ i ];

	// Compute warp in the shadow plane
	ComputeTrapezoid( Frustum, Trapezoid, Focus, FocusPerc );
    matrix3 Warp{ 1.f };
	MapTrapezoidToSquare( Warp, Trapezoid );

	// find the depth range
	fcouple DepthRange( std::numeric_limits<float>::max(), 
		-std::numeric_limits<float>::max() );
	for ( unsigned int i=0; i<8; ++i )
	{
		const vec3& p = Frustum[i];
		float w = p[0]*Warp(2,0)+p[1]*Warp(2,1)+Warp(2,2);
		float pz = p[2] / w;
		
		DepthRange[0]=std::min(pz,DepthRange[0]);
		DepthRange[1]=std::max(pz,DepthRange[1]);
	}

	// Depth scale and depth bias
	const float ds = -2.f / ( DepthRange[ 1 ] - DepthRange[ 0 ] );
	const float db = ( DepthRange[ 1 ] + DepthRange[ 0 ] )
							/( DepthRange[ 1 ] - DepthRange[ 0 ] );
	matrix4 m{ 1.f };
	// this is depth_bias_matrix * warp
	m.set( Warp(0,0),    Warp(0,1), 0.f,    Warp(0,2),
		   Warp(1,0),    Warp(1,1), 0.f,    Warp(1,2),
		db*Warp(2,0), db*Warp(2,1),  ds, db*Warp(2,2),
		   Warp(2,0),    Warp(2,1), 0.f,    Warp(2,2) );

	if ( TrapezoidResult )
		std::copy( Trapezoid, Trapezoid+4, TrapezoidResult );

	return m;
}

bool
TSM::ComputeTrapezoid( const plane3* FrustumPlanes, const matrix4& LightSceneMatrix, vec2* Trapezoid, float Focus, float FocusPerc )
{
	// Compute the frustum in worldspace
	boost::array< vec3, 8 > Frustum;

	math::compute_frustum_corners( FrustumPlanes, Frustum.data() );

	// Move it to lightspace
	for ( unsigned int i = 0; i < 8 ; ++i )
		Frustum[ i ] = LightSceneMatrix * Frustum[ i ];

	return ComputeTrapezoid( Frustum, Trapezoid, Focus, FocusPerc );
}
/** Compute a trapezoidal 2d warping projection.
*/
bool
TSM::ComputeTrapezoid( const boost::array< vec3, 8 >& Frustum, vec2* Trapezoid,
						    float Focus, float FocusPerc )
{
	vec3 Center[ 2 ];

	// compute the center of the near and the far plane respectively, in world space
	Center[ 0 ] = ( Frustum[ 0 ] + Frustum[ 1 ] + Frustum[ 2 ] + Frustum[ 3 ] ) * 0.25f;
	Center[ 1 ] = ( Frustum[ 4 ] + Frustum[ 5 ] + Frustum[ 6 ] + Frustum[ 7 ] ) * 0.25f;

	const float Delta = Focus;

	// compute the line going thru the center of the near plane to the far plane, in world space
	vector3f PL = Center[ 0 ] + normalized( Center[ 1 ] - Center[ 0 ] )*Delta;

	vector2f BaseDelta = math::splice2( Center[ 1 ] ) - math::splice2( Center[ 0 ] );
	float SqrDistance = BaseDelta.squared();

	float ViewDot = std::abs(normalized(Center[1]-Center[0])[2]); // Z coordinate in lightspace

	if ( SqrDistance < 5.f )
	{
		// we're nearly coincident, so just use the far polygon
		Trapezoid[ 0 ] = math::splice2( Frustum[ 4 ] );
		Trapezoid[ 1 ] = math::splice2( Frustum[ 5 ] );
		Trapezoid[ 2 ] = math::splice2( Frustum[ 6 ] );
		Trapezoid[ 3 ] = math::splice2( Frustum[ 7 ] );

		if ( math::det( Trapezoid[ 1 ]-Trapezoid[ 0 ],
			Trapezoid[ 2 ]-Trapezoid[ 1 ] ) < 0.f )
		{
			std::reverse( Trapezoid, Trapezoid + 4 );
		}

		return true;
	}
	else
	{
		// project the frustum to the XY plane
		vec2 ProjectedFrustum[ 8 ];
		std::transform( Frustum.begin(), Frustum.end(), ProjectedFrustum, math::splice2< float > );

		const unsigned int PointCount = math::gift_wrap( ProjectedFrustum, 8 );

		{
			normalize( BaseDelta );

			// figure out the range on the centerline
			fcouple Range = ProjectedRange( ProjectedFrustum,
				ProjectedFrustum + PointCount, BaseDelta );

			const vec2 BaseOrigin = math::splice2( Center[0] );
			const float LineOffset = dot( BaseOrigin, BaseDelta );
			line2 BaseLine( BaseOrigin + (Range[ 0 ]-LineOffset) * BaseDelta, vector2f( -BaseDelta[ 1 ], BaseDelta[ 0 ] ) );
			line2 TopLine( BaseOrigin + (Range[ 1 ]-LineOffset) * BaseDelta, vector2f( BaseDelta[ 1 ], -BaseDelta[ 0 ] ) );

			float Delta_ = std::abs( dot( math::splice2(PL) - BaseLine.origin, BaseDelta ) );
			float Xi = ((FocusPerc * -2.f) + 1.f);
			float Lambda = Range[ 1 ] - Range[ 0 ];
			float Nu = Lambda * Delta_ * ( 1 + Xi ) / ( Lambda * ( 1 - Xi ) - 2*Delta_ );

			// compute the center of the projection
			vector2f q = BaseLine.origin - Nu*BaseDelta;

			unsigned int l=0,r=0;

			for ( unsigned int i = 1; i < PointCount; ++i )
			{
				vector2f t = ProjectedFrustum[ i ]-q;
				if ( math::det( t, ProjectedFrustum[ l ]-q ) > 0.f )
					l = i;

				if ( math::det( ProjectedFrustum[ r ]-q, t ) > 0.f )
					r = i;
			}

			// intersect rays from the center of the projection
			Trapezoid[ 0 ] = Intersect( q, ProjectedFrustum[ l ], TopLine.origin, TopLine.get_point( 1.f ) );
			Trapezoid[ 1 ] = Intersect( q, ProjectedFrustum[ r ], TopLine.origin, TopLine.get_point( 1.f ) );
			Trapezoid[ 2 ] = Intersect( q, ProjectedFrustum[ r ], BaseLine.origin, BaseLine.get_point( 1.f ) );
			Trapezoid[ 3 ] = Intersect( q, ProjectedFrustum[ l ], BaseLine.origin, BaseLine.get_point( 1.f ) );


			float MinAngle = math::lerp( 
				math::convert_to_radians( 30.f ),
				math::convert_to_radians( 90.f ), std::pow( ViewDot, 3.f ) );

			vector2f TopDir = normalized( TopLine.direction );

			float RightAngle = std::acos( dot(TopDir, normalized(Trapezoid[2]-Trapezoid[1])));
			float LeftAngle = std::acos( dot(TopDir, normalized(Trapezoid[0]-Trapezoid[3])) );

			// construct a new right border
			float Cos=std::cos(MinAngle);
			float Sin=std::sin(MinAngle);

			if ( RightAngle < MinAngle )
			{
				vector2f NewDir( Cos*TopDir[0]+Sin*TopDir[1], -Sin*TopDir[0]+Cos*TopDir[1] );

				// find the rightmost point to connect to
				unsigned int r=0;
				for ( unsigned int i = 1; i < PointCount; ++i )
				{
					if ( math::det( ProjectedFrustum[i]-ProjectedFrustum[r], NewDir ) > 0.f )
						r=i;
				}
				vector2f pa = ProjectedFrustum[r];
				vector2f pb = pa + NewDir;

				Trapezoid[1] = Intersect( pa, pb, TopLine.origin, TopLine.get_point( 1.f ) );
				Trapezoid[2] = Intersect( pa, pb, BaseLine.origin, BaseLine.get_point( 1.f ) );
			}

			if ( LeftAngle < MinAngle )
			{
				vector2f NewDir( -Cos*TopDir[0]+Sin*TopDir[1], -Sin*TopDir[0]-Cos*TopDir[1] );
				// find the leftmost point to connect to
				unsigned int l=0;
				for ( unsigned int i = 1; i < PointCount; ++i )
				{
					if ( math::det( ProjectedFrustum[i]-ProjectedFrustum[l], NewDir ) < 0.f )
						l=i;
				}
				
				vector2f pa = ProjectedFrustum[l];
				vector2f pb = pa + NewDir;

				Trapezoid[ 0 ] = Intersect( pa, pb, TopLine.origin, TopLine.get_point( 1.f ) );
				Trapezoid[ 3 ] = Intersect( pa, pb, BaseLine.origin, BaseLine.get_point( 1.f ) );
			}

			return true;
		}
	}
}


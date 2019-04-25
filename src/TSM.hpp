/*  Cataclysm-Software Disaster Engine

	$Id: TSM.hpp 793 2009-05-20 17:02:25Z ltjax $  

	Copyright 2006-2007 Marius Elvert
*/

#ifndef TSM_HPP
#define TSM_HPP

#include "Common.hpp"
#include <boost/array.hpp>
#include <replay/vector2.hpp>
#include <replay/plane3.hpp>
#include <replay/matrix3.hpp>
#include <replay/matrix4.hpp>
#include <replay/transformation.hpp>


namespace TSM
{
	using namespace replay;

	/** builds a 3x3 projective 2D matrix that maps the given trapezoid to the [-1,1]^2 square.
	*/
	void	MapTrapezoidToSquare( matrix3& m, const vector2f& t0, const vector2f& t1,
						  const vector2f& t2, const vector2f& t3 );
	
	/** builds a 3x3 projective 2D matrix that maps the given trapezoid to the [-1,1]^2 square.
	*/
	inline
	void	MapTrapezoidToSquare( matrix3& m, const vector2f Trapezoid[] )
	{
		MapTrapezoidToSquare( m, Trapezoid[ 0 ], Trapezoid[ 1 ], Trapezoid[ 2 ], Trapezoid[ 3 ] );
	};

	/** builds a 4x4 matrix projective 3D matrix that maps the given trapezoid from the XY plane to the [-1,1]^2 square.
	*/
	void	MapTrapezoidToSquare( matrix4& m, const vector2f Trapezoid[], const fcouple& DepthRange );

	matrix4 ComputeWarpingProjection( const plane3* FrustumPlanes, const matrix4& LightSceneMatrix,
		float Focus, float FocusPerc, vec2* Trapezoid=0 );

	bool ComputeTrapezoid( const plane3* Frustum, const matrix4& LightSceneMatrix,
		vec2* Trapezoid, float Focus, float FocusPerc );

	bool ComputeTrapezoid( const boost::array< vec3, 8 >& Frustum, vec2* Trapezoid,
						    float Focus, float FocusPerc );
}

#endif

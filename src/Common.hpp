
#ifndef COMMON_HPP
#define COMMON_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>
#include <replay/vector2.hpp>
#include <replay/vector3.hpp>
#include <replay/affinity.hpp>

using namespace replay;
using boost::scoped_ptr;
using boost::shared_ptr;

typedef unsigned int	uint;
typedef unsigned char	byte;
typedef unsigned char	ubyte;

typedef replay::vector2f vec2;
typedef replay::vector3f vec3;
typedef replay::vector4f vec4;

/** Assertion macro with additional stream arguments.
	Throws std::runtime_error when triggered.
*/
#define ASSERT_MESSAGE( cond, streamargs )			\
	do {											\
		if ( !(cond) ) {							\
			std::ostringstream str;					\
			str << streamargs;						\
			throw std::runtime_error( str.str() );	\
		}											\
	} while ( false )


/** A simple type to encapsulate 3D sphere.
*/
struct CSphere
{
	vec3 Center;
	float Radius;

	CSphere() {}
	CSphere(const vec3& Center,float Radius) :
		Center(Center),Radius(Radius)
	{
	}
};

#endif


#ifndef MISC_HPP
#define MISC_HPP

#include "Common.hpp"
#include <replay/pixbuf.hpp>
#include <replay/matrix2.hpp>
#include <replay/plane3.hpp>
#include <vector>

CSphere ApproximateBoundingBall( const std::vector<CSphere>& Input );

/** Encapsulate a directional light, like the sun.
*/
class CDirectionalLight
{
public:
	CDirectionalLight() {}

	void						Setup( vec3 Direction,
									   const vec4& Ambient,
									   const vec4& Diffuse );

	inline const vec4&			GetDirection() const { return Direction; }
	inline const vec4&			GetDiffuse() const { return Diffuse; }
	inline const vec4&			GetAmbient() const { return Ambient; }
	const matrix4&				GetViewMatrix() const { return ViewMatrix; }

private:
	matrix4						ViewMatrix;
	vec4						Direction;

	vec4						Ambient;
	vec4						Diffuse;
};

/** Encapsulate fundamental display properties.
*/
class CDisplayData
{
public:
	CDisplayData( const vec3& Eypoint, const matrix4& Scene, int w, int h );

	vec3					WorldEyepoint;
	matrix4					Matrix;
	boost::array<plane3,6>	Frustum;
	int						w;
	int						h;
};

/** Encapsulate the matrix to project a point to its corresponding texture coordinate.
*/
class CTexgenBase
{
public:
	vec4		u;
	vec4		v;
};

void AlphaDownsampleRgba(
					const replay::pixbuf& a, const replay::pixbuf& b,
					const replay::pixbuf& c, const replay::pixbuf& d,
					replay::pixbuf& target );

void MinimalAreaBoundingRectangle( const vec2* ConvexHull, uint n, matrix2& A,
								   vec2& MinOut, vec2& MaxOut );

inline void ByteCopyAdvance( const vec3& v, byte*& t )
{
	std::copy( reinterpret_cast<const byte*>(v.ptr()),
		reinterpret_cast<const byte*>(v.ptr()+sizeof(vec3)), t );

	t += sizeof(vec3);
}

inline void ByteCopyAdvance( const vec4& v, byte*& t )
{
	std::copy( reinterpret_cast<const byte*>(v.ptr()),
		reinterpret_cast<const byte*>(v.ptr()+sizeof(vec4)), t );

	t += sizeof(vec4);
}

inline void ByteCopy( const vec3& v, byte* t )
{
	std::copy( reinterpret_cast<const byte*>(v.ptr()),
		reinterpret_cast<const byte*>(v.ptr()+sizeof(vec3)), t );
}

inline void ByteCopyAdvance( const uint n, byte*& t )
{
	std::copy( reinterpret_cast<const byte*>(&n),
		reinterpret_cast<const byte*>(&n+sizeof(uint)),t );
	t+=sizeof(uint);
}

inline void ByteCopy( const uint n, byte* t )
{
	std::copy( reinterpret_cast<const byte*>(&n),
		reinterpret_cast<const byte*>(&n+sizeof(uint)),t );
}

std::size_t JarvisMarch( vec2* Vertex, std::size_t n );

bool IsCCW( const vec2* Hull, std::size_t n );

void PreMultiply( const matrix2& Lhs, vec4& Rhs0, vec4& Rhs1 );

/** Project the given vector x onto base vectors u and v.
*/
inline vec2 Project( const vec4& x, const vec4& u, const vec4& v )
{
	return vec2( x|u, x|v );
}

/** Find the next number that is a multiple of the alignment.
*/
inline uint AlignWith( uint Number, uint Alignment )
{
	return ((Number+Alignment-1)/Alignment)*Alignment;
}

/** Find the next pointer that is a multiple of the alignment.
*/
template <class T>
inline T* AlignWith( T* Pointer, uint Alignment )
{
	T* NullPtr = static_cast<T*>(0);
	return AlignWith(Pointer-NullPtr,Alignment)+NullPtr;
}

template <class T>
inline bool IsAligned( T* Pointer, uint Alignment )
{
	const T* NullPtr = static_cast<T*>(0);
	return ((Pointer-NullPtr)%Alignment)==0;
}

#endif

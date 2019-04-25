/*
replay
Software Library

Copyright (c) 2010 Marius Elvert

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

*/

#ifndef replay_matrix4_hpp
#define replay_matrix4_hpp

#include <cstddef>
#include <replay/vector3.hpp>
#include <replay/vector4.hpp>

namespace replay {

class quaternion;
class plane3;

/** 4x4 float matrix.
	uses opengl-like column major internal format:
	0	4	8	12
	1	5	9	13
	2	6	10	14
	3	7	11	15
	\ingroup Math
*/
class matrix4
{
private:
	float			data[ 16 ];
public:
					matrix4();
	explicit		matrix4( const quaternion& rotation );
					matrix4( const quaternion& rotation, const vector3f& offset );
					matrix4( const quaternion& rotation, const vector3f& offset, float sign );
					matrix4( const float a11, const float a21, const float a31, const float a41,
							 const float a12, const float a22, const float a32, const float a42,
							 const float a13, const float a23, const float a33, const float a43,
							 const float a14, const float a24, const float a34, const float a44 );

	float*			ptr();
	const float*	ptr() const;

	/** Index access operator.
	*/
	template <class index_type>
	float&			operator[]( const index_type i ) { return data[i]; }

	/** Index access operator.
	*/
	template <class index_type>
	const float&	operator[]( const index_type i ) const { return data[i]; }

	
	matrix4&		set( const float a11, const float a21, const float a31, const float a41,
						 const float a12, const float a22, const float a32, const float a42,
						 const float a13, const float a23, const float a33, const float a43,
						 const float a14, const float a24, const float a34, const float a44 );

	matrix4&		set_identity();
	matrix4&		set_rotation_x( const float angle );
	matrix4&		set_rotation_y( const float angle );
	matrix4&		set_rotation_z( const float angle );

	matrix4&		set_rotation( const float angle, const vector3f& axis );
	matrix4&		set_scale( const float sx, const float sy, const float sz );
	matrix4&		set_translation( const vector3f& translation );

	void			set_column( unsigned int i, const vector4f& column );
	void			set_row( unsigned int i, const vector4f& row );

	void			swap_column( unsigned int i, unsigned int j );
	void			swap_row( unsigned int i, unsigned int j );

	const vector4f	get_column( unsigned int i ) const;
	const vector4f	get_row( unsigned int i ) const;
	
	const matrix4	inverted_orthogonal() const;
	float			determinant() const;

	void			transpose();

	matrix4&		scale( const float x, const float y, const float z );
	matrix4&		scale( const vector3f& v );

	matrix4&		translate( const float x, const float y, const float z );
	matrix4&		translate( const vector3f& rhs );

	static void		multiply( const matrix4& a, const matrix4& b, matrix4& result );

	/** Get matrix elements by their indices.
	*/
	float&			operator()( unsigned int r, unsigned int c ) { return data[ (c<<2)+r ]; }
	
	/** Get matrix elements by their indices.
	*/
	float			operator()( unsigned int r, unsigned int c ) const { return data[ (c<<2)+r ]; }

	const matrix4	operator*( const matrix4& other ) const;
	const matrix4	operator+( const matrix4& other ) const;
	const matrix4	operator*( const float rhs ) const;

	const vector4f	operator*( const vector4f& other ) const;
	const vector3f	operator*( const vector3f& other ) const;

	const vector4f	multiply3( const vector3f& rhs ) const;

	matrix4&		operator=( const quaternion& rotation );
	matrix4&		operator*=( const matrix4& other );
	matrix4&		operator*=( const float rhs );
	matrix4&		operator+=( const matrix4& other );

};

plane3 operator*( const plane3& p, const matrix4& m );

/** Get a pointer to the elements.
*/
inline float* matrix4::ptr()
{
	return data;
}

/** Get a pointer to the elements.
*/
inline const float* matrix4::ptr() const
{
	return data;
}

/** Multiply two matrices.
*/
inline void
matrix4::multiply( const matrix4& a, const matrix4& b, matrix4& result )
{
	result.data[ 0] = b.data[ 0]*a.data[ 0] + b.data[ 1]*a.data[ 4] + b.data[ 2]*a.data[ 8] + b.data[ 3]*a.data[12];
	result.data[ 1] = b.data[ 0]*a.data[ 1] + b.data[ 1]*a.data[ 5] + b.data[ 2]*a.data[ 9] + b.data[ 3]*a.data[13];
	result.data[ 2] = b.data[ 0]*a.data[ 2] + b.data[ 1]*a.data[ 6] + b.data[ 2]*a.data[10] + b.data[ 3]*a.data[14];
	result.data[ 3] = b.data[ 0]*a.data[ 3] + b.data[ 1]*a.data[ 7] + b.data[ 2]*a.data[11] + b.data[ 3]*a.data[15];

	result.data[ 4] = b.data[ 4]*a.data[ 0] + b.data[ 5]*a.data[ 4] + b.data[ 6]*a.data[ 8] + b.data[ 7]*a.data[12];
	result.data[ 5] = b.data[ 4]*a.data[ 1] + b.data[ 5]*a.data[ 5] + b.data[ 6]*a.data[ 9] + b.data[ 7]*a.data[13];
	result.data[ 6] = b.data[ 4]*a.data[ 2] + b.data[ 5]*a.data[ 6] + b.data[ 6]*a.data[10] + b.data[ 7]*a.data[14];
	result.data[ 7] = b.data[ 4]*a.data[ 3] + b.data[ 5]*a.data[ 7] + b.data[ 6]*a.data[11] + b.data[ 7]*a.data[15];

	result.data[ 8] = b.data[ 8]*a.data[ 0] + b.data[ 9]*a.data[ 4] + b.data[10]*a.data[ 8] + b.data[11]*a.data[12];
	result.data[ 9] = b.data[ 8]*a.data[ 1] + b.data[ 9]*a.data[ 5] + b.data[10]*a.data[ 9] + b.data[11]*a.data[13];
	result.data[10] = b.data[ 8]*a.data[ 2] + b.data[ 9]*a.data[ 6] + b.data[10]*a.data[10] + b.data[11]*a.data[14];
	result.data[11] = b.data[ 8]*a.data[ 3] + b.data[ 9]*a.data[ 7] + b.data[10]*a.data[11] + b.data[11]*a.data[15];

	result.data[12] = b.data[12]*a.data[ 0] + b.data[13]*a.data[ 4] + b.data[14]*a.data[ 8] + b.data[15]*a.data[12];
	result.data[13] = b.data[12]*a.data[ 1] + b.data[13]*a.data[ 5] + b.data[14]*a.data[ 9] + b.data[15]*a.data[13];
	result.data[14] = b.data[12]*a.data[ 2] + b.data[13]*a.data[ 6] + b.data[14]*a.data[10] + b.data[15]*a.data[14];
	result.data[15] = b.data[12]*a.data[ 3] + b.data[13]*a.data[ 7] + b.data[14]*a.data[11] + b.data[15]*a.data[15];
}


/** Multiply two matrices.
*/
inline const matrix4
matrix4::operator*( const matrix4& m ) const
{
	matrix4 result;

	multiply( *this, m, result );

	return result;
}

/** Multiply a vector by the matrix, assuming the forth component to be 1 and the last row in the matrix to be [0,0,0,1]. 
*/
inline const vector3f
matrix4::operator*( const vector3f& other ) const
{
	vector3f result;

	result[ 0 ] = data[0]*other[0] + data[4]*other[1] + data[ 8]*other[2] + data[12];
	result[ 1 ] = data[1]*other[0] + data[5]*other[1] + data[ 9]*other[2] + data[13];
	result[ 2 ] = data[2]*other[0] + data[6]*other[1] + data[10]*other[2] + data[14];

	return result;
}

/** Multiply a vector by the matrix, assuming the forth component to be 1. 
*/
inline const vector4f
matrix4::multiply3( const vector3f& other ) const
{
	vector4f result;

	result[ 0 ] = data[0]*other[0] + data[4]*other[1] + data[ 8]*other[2] + data[12];
	result[ 1 ] = data[1]*other[0] + data[5]*other[1] + data[ 9]*other[2] + data[13];
	result[ 2 ] = data[2]*other[0] + data[6]*other[1] + data[10]*other[2] + data[14];
	result[ 3 ] = data[3]*other[0] + data[7]*other[1] + data[11]*other[2] + data[15];

	return result;
}

/** Multiply a vector by the matrix.
*/
inline const vector4f
matrix4::operator*( const vector4f& other ) const
{
	vector4f result;

	result[ 0 ] = data[0]*other[0] + data[4]*other[1] + data[ 8]*other[2] + data[12]*other[3];
	result[ 1 ] = data[1]*other[0] + data[5]*other[1] + data[ 9]*other[2] + data[13]*other[3];
	result[ 2 ] = data[2]*other[0] + data[6]*other[1] + data[10]*other[2] + data[14]*other[3];
	result[ 3 ] = data[3]*other[0] + data[7]*other[1] + data[11]*other[2] + data[15]*other[3];

	return result;
}

/** Inplace multiply the matrix by a scalar.
*/
inline matrix4&
matrix4::operator*=(const float rhs)
{
	for ( std::size_t i=0; i<16; ++i )
		data[i]*=rhs;
	return *this;
}

/** Inplace multiply the matrix.
	\note Due to the nature of matrix multiplication, this will create a temporary matrix internally.
*/
inline matrix4&
matrix4::operator*=(const matrix4& Other)
{
	*this = *this * Other;
	return *this;
}


}

#endif // replay_matrix4_hpp



#ifndef FANTEX_CURSOR_HPP
#define FANTEX_CURSOR_HPP

#include <boost/utility.hpp>
#include "Common.hpp"
#include "GLmm\Program.hpp"
#include "GLmm\BufferObject.hpp"

class CCursor :
	public boost::noncopyable
{
public:
	CCursor();
	~CCursor();

	bool					GetVisible() const { return Visible; }
	void					SetVisible( bool Rhs ) { Visible=Rhs; }

	void					SetPosition( const vec3& Rhs ) { Point=Rhs; }
	vec3					GetPosition() const { return Point; }

	float					GetRadius() const { return Radius; }
	void					SetRadius( float Rhs ) { Radius=Rhs; }

	vec4					GetColor() const { return Color; }
	void					SetColor( const vec4& Rhs ) { Color=Rhs; } 

	CSphere					GetSphere() const { return CSphere(Point,Radius); }

	void					Render( const matrix4& View, const matrix4& Proj );

private:
	// Renderer Bits
	GLmm::Program			Program;
	GLmm::BufferObject		Vertices;
	GLmm::BufferObject		Indices;
	size_t					VertexCount;
	size_t					IndexCount;

	// Data bits
	vec4					Color;
	bool					Visible;
	vec3					Point;
	float					Radius;
};

#endif // FANTEX_CURSOR_HPP

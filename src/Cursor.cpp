
#include "Cursor.hpp"
#include "GLSLUtils.hpp"
#include <boost/filesystem/fstream.hpp>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <replay/math.hpp>
#include <boost/math/constants/constants.hpp>

namespace {

/** Generate a sphere model.
*/
void GenerateSphereModel( int LongitudeDiv, int LatitudeDiv,
	std::vector<vec3>& PointData, std::vector<unsigned short>& TriangleData )
{
	using namespace boost::assign;

	// Add the north pole
	PointData += vec3(0.f,0.f,1.f);
	for ( int x=0; x<LongitudeDiv; ++x )
	{
		float Yaw = (static_cast<float>(x)/LongitudeDiv)*boost::math::constants::two_pi<float>();

		float px=std::cos(Yaw);
		float py=std::sin(Yaw);

		for ( int y=0; y<LatitudeDiv; ++y )
		{
			// Relative to the north pole
			float Angle = ((y+1) / (LatitudeDiv+1.f))* boost::math::constants::pi<float>();
			float z = std::cos(Angle);
			float r = std::sin(Angle);

			PointData += vec3(r*px,r*py,z);
		}
	}
	// Add the south pole
	PointData += vec3(0.f,0.f,-1.f);

	// Fill geometry between different longitudes
	for ( int x=0; x<LongitudeDiv; ++x )
	{
		// Connect to the north pole
		unsigned short LastLeft = x*LatitudeDiv+1;
		unsigned short LastRight = ((x+1)%LongitudeDiv)*LatitudeDiv+1;

		TriangleData += 0,LastLeft,LastRight;

		for ( int y=0; (y+1)<LatitudeDiv; ++y )
		{
			unsigned short NewLeft = LastLeft+1;
			unsigned short NewRight = LastRight+1;

			if ( (y+x)&1 ) // Alternate diagonals
			{
				// LastRight -> NewLeft diagonal
				TriangleData += LastLeft,NewLeft,LastRight,
					LastRight,NewLeft,NewRight;
			}
			else
			{
				// LastLeft -> NewRight diagonal
				TriangleData += LastLeft,NewRight,LastRight,
					LastLeft,NewLeft,NewRight;
			}
			LastLeft = NewLeft;
			LastRight = NewRight;
		}
		
		// Connect to the south pole
		TriangleData += LastRight,LastLeft,(PointData.size()-1);
	}
}

};

CCursor::CCursor()
	: Visible(false), Radius(2.5f), Color(1.f, 0.f, 0.f, 1.f)
{
	// Setup the cursor
	std::vector<vec3> PointData;
	std::vector<unsigned short> TriangleData;

	int LongitudeDiv = 32;
	int LatitudeDiv = LongitudeDiv / 2;

	GenerateSphereModel( 32, 16, PointData, TriangleData );

	// Send the data to the GL
	VertexCount=PointData.size();
	Vertices.SetData( GL_ARRAY_BUFFER, PointData, GL_STATIC_DRAW );
	IndexCount=TriangleData.size();
	Indices.SetData( GL_ELEMENT_ARRAY_BUFFER, TriangleData, GL_STATIC_DRAW );

	// Setup a GPU program
	CompileShader( boost::filesystem::current_path()/"Cursor.glsl", Program );
	glBindAttribLocation( Program.GetGLObject(), 0, "Vertex" );
	Program.Link();
}

CCursor::~CCursor()
{
}


void CCursor::Render( const matrix4& View, const matrix4& Proj )
{
	if ( !Visible )
		return;

	glEnable( GL_BLEND );
	glDepthMask( GL_FALSE );
	glBlendFunc( GL_ONE, GL_ONE );

	matrix4 ViewC = View;
	ViewC.translate( Point );
	ViewC.scale( vec3(Radius) );

	Vertices.Bind(GL_ARRAY_BUFFER);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer( 0,3, GL_FLOAT, GL_FALSE, sizeof(vec3), NULL );
	Program.Use();
	glUniformMatrix4fv( Program.GetUniformLocation("Modelview"),1,GL_FALSE,ViewC.ptr());
	glUniformMatrix4fv( Program.GetUniformLocation("Projection"),1,GL_FALSE,Proj.ptr());
	glUniform4fv( Program.GetUniformLocation("Color"),1,Color.ptr() );
	Indices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	glDrawRangeElements(GL_TRIANGLES,0,VertexCount-1,IndexCount,GL_UNSIGNED_SHORT,0);
	glDisableVertexAttribArray(0);
		
	glDepthMask( GL_TRUE );
	glDisable( GL_BLEND );
}

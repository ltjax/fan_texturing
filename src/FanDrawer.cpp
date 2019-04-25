
#include <boost/bind.hpp>

#include <replay/couple.hpp>
#include <replay/vector_math.hpp>

#include "GLmm/GL.hpp"

#include "FanDrawer.hpp"
#include "FantexMesh.hpp"
#include "PageAtlas.hpp"


CFanDrawer::CFanDrawer( CFantexMesh& Mesh )
: Mesh(Mesh), VboElementCount(24)
{
	Vbo.SetData( GL_ARRAY_BUFFER, sizeof(vec3)*2*VboElementCount, 0, GL_STREAM_DRAW );
}

void CFanDrawer::Draw( uint Index )
{
	using replay::fcouple;
	using replay::matrix4;

	const std::vector<CTexgenBase>& NormalizedBase =
		Mesh.GetBaseVector();
	const std::vector<vec3>& Coord =
		Mesh.GetCoordVector();
	const std::vector<vec3>& Normal =
		Mesh.GetNormalVector();

	const float		OneThird = 1.f/3.f;
	const float		SeamWidth = 0.01f;
	const vec2		SeamTip = vec2( (2.f*SeamWidth), 2.f*SeamWidth)+ vec2(OneThird, OneThird);
	const float		SideP = 3.f*SeamWidth + 1.f/2.f;

	// Draw a leaf

	glClearColor(0.f,0.f,0.f,0.f);
	glClear( GL_COLOR_BUFFER_BIT );

	matrix4 View;
	View.set_identity();

	const vec4& u = NormalizedBase[Index].u;
	const vec4& v = NormalizedBase[Index].v;

	View.set_row(0,u);
	View.set_row(1,v);
	View = View;

	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( View.ptr() );


	std::size_t VertexCount = Mesh.GetTriangleCount(Index)*3;

	// Make sure the VBO is big enough
	bool VboResized=false;
	while ( VertexCount > VboElementCount )
	{
		VboElementCount*=2;
		VboResized=true;
	}

	if ( VboResized )
		Vbo.SetData( GL_ARRAY_BUFFER, VboElementCount*2*sizeof(vec3), 0, GL_STREAM_DRAW );

	// Generate data for the VBO
	Vbo.SetGeneratedSubData( GL_ARRAY_BUFFER, 0, VertexCount*2*sizeof(vec3),
		boost::bind( &CFanDrawer::FillVertexBuffer, this, Index, _1 ) );

	// Setup vertex attributes
	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_NORMAL_ARRAY );
	glVertexPointer(3, GL_FLOAT, sizeof(vec3)*2, (GLubyte*)sizeof(vec3) );
	glNormalPointer(GL_FLOAT, sizeof(vec3)*2, 0 );
	GLMM_CHECK();

	// Draw
	glDrawArrays( GL_TRIANGLES, 0, GLsizei(VertexCount) );
	GLMM_CHECK();

	// Disable vertex attribs
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_NORMAL_ARRAY );
	GLMM_CHECK();
}

/** Fill the vertex buffer for rendering.
*/
void CFanDrawer::FillVertexBuffer( uint Index, GLubyte* Target )
{
	const float Sqrt2 = std::sqrt(2.f);

	// Get the source buffers
	const std::vector<CTexgenBase>& NormalizedBase =
		Mesh.GetBaseVector();
	const std::vector<vec3>& Coord =
		Mesh.GetCoordVector();
	const std::vector<vec3>& Normal =
		Mesh.GetNormalVector();

	// Shorthandles
	const vec4& u = NormalizedBase[Index].u;
	const vec4& v = NormalizedBase[Index].v;

	uint Triangle[3];
	for ( std::size_t j=0,e=Mesh.GetTriangleCount(Index); j<e; ++j )
	{
		Mesh.GetTriangle( Index, uint(j), Triangle );

		using replay::math::lerp;

		// Compute the additional triangle vertices
		const vec3 a = Coord[Triangle[0]];
		const vec3 na = Normal[Triangle[0]];
		const vec3 b = Coord[Triangle[1]];
		const vec3 nb = Normal[Triangle[1]];
		const vec3 c = Coord[Triangle[2]];
		const vec3 nc = Normal[Triangle[2]];

		const vec2 pba = Project(vec4(a-b,0.f),u,v);
		const vec2 pbc = Project(vec4(c-b,0.f),u,v);

		float LengthC = magnitude(pba);
		float Beta = std::acos((pba|normalized(pbc))/LengthC);
		float Height = std::sin(Beta)*LengthC;

		float Factor = std::max( Sqrt2/Height,1.f );

		// Draw to triangles to fill this
		// Uses [Normal/Vertex] layout
		ByteCopyAdvance( na, Target );
		ByteCopyAdvance( a, Target );

		ByteCopyAdvance( lerp(na,nb,Factor), Target );
		ByteCopyAdvance( lerp(a,b,Factor), Target );

		ByteCopyAdvance( lerp(na,nc,Factor), Target );
		ByteCopyAdvance( lerp(a,c,Factor), Target );
	}
}

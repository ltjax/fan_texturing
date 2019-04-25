
#include "FantexRenderer.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <replay/plane3.hpp>
#include <replay/vector_math.hpp>
#include "PageAtlas.hpp"
#include "GLmm/GL.hpp"
#include "GLSLUtils.hpp"
#include <boost/filesystem.hpp>


namespace {

inline bool CullSphere( const boost::array<replay::plane3,6>& Frustum, const vector3f& Center, float Radius )
{
	for ( uint i=0; i<6; ++i )
	{
		float d=distance(Frustum[i],Center);
		if ( -d > Radius )
			return true;
	}
	return false;
}

inline float GetAxisScreenLength( const vec3& D, const replay::matrix4& M )
{
	// This is an optimized version of getting the first two components of M*(D|0)
	const vec2 ScreenD( M[0]*D[0]+M[4]*D[1]+M[8]*D[2],
		M[1]*D[0]+M[5]*D[2]+M[9]*D[2] );

	return ScreenD.squared();
}

inline float GetElementCoverage( vec3 Point, const CTexgenBase& Base, const replay::matrix4& M, 
							 const replay::plane3& NearPlane )
{
	// If the point was clipped by the near plane, project onto it
	float Distance=distance(NearPlane,Point);
	if ( Distance < 0.f )
		Point += Distance*NearPlane.normal;
	
	// Compute the center points w coordinate (approximation for the other points)
	// Optimized version of M*(Point|1)
	const float pw=M[3]*Point[0]+M[7]*Point[1]+M[11]*Point[2]+M[15];

	vec3 U(Base.u.ptr());
	U *= (1.f/U.squared());

	vec3 V(Base.v.ptr());
	V *= (1.f/V.squared());

	return std::sqrt(std::max(GetAxisScreenLength(U,M),
		GetAxisScreenLength(V,M)))/pw;
}

}


CFantexRenderer::CFantexRenderer( CFantexMesh& Mesh,
								  const boost::filesystem::path& PageFile,
								  uint PageNumSqrt )
: Mesh(Mesh), MipmapBias(0.9f), FanDrawer(Mesh), SmoothBlend(true)
{
	// Load and compile the shader
	CompileShader( "Shader.glsl", Program );

	glProgramParameteriEXT(Program.GetGLObject(),GL_GEOMETRY_INPUT_TYPE_EXT,GL_TRIANGLES);
	glProgramParameteriEXT(Program.GetGLObject(),GL_GEOMETRY_OUTPUT_TYPE_EXT,GL_TRIANGLE_STRIP);
	glProgramParameteriEXT(Program.GetGLObject(),GL_GEOMETRY_VERTICES_OUT_EXT,3);
	GLMM_CHECK();
	
	glBindAttribLocation(Program.GetGLObject(),0,"Coord");

	Program.Link();

	const GLuint VertexCount = GLuint(Mesh.GetVertexCount());
	const GLsizei IndexCount = GLsizei(Mesh.GetIndexCount());

	// Create the texture management
	PageAtlas.reset( new CPageAtlas( PageFile, VertexCount, PageNumSqrt ) );

	// Setup the static vertex buffer
	std::size_t VertexDataSize = (sizeof(vec3)+sizeof(vec4)*2)*VertexCount;
	VertexBuffer.SetGeneratedData( GL_ARRAY_BUFFER, VertexDataSize, 
		boost::bind( &CFantexRenderer::CopyVertexData, this, _1 ), GL_STATIC_DRAW );

	// Setup the dynamic vertex buffer
	OffsetBuffer.SetData( GL_ARRAY_BUFFER, sizeof(vec3)*VertexCount,
		0, GL_DYNAMIC_DRAW );

	GLmm::BufferObject::Unbind(GL_ARRAY_BUFFER);

	const std::vector<uint>& Indices( Mesh.GetIndexVector() );
	IndexBuffer.SetData( GL_ELEMENT_ARRAY_BUFFER, Indices, GL_STATIC_DRAW );

	GLmm::BufferObject::Unbind( GL_ELEMENT_ARRAY_BUFFER );

	// Setup the editing objects
	Rbo.SetStorage( GL_RGBA, PageAtlas->GetTileSize(), PageAtlas->GetTileSize() );
	Fbo.Attach( GL_COLOR_ATTACHMENT0, Rbo );
	Fbo.TestCompleteness();
	GLmm::Framebuffer::Unbind();

	CompileShader( boost::filesystem::current_path()/"Edit.glsl",EditProgram);
	EditProgram.Link();
}


bool CFantexRenderer::GetSmoothBlend() const
{
	return SmoothBlend;
}

void CFantexRenderer::SetSmoothBlend( bool Rhs )
{
	if ( Rhs == SmoothBlend )
		return;

	GLmm::Program NewProgram;
	std::vector<std::string> Defs;

	SmoothBlend = Rhs;
	if ( !Rhs )
		Defs.push_back( "NO_SMOOTH_BLEND" );
	
	CompileShader( "Shader.glsl", Defs, NewProgram );

	glProgramParameteriEXT(NewProgram.GetGLObject(),GL_GEOMETRY_INPUT_TYPE_EXT,GL_TRIANGLES);
	glProgramParameteriEXT(NewProgram.GetGLObject(),GL_GEOMETRY_OUTPUT_TYPE_EXT,GL_TRIANGLE_STRIP);
	glProgramParameteriEXT(NewProgram.GetGLObject(),GL_GEOMETRY_VERTICES_OUT_EXT,3);
	GLMM_CHECK();
	
	glBindAttribLocation(NewProgram.GetGLObject(),0,"Coord");

	NewProgram.Link();

	boost::swap( NewProgram, this->Program );
}

void CFantexRenderer::SetBias( float Bias )
{
	MipmapBias = Bias;
}

float CFantexRenderer::GetBias() const
{
	return MipmapBias;
}

void CFantexRenderer::SetEditable( bool Rhs )
{
	PageAtlas->SetEditable( Rhs );
}

bool CFantexRenderer::GetEditable() const
{
	return PageAtlas->GetEditable();
}

#ifdef RUNTIME_SHADOWS
void CFantexRenderer::Render( GLmm::Texture2DArray& ShadowTexture, const matrix4& ShadowMatrix )
#else
void CFantexRenderer::Render()
#endif
{
	Program.Use();

	const GLuint VertexCount = GLuint(Mesh.GetVertexCount());
	const GLsizei IndexCount = GLsizei(Mesh.GetIndexCount());

	// Get the attribute locations
	const int CoordAttrib = Program.GetAttribLocation( "Coord" );
	const int BaseUAttrib = Program.GetAttribLocation( "BaseU" );
	const int BaseVAttrib = Program.GetAttribLocation( "BaseV" );
	const int OffsetAttrib = Program.GetAttribLocation( "Offset" );

	// Get uniform locations
	const int TextureLocation = Program.GetUniformLocation( "Texture" );
	const int HalfTexelUniform = Program.GetUniformLocation( "HalfTexel" );

	const uint TextureSize = PageAtlas->GetTextureSize();

	glUniform1f( HalfTexelUniform, float(0.5/double(TextureSize)) );

	// Bind the shadow texture
#ifdef RUNTIME_SHADOWS
	const int ShadowLocation = Program.GetUniformLocation("ShadowTexture");
	glUniform1i( ShadowLocation, 1 );
	glActiveTexture( GL_TEXTURE1 );
	ShadowTexture.Bind();

	// Bind the shadow matrix
	glUniformMatrix4fv( Program.GetUniformLocation("ShadowMatrix"),1,GL_FALSE,
		ShadowMatrix.ptr() );
#endif

	// Bind the page cache
	glActiveTexture( GL_TEXTURE0);
	glUniform1i( TextureLocation, 0 );
	PageAtlas->BindTexture();

	// Setup the static buffer vertex format
	VertexBuffer.Bind(GL_ARRAY_BUFFER);

	glEnableVertexAttribArray( CoordAttrib );
	glEnableVertexAttribArray( BaseUAttrib );
	glEnableVertexAttribArray( BaseVAttrib );

	GLsizei VertexSize = sizeof(vec3)+sizeof(vec4)*2;
	glVertexAttribPointer( CoordAttrib, 3, GL_FLOAT, GL_FALSE,
		VertexSize, 0 );
	glVertexAttribPointer( BaseUAttrib, 4, GL_FLOAT, GL_FALSE,
		VertexSize, (char*)sizeof(vec3) );
	glVertexAttribPointer( BaseVAttrib, 4, GL_FLOAT, GL_FALSE,
		VertexSize, (char*)(sizeof(vec4)+sizeof(vec3)) );

	// Setup the dynamic buffer vertex format
	OffsetBuffer.Bind(GL_ARRAY_BUFFER);
	glEnableVertexAttribArray( OffsetAttrib );
	glVertexAttribPointer( OffsetAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0 );

	GLmm::BufferObject::Unbind(GL_ARRAY_BUFFER);

	IndexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);
	glDrawRangeElements(GL_TRIANGLES,0,VertexCount-1,IndexCount,GL_UNSIGNED_INT,0);
	GLmm::BufferObject::Unbind(GL_ELEMENT_ARRAY_BUFFER);

	glDisableVertexAttribArray( BaseUAttrib );
	glDisableVertexAttribArray( BaseVAttrib );
	glDisableVertexAttribArray( CoordAttrib );
	glDisableVertexAttribArray( OffsetAttrib );
}


void CFantexRenderer::RenderDepth( GLmm::Program& Program )
{
	const GLuint VertexCount = GLuint(Mesh.GetVertexCount());
	const GLsizei IndexCount = GLsizei(Mesh.GetIndexCount());

	const int CoordAttrib = Program.GetAttribLocation( "Coord" );

	// Setup the static buffer vertex format
	VertexBuffer.Bind(GL_ARRAY_BUFFER);

	glEnableVertexAttribArray( CoordAttrib );

	const GLsizei VertexSize = sizeof(vec3)+sizeof(vec4)*2;
	glVertexAttribPointer( CoordAttrib, 3, GL_FLOAT, GL_FALSE,
		VertexSize, 0 );

	IndexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);
	glDrawRangeElements(GL_TRIANGLES,0,VertexCount-1,
		IndexCount,GL_UNSIGNED_INT,0);
	GLmm::BufferObject::Unbind(GL_ELEMENT_ARRAY_BUFFER);

	glDisableVertexAttribArray( CoordAttrib );
}



void CFantexRenderer::CopyVertexData( GLubyte* Target )
{
	const std::vector<vec3>& Coord( Mesh.GetCoordVector() );
	const std::vector<CTexgenBase>& NormalizedBase( Mesh.GetBaseVector() );

	for ( std::size_t i=0; i < Coord.size(); ++i )
	{
		ByteCopyAdvance( Coord[i], Target );
		ByteCopyAdvance( NormalizedBase[i].u, Target );
		ByteCopyAdvance( NormalizedBase[i].v, Target );
	}
}


void CFantexRenderer::UpdateOffsets( GLubyte* Target, const CDisplayData& DisplayData,
									const CSphere* CursorSphere )
{
	float CursorPreloadRadius = 0.f;
	
	if ( CursorSphere )
		CursorPreloadRadius = CursorSphere->Radius * 1.5f;

	const std::vector<vec3>& Coord( Mesh.GetCoordVector() );
	const std::vector<vec3>& Normal( Mesh.GetNormalVector() );
	const std::vector<CTexgenBase>& NormalizedBase( Mesh.GetBaseVector() );

	uint TileSize = PageAtlas->GetTileSize();
	uint MaxLevel = GetMaximumLevel( uint(Mesh.GetVertexCount()) );
	PageAtlas->UpdatePages();

	uint TopLevel = IntegerLog(TileSize);

	// Build a model to viewport matrix (with 0,0 at the center)
	matrix4 ScreenMatrix=matrix4().set_scale(0.5f*DisplayData.w,0.5f*DisplayData.h,1.f)
		* DisplayData.Matrix;

	// Select pages for each vertex
	for ( CPageAtlas::CPageIterator Current=PageAtlas->GetBegin(); Current.IsValid(); ++Current )
	{
		std::size_t Index = Current.GetIndex();
		const CSphere& Sphere( Mesh.GetFanBounds(Index) );

		// Check if this fan is visible..
		if ( CullSphere( DisplayData.Frustum, Sphere.Center, Sphere.Radius ) )
			continue;

		// Check if we need to load it because of the cursor
		if ( CursorSphere )
		{
			const float Distance = distance(CursorSphere->Center,Sphere.Center)-Sphere.Radius;

			if ( Distance < CursorPreloadRadius )
			{
				PageAtlas->ResetDrawCounter( Current );
				PageAtlas->AsyncMap(page_id(0,Index));
			}
		}

		// Find the direction to the fan center
		vec3 d=DisplayData.WorldEyepoint-Coord[Index];
		d /= magnitude(d);

		// Check if we're backfacing
		if ( vec3::dot_product(Normal[Index],d)<(Mesh.GetMaxAngle(Index)-0.02) )
			continue;

		// Compute the coverage
		const CTexgenBase& SourceBase = NormalizedBase[Index];
		float Coverage = MipmapBias*GetElementCoverage( Coord[Index], SourceBase, ScreenMatrix,
			DisplayData.Frustum[4] );

		// Select a mipmap level
		using replay::math::clamp;
		uint PixelSize = clamp<uint>( uint(Coverage), 1, TileSize );
		uint SelectedLevel = std::min( TopLevel-IntegerLog(PixelSize), uint(MaxLevel) );
		
		PageAtlas->SetupLevel(Current,SelectedLevel,Target);
	}

	const int CacheStats = PageAtlas->UpdateCache();

	if ( CacheStats > int(PageAtlas->GetPageCountSqrt()) )
	{
		this->MipmapBias = std::min( this->MipmapBias+0.025f, 1.0f );
	}
	else if ( CacheStats < 0 )
	{
		this->MipmapBias = std::max( 0.1f, this->MipmapBias-0.025f );
	}
}

void CFantexRenderer::UpdatePages( const CDisplayData& DisplayData, const CSphere* Sphere )
{
	OffsetBuffer.SetGeneratedSubData( GL_ARRAY_BUFFER, 0, sizeof(vec3)*Mesh.GetVertexCount(),
		boost::bind( &CFantexRenderer::UpdateOffsets, this, _1, boost::cref( DisplayData ), Sphere ) );

	GLmm::BufferObject::Unbind(GL_ARRAY_BUFFER);
}

bool CFantexRenderer::DrawAt(const CSphere& Cursor, const vec4& Color)
{
	std::vector<std::size_t> Affected;

	for ( std::size_t Index=0; Index<Mesh.GetVertexCount(); ++Index )
	{
		const CSphere& Sphere( Mesh.GetFanBounds(Index) );

		// Check whether it's in drawing range
		if ( distance(Cursor.Center,Sphere.Center)>(Cursor.Radius+Sphere.Radius) )
			continue;

		Affected.push_back(Index);
	}

	float Smoothness = 0.8f;

	// Update high detail tiles
	BOOST_FOREACH( const std::size_t& Index, Affected )
	{
		EditProgram.Use();
		glUniform4fv( EditProgram.GetUniformLocation("BrushColor"), 1,
			Color.ptr() );
		glUniform2fv( EditProgram.GetUniformLocation("BrushRadii"), 1,
			vec2( Smoothness*Cursor.Radius, Cursor.Radius ).ptr() );
		glUniform3fv( EditProgram.GetUniformLocation("BrushPosition"), 1,
			Cursor.Center.ptr() );

		glActiveTexture( GL_TEXTURE0 );
		PageAtlas->BindTexture();
		glUniform1i( EditProgram.GetUniformLocation( "CacheTexture" ), 0 );

		Fbo.Bind();
		glViewport( 0, 0, PageAtlas->GetTileSize(), PageAtlas->GetTileSize() );

		CPageAtlas::CPageEntry* Page = PageAtlas->FindPage(page_id(0,Index),true);
		vec3 Center = Mesh.GetCoordVector()[Index];
		vec3 OffsetAndScale = PageAtlas->ComputeOffsetAndScale(Index,Page);
		glUniform4fv( EditProgram.GetUniformLocation("Center"), 1, vec4(Center,0.f).ptr() );
		glUniform3fv( EditProgram.GetUniformLocation("OffsetAndScale"), 1, OffsetAndScale.ptr() );

		FanDrawer.Draw(Index);
		PageAtlas->ReplaceContent(Page,Fbo);
	}

	GLmm::Framebuffer::Unbind();

	return true;
}
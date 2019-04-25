
#include "ShadowmapRenderer.hpp"
#include "GLSLUtils.hpp"
#include <replay/vector_math.hpp>
#include "GLmm/GL.hpp"

namespace {
	const replay::matrix4 ClipspaceToTexturespaceMatrix(
				0.5f, 0.0f, 0.0f, 0.5f,
				0.0f, 0.5f, 0.0f, 0.5f,
				0.0f, 0.0f, 0.5f, 0.5f,
				0.0f, 0.0f, 0.0f, 1.0f );
};

CShadowmapRenderer::CShadowmapRenderer( const CFantexMesh& Mesh, const CDirectionalLight& Light, uint TextureSize )
: Light( Light )
{
	// Copy basic triangle mesh data
	VertexBuffer.SetData( GL_ARRAY_BUFFER, Mesh.GetCoordVector(), GL_STATIC_DRAW );
	VertexCount = Mesh.GetCoordVector().size();

	IndexBuffer.SetData( GL_ELEMENT_ARRAY_BUFFER, Mesh.GetIndexVector(), GL_STATIC_DRAW );
	IndexCount = Mesh.GetIndexCount();

	// Setup the shadow map
	DepthTexture.SetImage(0, GL_DEPTH_COMPONENT, TextureSize, TextureSize );
	DepthTexture.SetWrap(GL_CLAMP_TO_EDGE);
	DepthTexture.SetFilter(GL_NEAREST,GL_LINEAR);
	DepthTexture.SetCompareMode( GL_COMPARE_R_TO_TEXTURE );
	DepthTexture.SetCompareFunc( GL_LESS );

	// Setup the rendering target
	Framebuffer.Bind();
	glReadBuffer( GL_NONE );
	glDrawBuffer( GL_NONE );
	Framebuffer.Attach( GL_DEPTH_ATTACHMENT, DepthTexture, 0 );
	Framebuffer.TestCompleteness();
	Framebuffer.Unbind();
	ShadowTextureSize = TextureSize;

	// Load the shader
	CompileShader( "Shadow.glsl", Program );
	glBindAttribLocation(Program.GetGLObject(),0,"Coord");
	Program.Link();

}

void CShadowmapRenderer::Render( const CSphere& Bounds )
{
	// Build an orthogonal view centered around the bounds.
	float is = 1.f/Bounds.Radius;
	float r = is;
	vec3 C = Light.GetViewMatrix()*Bounds.Center*r;
	ShadowMatrix.set(
		  r, 0.f, 0.f, -C[0],
		0.f,   r, 0.f, -C[1],
		0.f, 0.f,   r, -C[2],
		0.f, 0.f, 0.f, 1.f );

	//ShadowMatrix = replay::math::make_orthographic_matrix( Range, Range, Range ); 
	ShadowMatrix = ShadowMatrix * Light.GetViewMatrix();

	// Setup the viewport
	Framebuffer.Bind();
	glViewport( 0, 0, ShadowTextureSize, ShadowTextureSize );
	glClear( GL_DEPTH_BUFFER_BIT );

	Program.Use();
	glUniformMatrix4fv( Program.GetUniformLocation("Matrix"),1,GL_FALSE,ShadowMatrix.ptr());

	// Setup the mesh source
	const int CoordAttrib = Program.GetAttribLocation( "Coord" );
	
	glPolygonOffset( 2.0f, 8.0f );
	//glEnable( GL_POLYGON_OFFSET_FILL );
	VertexBuffer.Bind(GL_ARRAY_BUFFER);
	glEnableVertexAttribArray( CoordAttrib );
	glVertexAttribPointer( CoordAttrib, 3, GL_FLOAT, GL_FALSE,
		sizeof(vec3), 0 );

	// Draw
	IndexBuffer.Bind(GL_ELEMENT_ARRAY_BUFFER);
	glDrawRangeElements(GL_TRIANGLES,0,VertexCount-1,IndexCount,GL_UNSIGNED_INT,0);
	
	glDisable( GL_POLYGON_OFFSET_FILL );
	// Disable
	glDisableVertexAttribArray( CoordAttrib );

	ShadowMatrix = ClipspaceToTexturespaceMatrix*ShadowMatrix;
}

const replay::matrix4& CShadowmapRenderer::GetShadowMatrix() const
{
	return ShadowMatrix;
}

const GLmm::Texture2D& CShadowmapRenderer::GetShadowTexture() const
{
	return DepthTexture;
}

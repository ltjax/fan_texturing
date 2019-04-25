
#ifndef FANTEX_RENDERER_HPP
#define FANTEX_RENDERER_HPP

#include "FantexMesh.hpp"
#include "GLmm/Framebuffer.hpp"
#include "GLmm/Program.hpp"
#include "FanDrawer.hpp"

class CPageAtlas;

/** Manages on-GPU mesh data and rendering logic for a fan-texturing mesh.
*/
class CFantexRenderer
{
public:
	CFantexRenderer( CFantexMesh& Mesh, const boost::filesystem::path& PageFile, uint PageNumSqrt );

#ifdef RUNTIME_SHADOWS
	void						Render( GLmm::Texture2DArray& ShadowTexture, const matrix4& ShadowMatrix );
#else
	void						Render();
#endif
	
	
	void						RenderDepth( GLmm::Program& Program );

	void						SetBias( float Bias );
	float						GetBias() const;

	void						UpdatePages( const CDisplayData& DisplayData, const CSphere* Sphere );

	bool						GetEditable() const;
	void						SetEditable( bool Rhs );

	bool						DrawAt( const CSphere& Cursor, const vec4& Color );

	bool						GetSmoothBlend() const;
	void						SetSmoothBlend( bool Rhs );


private:
	
	void						UpdateOffsets( GLubyte* Target,
									const CDisplayData& DisplayData,
									const CSphere* Sphere );

	void						CopyVertexData( GLubyte* Target );

	CFantexMesh&				Mesh;
	scoped_ptr<CPageAtlas>		PageAtlas;
	
	GLmm::Program				Program;

	GLmm::BufferObject			VertexBuffer;
	GLmm::BufferObject			IndexBuffer;
	GLmm::BufferObject			OffsetBuffer;

	float						MipmapBias;

	GLmm::Framebuffer			Fbo;
	GLmm::Renderbuffer			Rbo;
	GLmm::Program				EditProgram;

	CFanDrawer					FanDrawer;

	bool						SmoothBlend;
};

#endif // FANTEX_RENDERER_HPP

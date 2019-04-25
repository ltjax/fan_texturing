
#ifndef WIREFRAME_RENDERER_HPP
#define WIREFRAME_RENDERER_HPP

#include "Common.hpp"
#include "FantexMesh.hpp"
#include <replay/buffer.hpp>
#include "FanDrawer.hpp"
#include "Misc.hpp"
#include "GLmm/Framebuffer.hpp"

/** Renderer that is exclusive to rendering shadow map geometry.
*/
class CWireframeRenderer :
	public boost::noncopyable
{
public:
	CWireframeRenderer( const CFantexMesh& Mesh );
	~CWireframeRenderer();

	void							Render( const matrix4& View, const matrix4& Proj );
	
	void							SetRenderTiles( bool Rhs );
	bool							GetRenderTiles() const;

	void							SetOverlay( bool Rhs );
	bool							GetOverlay() const;

private:
	GLmm::BufferObject				VertexBuffer;
	size_t							ModelCount;
	size_t							VertexCount;
	size_t							NormalCount;
	size_t							TileVertexCount;

	GLmm::Program					Program;
	GLmm::Program					NormalProgram;
	GLmm::Program					TilesProgram;

	bool							RenderTiles;
	bool							Overlay;
};

#endif // WIREFRAME_RENDERER_HPP

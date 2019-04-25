
#ifndef SHADOWMAP_RENDERER_HPP
#define SHADOWMAP_RENDERER_HPP

#include "Common.hpp"
#include "FantexMesh.hpp"
#include <replay/buffer.hpp>
#include "FanDrawer.hpp"
#include "Misc.hpp"
#include "GLmm/Framebuffer.hpp"

/** Renderer that is exclusive to rendering shadow map geometry.
*/
class CShadowmapRenderer :
	public boost::noncopyable
{
public:
	CShadowmapRenderer( const CFantexMesh& Mesh, const CDirectionalLight& Light, uint ShadowmapSize );

	void							Render( const CSphere& Bounds );

	const matrix4&					GetShadowMatrix() const;
	const GLmm::Texture2D&			GetShadowTexture() const;

private:
	const CDirectionalLight&		Light;

	GLmm::Texture2D					DepthTexture;
	GLmm::Framebuffer				Framebuffer;

	GLmm::BufferObject				VertexBuffer;
	size_t							VertexCount;

	GLmm::BufferObject				IndexBuffer;
	size_t							IndexCount;

	GLmm::Program					Program;

	matrix4							ShadowMatrix;
	size_t							ShadowTextureSize;
};

#endif

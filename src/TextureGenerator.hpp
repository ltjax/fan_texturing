
#ifndef TEXTURE_GENERATOR_HPP
#define TEXTURE_GENERATOR_HPP

#include "FantexMesh.hpp"
#include "ShadowmapRenderer.hpp"

class CTextureGenerator :
	public CAbstractTextureGenerator
{
public:
						CTextureGenerator( const CFantexMesh& Mesh, const CDirectionalLight& Light );
						~CTextureGenerator();

	virtual void		Bind( const vec3& Center, const CSphere& Bounds );

private:
	CShadowmapRenderer	ShadowmapRenderer;

	void				SetupTextures();
	void				SetupGLSLShader();

	GLmm::Program		GenerateProgram;

	GLmm::Texture2D		SimplexNoiseTexture;
	GLmm::Texture2D		RockTexture;
	GLmm::Texture2D		GrassTexture;
	GLmm::Texture2D		SandTexture;
};

#endif

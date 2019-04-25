
#ifndef SHADER_TEST_KERNEL_HPP
#define SHADER_TEST_KERNEL_HPP

#include <glsk/glsk.hpp>
#include "GLmm/BufferObject.hpp"
#include "GLmm/Program.hpp"
#include "GLmm/Texture.hpp"
#include "FileMonitor.hpp"

class CShaderTestKernel
{
public:
	CShaderTestKernel();
	~CShaderTestKernel();

	void			OnResize( int w, int h );
	void			OnMouse( const glsk::mouse_event& msg );
	void			OnInput( const glsk::input_event& msg );
	void			OnFileChange( const std::vector<boost::filesystem::path>& Paths );

	void			OnIdle();

	void			UpdateShader();


private:

	bool			RenderingEnabled;

	int				Width;
	int				Height;

	GLmm::BufferObject	Quad;
	GLmm::Program		Prog;
	GLmm::Texture2D		NoiseTexture;

	CFileMonitor	FileMonitor;
};

#endif // SHADER_TEST_KERNEL_HPP

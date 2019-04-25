
#ifndef APPLICATION_KERNEL_HPP
#define APPLICATION_KERNEL_HPP

#include <glsk/glsk.hpp>
#include <replay/vector_math.hpp>
#include "UserCamera.hpp"
#include "FantexMesh.hpp"
#include "FantexRenderer.hpp"
#include "WireframeRenderer.hpp"
#include "GLmm/Framebuffer.hpp"
#include "Cursor.hpp"
#include "Misc.hpp"



class CApplicationKernel
{
public:
							CApplicationKernel(glsk::window& Wnd,const boost::filesystem::path& Filename );
							~CApplicationKernel();

	void					OnResize( int w, int h );
	void					OnMouse( const glsk::mouse_event& msg );
	void					OnInput( const glsk::input_event& msg );

	void					OnIdle();


private:
	glsk::window&			Window;

	void					OnPrintScreen();
	void					OnSelectColor();

	void					LoadModel( const boost::filesystem::path& Filename,
									std::vector<vec3>& Vertices,
									std::vector<uint>& Indices );

	int						Width;
	int						Height;

	float					MouseX;
	float					MouseY;
	float					CacheScale;

	CUserCamera				Camera;
	CFantexMesh				Mesh;

	boost::scoped_ptr<CFantexRenderer>
							Renderer;

	boost::scoped_ptr<CWireframeRenderer>
							WireframeRenderer;

	matrix4					Projection;


	CDirectionalLight		Light;

	vec3					PointerDir;

	bool					DrawCache;

	GLmm::Program			DrawCacheProgram;
	GLmm::BufferObject		QuadBuffer;

	CCursor					Cursor;

	bool					DoDraw;

	bool					ShowWireframe;

#ifdef RUNTIME_SHADOWS
	GLmm::Framebuffer		ShadowFramebuffer;
	GLmm::Texture2DArray	ShadowTexture;
	GLmm::Program			ShadowProgram;
#endif
};

#endif // APPLICATION_KERNEL_HPP

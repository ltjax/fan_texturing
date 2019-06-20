
#pragma once

#include <replay/vector_math.hpp>
#include "UserCamera.hpp"
#include "FantexMesh.hpp"
#include "FantexRenderer.hpp"
#include "WireframeRenderer.hpp"
#include "GLmm/Framebuffer.hpp"
#include "Cursor.hpp"
#include "Misc.hpp"
#include <SDL2/SDL.h>


class CApplicationKernel
{
public:
							CApplicationKernel(SDL_Window* Wnd, const boost::filesystem::path& Filename );
							~CApplicationKernel();

	void					OnResize( int w, int h );
	void					OnMouseMove(SDL_MouseMotionEvent* msg);
    void                    OnMouseButton(SDL_MouseButtonEvent* msg);
	void					OnKey(SDL_KeyboardEvent msg );

	void					OnIdle();


private:
    SDL_Window* 			Window;

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


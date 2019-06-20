
#ifndef RENDERWINDOW_HPP
#define RENDERWINDOW_HPP

#include <GL/glew.h>
#include <replay/affinity.hpp>
#include <replay/matrix4.hpp>
#include <replay/byte_color.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/scoped_ptr.hpp>
#include "GLmm/GL.hpp"
#include <SDL2/SDL.h>

using replay::vector2i;
using replay::vector2f;
using replay::vector3f;
using replay::affinity;
using replay::matrix4;

template <class T>
class CRenderWindow
{
public:
	void OnCreate( SDL_Window* Window, int w, int h, boost::filesystem::path Filename )
	{
		const char *GLVersionString = (const char*)glGetString(GL_VERSION);
		GLMM_CHECK();

		GLenum GlewError = glewInit();

		if ( GlewError != GLEW_NO_ERROR )
			throw std::runtime_error( "Glew init error!" );

		std::cout << "GLEW initialized..." << std::endl;
		GLMM_CHECK();

		Kernel.reset( new T(Window,Filename) );
		Kernel->OnResize(w,h);
	}

	void OnDestroy()
	{
		Kernel.reset();
	}

	CRenderWindow( boost::filesystem::path Filename ) : Filename(Filename)
	{
	}

    bool PumpMessages()
    {
        SDL_Event SDLEvent;
        while (SDL_PollEvent(&SDLEvent))
        {
            switch (SDLEvent.type)
            {
                case SDL_QUIT:
                    return false;

                case SDL_KEYDOWN:
                    Kernel->OnKey(&SDLEvent.key);
                    break;

                case SDL_KEYUP:
                    Kernel->OnKey(&SDLEvent.key);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    Kernel->OnMouseButton(&SDLEvent.button);
                    break;

                case SDL_MOUSEBUTTONUP:
                    Kernel->OnMouseButton(&SDLEvent.button);
                    break;

                case SDL_MOUSEMOTION:
                    Kernel->OnMouseMove(&SDLEvent.motion);
                    break;
            }

        }
        return true;
    }

	void Run( int w, int h, const std::string& Title )
	{
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
        {
            throw std::runtime_error(std::string("Unable to initialize SDL: ") + SDL_GetError());
        }

        atexit(SDL_Quit);

        SDL_Window* Window = nullptr;
        SDL_GLContext Context = nullptr;
        
        // Create window
        Window = SDL_CreateWindow(Title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);

        Context = SDL_GL_CreateContext(Window);


        OnCreate(Window, w, h, Filename);

        // Most of the loading is done here, so show the window
        SDL_ShowWindow(Window);

		while (PumpMessages())
		{
			Kernel->OnIdle();
            SDL_GL_SwapWindow(Window);
		}

        Kernel.reset();
	}

private:
	std::unique_ptr<T>      Kernel;
    boost::filesystem::path Filename;
};


#endif

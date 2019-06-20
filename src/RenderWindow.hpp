
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
using boost::scoped_ptr;

template <class T>
class CRenderWindow
{
public:
	void OnCreate( SDL_Window* Window, int w, int h, boost::filesystem::path Filename )
	{
		// Select the rendering contenxt and init GLEW
		//select_rc();

		const char *GLVersionString = (const char*)glGetString(GL_VERSION);
		GLMM_CHECK();

		GLenum GlewError = glewInit();

		if ( GlewError != GLEW_NO_ERROR )
			throw std::runtime_error( "Glew init error!" );

		std::cout << "GLEW initialized..." << std::endl;
		GLMM_CHECK();

		//try {

			// Create and init the kernel
			Kernel.reset( new T(Window,Filename) );
			Kernel->OnResize(w,h);

			// Bind signals to the kernel
			//signal_configure().connect( boost::bind( &T::OnResize, Kernel.get(), _1, _2 ) );
			//signal_mouse().connect( boost::bind( &T::OnMouse, Kernel.get(), _1 ) );
			//signal_input().connect( boost::bind( &T::OnInput, Kernel.get(), _1 ) );
		//}
		//catch( std::runtime_error& x )
		//{
		//	glsk::error_box( x.what() );
		//	throw glsk::unsuccessful();
		//}
		//catch( ... )
		//{
		//	glsk::error_box( "Unknown error during initialization!" );
		//	throw glsk::unsuccessful();
		//}
	}

	void OnDestroy()
	{
		//signal_configure().disconnect_all_slots();
		//signal_mouse().disconnect_all_slots();
		//signal_input().disconnect_all_slots();

		Kernel.reset();
	}

	CRenderWindow( boost::filesystem::path Filename ) : Filename(Filename)
	{
		typedef CRenderWindow<T> SelfType;
		
		// bind basic signals;
		//signal_create().connect( boost::bind( &SelfType::OnCreate, this, _1, _2, Filename ) );
		//signal_destroy().connect( boost::bind( &SelfType::OnDestroy, this ) );
		//signal_close().connect( &glsk::main::quit );
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
	}

private:
	scoped_ptr<T>	Kernel;
    boost::filesystem::path Filename;
};


#endif

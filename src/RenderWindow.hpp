
#ifndef RENDERWINDOW_HPP
#define RENDERWINDOW_HPP

#include <glsk/glsk.hpp>
#include <GL/glew.h>
#include <replay/affinity.hpp>
#include <replay/matrix4.hpp>
#include <replay/byte_color.hpp>
#include <boost/filesystem/path.hpp>
#include "GLmm/GL.hpp"
using replay::vector2i;
using replay::vector2f;
using replay::vector3f;
using replay::affinity;
using replay::matrix4;
using boost::scoped_ptr;

template <class T>
class CRenderWindow :
	public glsk::window
{
public:
	void OnCreate( int w, int h, boost::filesystem::path Filename )
	{
		// Select the rendering contenxt and init GLEW
		select_rc();

		const char *GLVersionString = (const char*)glGetString(GL_VERSION);
		GLMM_CHECK();

		GLenum GlewError = glewInit();

		if ( GlewError != GLEW_NO_ERROR )
			throw std::runtime_error( "Glew init error!" );

		std::cout << "GLEW initialized..." << std::endl;
		GLMM_CHECK();

		try {

			// Create and init the kernel
			Kernel.reset( new T(*this,Filename) );
			Kernel->OnResize(w,h);

			// Bind signals to the kernel
			signal_configure().connect( boost::bind( &T::OnResize, Kernel.get(), _1, _2 ) );
			signal_mouse().connect( boost::bind( &T::OnMouse, Kernel.get(), _1 ) );
			signal_input().connect( boost::bind( &T::OnInput, Kernel.get(), _1 ) );
		}
		catch( std::runtime_error& x )
		{
			glsk::error_box( x.what() );
			throw glsk::unsuccessful();
		}
		catch( ... )
		{
			glsk::error_box( "Unknown error during initialization!" );
			throw glsk::unsuccessful();
		}
	}

	void OnDestroy()
	{
		signal_configure().disconnect_all_slots();
		signal_mouse().disconnect_all_slots();
		signal_input().disconnect_all_slots();

		Kernel.reset();
	}

	CRenderWindow( glsk::config Config, boost::filesystem::path Filename ) : glsk::window(Config)
	{
		typedef CRenderWindow<T> SelfType;
		
		// bind basic signals;
		signal_create().connect( boost::bind( &SelfType::OnCreate, this, _1, _2, Filename ) );
		signal_destroy().connect( boost::bind( &SelfType::OnDestroy, this ) );
		signal_close().connect( &glsk::main::quit );
	}

	void Run( const std::string& Title )
	{
		set_decorated( true );
		set_title( Title );

		show();
		open();

		while ( glsk::main::pump_messages() )
		{
			Kernel->OnIdle();
			swap_buffers();
		}
	}

private:
	scoped_ptr<T>	Kernel;
};


#endif

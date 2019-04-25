/** OpenGL 2 API Header
*/

#ifndef GLMM_GL_HPP
#define GLMM_GL_HPP

#include <boost/function.hpp>
#include <sstream>
#include <stdexcept>
#include <GL/glew.h>


/** OpenGL 2 API
*/
namespace GLmm {

	/** Check for OpenGL errors.
		If there are errors, the function will send an std::runtime_error with the errorcodes.
	*/
	void						CheckGLErrorsAt( const char* File, const unsigned int Line );

}

#ifdef _DEBUG
#	define GLMM_CHECK() GLmm::CheckGLErrorsAt( __FILE__, __LINE__ )
#else
#	define GLMM_CHECK()
#endif

/** Macro to easiely throw errors.
*/
#define GLMM_THROW_ERROR( ARGS ) \
	do { \
	std::ostringstream Str; \
	Str << ARGS; \
	throw std::runtime_error( Str.str() ); \
	} while ( false )

#endif


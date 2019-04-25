
#ifndef GEOMETRY_IMPORT_OBJ
#define GEOMETRY_IMPORT_OBJ

#include <string>
#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>
#include "Common.hpp"


namespace WavefrontObj {
	
	struct CModelGroup
	{
		vec3*			VertexBuffer;
		vec3*			NormalBuffer;
		vec2*			TexcoordBuffer;
		std::size_t		VertexCount;

		unsigned short*	IndexBuffer;
		std::size_t		IndexCount;
	};

	typedef boost::function< void ( const std::string&, CModelGroup ) > CallbackType;

	void				Import( const boost::filesystem::path& Path, CallbackType Callback );
}

#endif


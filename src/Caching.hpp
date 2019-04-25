/*

Cataclysm-Software Disaster Engine
----------------------------------

Repository-Info:
$Id: Caching.hpp 564 2007-11-19 15:54:42Z ltjax $

Copyright:
Marius Elvert (marius.elvert@cataclysm-software.com) 2006-2007

*/

#ifndef GEOMETRY_CACHING_HEADER
#define GEOMETRY_CACHING_HEADER

#include <memory>

namespace Caching {

	typedef unsigned short uint16;

	// for optimizing post-vertexcache access order
	void OptimizeTriangleIndicesForCache(
		uint16* IndexArray,
		std::size_t IndexCount,
		std::size_t VertexCount );

	// for optimizing pre-vertexcache access order
	void BuildVertexOrderTranslationTable( 
		const uint16* IndexArray, std::size_t IndexCount,
		uint16* TranslationTable, std::size_t VertexCount );

	// test post-vertexcache misses
	std::size_t CountMisses( 
		const uint16* IndexArray, std::size_t IndexCount,
		std::size_t CacheSize, bool UseLRU );
};

#endif


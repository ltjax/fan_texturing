
#ifndef FANTEX_MESH_HPP
#define FANTEX_MESH_HPP

#include "Common.hpp"

#include <vector>
#include <replay/vector2.hpp>
#include <replay/vector3.hpp>
#include <replay/vector4.hpp>
#include <replay/matrix4.hpp>
#include <replay/lines3.hpp>
#include <replay/table.hpp>
#include "GLmm/Program.hpp"
#include "GLmm/Texture.hpp"
#include "GLmm/BufferObject.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem/fstream.hpp>
#include "Index.hpp"



class CDisplayData;
class CTexgenBase;

/** Base class for texture generators.
	This has to define and setup a GLSL shader that is called
	in the generation process.
*/
class CAbstractTextureGenerator :
	public boost::noncopyable
{
public:
	CAbstractTextureGenerator() {}
	virtual ~CAbstractTextureGenerator() {}

	virtual void Bind( const vec3& Center, const CSphere& Bounds ) = 0;

};


/** Handle the fan-textured mesh.
*/
class CFantexMesh :
	public GLmm::Object
{
public:
	typedef std::size_t size_type;


	void						Init( std::vector<vec3>& Vertices,
									  const std::vector<uint>& Triangles );

	bool						Trace( const replay::ray3& Ray, vec3& Result ) const;

	const std::vector<vec3>&	GetCoordVector() const { return Coord; }
	const std::vector<vec3>&	GetNormalVector() const { return Normal; }
	const std::vector<CTexgenBase>& GetBaseVector() const { return NormalizedBase; }

	const std::vector<uint>&	GetIndexVector() const { return Triangles; }

	size_type					GetVertexCount() const { return Coord.size(); }
	size_type					GetIndexCount() const { return Triangles.size(); }

	inline 
	const CSphere&				GetFanBounds( size_type VertexIndex ) const { return FanBounds[VertexIndex]; }

	inline
	float						GetMaxAngle( size_type VertexIndex ) const { return Fandata[VertexIndex].MinCos; }
	void						GetTriangle( size_type VertexIndex, size_type TriangleIndex, uint* Triangle ) const;
	size_type					GetTriangleCount( size_type VertexIndex ) const;

								CFantexMesh();
								~CFantexMesh();


	bool						BuildEdgePolygons( size_type Index, std::vector<vec3>& Coord,
										std::vector<vec2>& ProjectedCoord ) const;
private:


	void						ComputeCullCone( size_type VertexIndex );

	struct CFandata
	{
		std::vector<std::size_t> Triangles;
		float MinCos;

		CFandata() : MinCos(1.f) {}
	};

	std::vector<CSphere>		FanBounds;

	std::vector<CFandata>		Fandata;
	std::vector<CTexgenBase>	NormalizedBase;

	std::vector<vec3>			Coord;
	std::vector<vec3>			Normal;

	std::vector<uint>			Triangles;


	void						SetupNormals();
	void						BuildBases();
	void						BuildFans();
	void						NormalizeRanges();

};


#endif


#include <iterator>
#include "FantexMesh.hpp"
#include <boost/assign.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/math/constants/constants.hpp>
#include <sstream>
#include <replay/vector_math.hpp>
#include <replay/pixbuf.hpp>
#include <replay/bstream.hpp>
#include <replay/buffer.hpp>
#include <replay/matrix2.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/bind.hpp>
#include "GLmm/Texture.hpp"
#include "GLmm/Framebuffer.hpp"
#include "GLmm/GL.hpp"
#include "PageAtlas.hpp"
#include "Misc.hpp"


namespace {


/** Add a triangles normal to all adjacent points.
	The triangle is given by indices into a point vector.
*/
void AddTriangleNormal( const std::vector<vec3>& Points,
					    std::vector<vec3>& Normals,
						uint a, uint b, uint c )
{
	vec3 ec=Points[b]-Points[a];
	vec3 eb=Points[a]-Points[c];
	vec3 ea=Points[c]-Points[b];

	vec3 Normal = cross( ec, -eb );

	const float Length = magnitude(Normal);

	if ( Length > 0.f )
	{
		normalize(ec);
		normalize(eb);
		normalize(ea);

		float Alpha = std::cos(-dot(ec,eb));
		float Beta = std::cos(-dot(ec,ea));
		float Gamma = std::cos(-dot(eb,ea));

		Normal /= Length;

		Normals[a] += Normal*Alpha;
		Normals[b] += Normal*Beta;
		Normals[c] += Normal*Gamma;
	}
}


struct FirstEquals
{
	FirstEquals(uint i):x(i) {}
	bool operator()(const std::pair<uint,uint>& rhs) const { return x==rhs.first;}
private:
	uint x;
};

template < class IteratorType >
inline void SortIndexPairList( IteratorType begin, IteratorType end )
{
	for ( IteratorType i=begin; (i+1)!= end; ++i )
	{
		IteratorType x=std::find_if(i+1,end,FirstEquals(i->second));

		if ( x!=end )
			std::swap( *(i+1), *x );
		else
		{
			std::size_t TailSize = end-i-1;

			// move the end to the front and sort it
			std::rotate( begin, i+1, end );
			
			SortIndexPairList( begin, begin+TailSize );

			return;
		}
	}
}

bool FindNeighbourPolygon( std::vector<std::pair<uint,uint>>& Edges, std::vector<uint>& Polygon )
{
	bool Result=true;
	Polygon.clear();
	SortIndexPairList(Edges.begin(),Edges.end());

	if ( Edges.back().second == Edges.front().first )
	{
		Polygon.reserve(Edges.size());
	}
	else
	{
		Polygon.reserve(Edges.size()+1);
		Polygon.push_back(Edges.front().first);
		Result=false;
	}

	for ( std::size_t i=0; i<Edges.size(); ++i )
		Polygon.push_back(Edges[i].second);

	return Result;
}

}

CFantexMesh::CFantexMesh()
{
}

CFantexMesh::~CFantexMesh()
{
}



bool CFantexMesh::Trace( const replay::ray3& Ray, vec3& Result ) const
{
	using namespace replay::math::intersection_test;

	float Closest=std::numeric_limits<float>::max();
	fcouple Range;
	uint Triangle[3];

	for ( std::size_t Vertex=0,EndVertex=Fandata.size(); Vertex<EndVertex; ++Vertex )
	{
		const CFandata& Fan(Fandata[Vertex]);
		const CSphere& Sphere( GetFanBounds(Vertex) );

		// Does the line that embedds the ray miss the bounding sphere?
		if ( !line_sphere( Ray, Sphere.Center, Sphere.Radius, Range.ptr(), Range.ptr()+1 ) )
			continue;

		// What about the ray itself?
		if ( Range[1] < 0.f )
			continue;

		// Do we have earlier results?
		if ( Range[0] > Closest )
			continue;

		for ( std::size_t i=0,ei=GetTriangleCount(Vertex);i<ei;++i )
		{
			GetTriangle(Vertex,i,Triangle);

			// Check whether we hit that triangle
			if ( !line_triangle( Ray, Coord[Triangle[0]],Coord[Triangle[1]],Coord[Triangle[2]], Range.ptr() ) )
				continue;

			// Hit the back
			if ( Range[0] < 0.f )
				continue;

			// We already got a closer one
			if ( Range[0] > Closest )
				continue;

			// New closest hit
			Closest = Range[0]; 
		}
	}

	if ( Closest == std::numeric_limits<float>::max() )
		return false;

	Result = Ray.get_point(Closest);

	return true;
}


/** Compute normals
*/
void CFantexMesh::SetupNormals()
{
	// Add normals from the newly generated triangles
	for ( std::size_t j=0; j<Triangles.size(); j+=3 )
		AddTriangleNormal( Coord, Normal, Triangles[j],Triangles[j+1],Triangles[j+2] );

}


void CFantexMesh::BuildBases()
{
	NormalizedBase.resize(Coord.size());

	// build projection bases for texture coordinates in each fan center
	for ( std::size_t i=0,e=Normal.size(); i<e; ++i )
	{
		vec3& N=Normal[i];
		normalize( N );

		vec3 T = replay::math::construct_perpendicular(N);		
		normalize( T );

		vec3 B = cross(N,T);

		NormalizedBase[i].u = vec4( T, 0.f );
		NormalizedBase[i].v = vec4( B, 0.f );		
	}
}

void CFantexMesh::BuildFans()
{
	Fandata.clear();
	Fandata.resize( Coord.size() );

	for ( std::size_t j=0; j<Triangles.size(); j+=3 )
	{	
		vec2 ProjTriangle[3];

		for ( std::size_t k=0; k<3; ++k )
		{
			uint Index = Triangles[j+k];

			Fandata[Index].Triangles.push_back( uint(j/3) ); // add the triangle index

			const vec4& Ubase = NormalizedBase[Index].u;
			const vec4& Vbase = NormalizedBase[Index].v;

			for ( std::size_t l=0; l<3; ++l )
			{
				// Use cutoff for non-center vertices
				vec3 Point = Coord[Triangles[j+l]];

				// Project the point and insert into the range
				ProjTriangle[l] = Project( vec4(Point,1.f), Ubase, Vbase );
			}

			const float TextureArea = replay::math::det(
				ProjTriangle[1]-ProjTriangle[0],
				ProjTriangle[2]-ProjTriangle[0] )*0.5f;

			if ( TextureArea < 0.f )
			{
				std::cout << "WARNING: Triangle #" << (j/3) << " flipped in fan #" << Index
					<< "." << std::endl; 
			}
		}
	}
}

bool CFantexMesh::BuildEdgePolygons( size_type Index, std::vector<vec3>& Points,
									 std::vector<vec2>& ProjectedPoints ) const
{
	// Constants for the seam
	const float		SeamWidth = 0.01f;
	const float		CenterP = 2.f*SeamWidth + (1.f/3.f);
	const float		SideP = 3.f*SeamWidth + (1.f/2.f);

	std::vector<std::pair<uint,uint>> EdgeBuffer;
	std::vector<uint> Polygon;
	Points.clear();
	ProjectedPoints.clear();
	const CTexgenBase& Base = NormalizedBase[Index];

	// Build a list of (index) edges on the polygon
	uint Triangle[3];
	for ( std::size_t i=0, ei=GetTriangleCount(Index); i<ei; ++i )
	{
		GetTriangle(Index,i,Triangle);
		EdgeBuffer.push_back(std::make_pair(Triangle[1],Triangle[2]));
	}

	// Order the edgebuffer and simplify
	bool PolygonClosed = FindNeighbourPolygon( EdgeBuffer, Polygon );


	// Generate the cut-off polygons in local space
	ProjectedPoints.push_back( vec2(0.0f) );
	Points.push_back( vec3(0.f) );

	vec3 db3=Coord[Polygon[0]]-Coord[Index];
	vec2 db=Project( vec4(db3,1.f), Base.u, Base.v );
	for ( std::size_t j=0, ej=Polygon.size(); j<ej; ++j )
	{
		std::size_t jpp = (j+1)%ej;

		const vec3 dc3=Coord[Polygon[jpp]]-Coord[Index];
		const vec2 dc=Project( vec4(dc3,1.f), Base.u, Base.v );

		ProjectedPoints.push_back( SideP*db );
		Points.push_back(db3*SideP);

		if ( PolygonClosed || j+1<Polygon.size() )
		{
			ProjectedPoints.push_back( CenterP*db+CenterP*dc );
			Points.push_back(CenterP*db3+CenterP*dc3);
		}

		db=dc;
		db3=dc3;
	}

	return PolygonClosed;
}

void CFantexMesh::ComputeCullCone( size_type VertexIndex )
{
	float& MinCos(Fandata[VertexIndex].MinCos);
	MinCos = 0.f;

	for ( std::size_t i=0,ei=GetTriangleCount(VertexIndex); i<ei; ++i )
	{
		uint Triangle[3];
		GetTriangle( VertexIndex,i, Triangle );

		const vec3& n = Normal[VertexIndex];

		vec3 a=Coord[Triangle[1]]-Coord[VertexIndex];
		vec3 b=Coord[Triangle[2]]-Coord[VertexIndex];
		vec3 d = cross(a,b);
		normalize(d);

		float fd = std::cos(std::acos(dot(d,n))+boost::math::constants::half_pi<float>());
		MinCos = std::min( MinCos, fd );
	}	
}

void CFantexMesh::NormalizeRanges()
{
	FanBounds.resize(Coord.size());

	// squish the projection into the [0..1]^2 interval
	for ( std::size_t i=0; i<Coord.size(); ++i )
	{
		std::vector<vec2> ProjectedPoints;
		std::vector<vec3> Points;
		BuildEdgePolygons(i,Points,ProjectedPoints);

		vec2 MinRange, MaxRange;
		float SqrRadius;

		ComputeCullCone(i);

		/*float& MinCos(Fandata[i].MinCos);
		MinCos = 0.f;
		for ( std::size_t j=1; j<Points.size(); ++j )
			MinCos = std::min(MinCos,vec3::dot_product(normalized(Points[j]),Normal[i]));*/

		// Compute a minimal bounding sphere in local sphere
		math::minimal_sphere( &(Points[0]), Points.size(),
			FanBounds[i].Center, SqrRadius );

		// Store in model space
		FanBounds[i].Center += Coord[i];
		FanBounds[i].Radius = std::sqrt(SqrRadius);

		// Find the convex hull
		// FIXME: Implement GrahamScan here, since we are sorting anyways
		ProjectedPoints.resize( JarvisMarch( &(ProjectedPoints[0]), ProjectedPoints.size() ) );

		CTexgenBase& Base = NormalizedBase[i];
		// Find the minimal bounding box
        matrix2 Rotation{ 1.f };
		MinimalAreaBoundingRectangle( &(ProjectedPoints[0]), uint(ProjectedPoints.size()),
			Rotation, MinRange, MaxRange );
		PreMultiply( Rotation, Base.u, Base.v );

		// Normalize into [0..1] range
		const vec2 Range = MaxRange-MinRange;

		Base.u[3] = -MinRange[0];
		Base.v[3] = -MinRange[1];

		Base.u /= Range[0];
		Base.v /= Range[1];
	}
}

void CFantexMesh::Init( std::vector<vec3>& InputPoints, const std::vector<uint>& InputTriangles )
{
	// grab ownership of the points vector
	Coord = InputPoints;

	// resize other arrays accordingly
	Normal.clear(); // make sure the normals are all zero
	Normal.resize(Coord.size());

	this->Triangles = InputTriangles;
	SetupNormals();

	// build projection bases for texture coordinates in each fan center
	BuildBases();

	// find each fans extensions in projected space
	BuildFans();
	NormalizeRanges();
}


void CFantexMesh::GetTriangle( size_type VertexIndex, size_type TriangleIndex, uint* Triangle ) const
{
	const uint j = Fandata[VertexIndex].Triangles[TriangleIndex]*3; // find the position in the index array
	std::vector<uint>::const_iterator i = Triangles.begin()+j;

	// Copy the triangle indices so that the first index is this one
	std::rotate_copy( i, std::find( i, i+3, VertexIndex ), i+3, Triangle );
}

std::size_t CFantexMesh::GetTriangleCount( size_type VertexIndex ) const
{
	return Fandata[VertexIndex].Triangles.size();
}

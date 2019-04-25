
#include "WireframeRenderer.hpp"
#include "GLSLUtils.hpp"
#include "GLmm/GL.hpp"

#include <replay/byte_color.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/assign.hpp>

#include <boost/filesystem.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/sequential_vertex_coloring.hpp>


/*
	std::vector<color_type> color_table;

	{
		using namespace boost::assign;
		color_table += color_type( 255,0,0 ), color_type( 0,255,0 ), color_type( 0,0,255 ),
			color_type(255,255,0),color_type(255,0,255),color_type(255,255,255),color_type(0,0,0);
	}

	{
		using namespace boost;
		typedef adjacency_list<vecS, vecS, undirectedS> graph_type;

		graph_type graph( td.roots.size() );

		std::size_t a=0;
		triangles.clear();
		for ( std::size_t i=0; i<polys.size(); ++i )
		{
			triangulate( td.roots, polys[i], triangles );

			for ( std::size_t j=a; j<triangles.size(); j+=3 )
			{
				add_edge( triangles[j],triangles[j+1], graph );
				add_edge( triangles[j+1],triangles[j+2], graph );
				add_edge( triangles[j+2],triangles[j], graph );

			}
			a=triangles.size();
		}

		typedef graph_traits<graph_type>::vertices_size_type size_type;
		typedef property_map<graph_type, vertex_index_t>::const_type vertex_index_map;

		std::vector<size_type> color_vec( num_vertices(graph)) ;


		iterator_property_map<size_type*, vertex_index_map> color_map(&color_vec.front(), get(vertex_index, graph));
		size_type num_colors = sequential_vertex_coloring(graph, color_map);
		std::cout << num_colors << std::endl;

		if ( num_colors > color_table.size() )
			throw std::runtime_error( "fail!" );

		this->color.resize( color_vec.size() );
		for ( std::size_t i=0, e=color_vec.size(); i<e; ++i )
		{
			this->color[i] = color_table[color_vec[i]];			
		}
	}
*/

namespace {
	struct VertexType
	{
		vec3 Position;
		vec3 Color;

		VertexType() {}
		VertexType( vec3 Position, vec3 Color ) : Position(Position), Color(Color) {}
	};
};

CWireframeRenderer::CWireframeRenderer( const CFantexMesh& Mesh )
: RenderTiles(false), Overlay(false)
{
	typedef byte_color4 ColorType;
	std::vector<ColorType> ColorTable;

	// Initialize the color table
	{
		using namespace boost::assign;
		ColorTable += ColorType( 255,0,0 ), ColorType( 0,196,0 ), ColorType( 0,0,255 ),
			ColorType(255,255,0),ColorType(255,0,255),ColorType(255,128,255),
			ColorType(255,128,0),ColorType(0,196,196),ColorType(64,196,64);
	}

	using namespace boost;
	typedef adjacency_list<vecS, vecS, undirectedS> GraphType;
	
	GraphType Graph( Mesh.GetVertexCount() );

	// Build the graph from the triangle info
	const std::vector<uint>& Indices = Mesh.GetIndexVector();

	for ( std::size_t i=0; i<Indices.size(); i+=3 )
	{
		add_edge( Indices[i],Indices[i+1], Graph );
		add_edge( Indices[i+1],Indices[i+2], Graph );
		add_edge( Indices[i+2],Indices[i], Graph );
	}


	// Compute a vertex coloring
	typedef graph_traits<GraphType>::vertices_size_type size_type;
	typedef property_map<GraphType, vertex_index_t>::const_type vertex_index_map;

	std::vector<size_type> ColorIndex( num_vertices(Graph) ) ;

	iterator_property_map<size_type*, vertex_index_map>
		ColorMapping(&ColorIndex.front(), get(vertex_index, Graph));
	size_type ColorCount = sequential_vertex_coloring( Graph, ColorMapping );



	std::vector<VertexType> VertexData;
	const std::vector<vec3>& CoordVector=Mesh.GetCoordVector();

	// Insert a wireframe version of the model itself
	for ( auto i=edges(Graph).first; i != edges(Graph).second; ++i )
	{
		using namespace boost::assign;

		VertexData += VertexType( Mesh.GetCoordVector()[source(*i,Graph)], vec3( 0.f, 0.f, 0.f ) ),
			VertexType( Mesh.GetCoordVector()[target(*i,Graph)], vec3( 0.f, 0.f, 0.f ) );
	}
	ModelCount = VertexData.size();

	// Make colored fans
	std::vector<VertexType> TileVertexData;
	uint mx=0,my=0;
	SplitIndex(Mesh.GetVertexCount()-1,mx,my);
	float sidx=1.f/(std::max(mx,my)+1);

	for ( std::size_t i=0,ei=Mesh.GetVertexCount(); i < ei; ++i )
	{
		using namespace boost::assign;

		std::vector<vec3> ModelCoord;
		std::vector<vec2> TileCoord;

		vec3 BasePoint = CoordVector[i];
		bool IsClosed = Mesh.BuildEdgePolygons(i,ModelCoord,TileCoord);

		//byte_color4 ByteColor = ColorTable[ColorIndex[i]%ColorTable.size()];
		byte_color4 ByteColor = ColorTable[i%ColorTable.size()];
		vec3 Color( ByteColor[0]/255.f, ByteColor[1]/255.f, ByteColor[2]/255.f );

		// Build 3D Fan borders
		for ( std::size_t j=2,ej=ModelCoord.size(); j<ej; ++j )
		{
			VertexData += VertexType( ModelCoord[j-1]+BasePoint, Color ),
				VertexType( ModelCoord[j]+BasePoint, Color );	
		}

		if ( IsClosed )
		{
			VertexData += VertexType( ModelCoord[ModelCoord.size()-1]+BasePoint, Color ),
				VertexType( ModelCoord[1]+BasePoint, Color );
		}



		// Build 2D Fan borders
		uint cx=0,cy=0;
		SplitIndex(i,cx,cy);
		const CTexgenBase& Basis = Mesh.GetBaseVector()[i];
		auto Project2h = [](const vec3& p, const vec4& b) {return p[0]*b[0]+p[1]*b[1]+p[2]*b[2]+b[3];};

		for ( std::size_t j=0,ej=TileCoord.size(); j<ej; ++j )
		{
			TileCoord[j] = vec2(Project2h(ModelCoord[j],Basis.u),Project2h(ModelCoord[j],Basis.v));
		}
		
		auto Lambda = [&](std::size_t i)
		{
			return vec3( sidx*(TileCoord[i][0]+cx), sidx*(TileCoord[i][1]+cy), 0.f);
		};

		for ( std::size_t j=2,ej=ModelCoord.size(); j<ej; ++j )
		{
			TileVertexData += VertexType( Lambda(j-1), Color ),
				VertexType( Lambda(j), Color );	
		}

		if ( IsClosed )
		{
			TileVertexData += VertexType( Lambda(ModelCoord.size()-1),Color),
				VertexType( Lambda(1), Color );						
		}
		else
		{
			TileVertexData += VertexType( Lambda(ModelCoord.size()-1),Color),
				VertexType( Lambda(0), Color ), VertexType( Lambda(0), Color ),
				VertexType( Lambda(1),Color);
		}
	}

	vec3 NormalColor( 0.7f, 0.f, 0.3f );
	float NormalLength = 15.f;

	VertexCount = VertexData.size();

	const std::vector<vec3>& NormalVector=Mesh.GetNormalVector();
	for ( std::size_t i=0,ei=Mesh.GetVertexCount(); i < ei; ++i )
	{
		using namespace boost::assign;
		VertexData += VertexType( CoordVector[i],NormalColor ),
			VertexType( CoordVector[i]+NormalVector[i]*NormalLength, NormalColor ); 
	}
	NormalCount = VertexData.size()-VertexCount;
	
	TileVertexCount = TileVertexData.size();

	VertexData.resize( VertexData.size() + TileVertexCount );
	std::copy( TileVertexData.begin(), TileVertexData.end(), VertexData.begin()+VertexCount+NormalCount );

	VertexBuffer.SetData( GL_ARRAY_BUFFER, VertexData, GL_STATIC_DRAW );



	/*this->color.resize( color_vec.size() );
	for ( std::size_t i=0, e=color_vec.size(); i<e; ++i )
	{
		this->color[i] = ColorTable[ColorIndex[i]];			
	}*/

    
	CompileShader( boost::filesystem::current_path()/"Wireframe.glsl", Program );
	Program.Link();

	CompileShader( boost::filesystem::current_path()/"WireframeNormal.glsl", NormalProgram );
	NormalProgram.Link();

	CompileShader( boost::filesystem::current_path()/"WireframeTiles.glsl", TilesProgram );
	TilesProgram.Link();
}

CWireframeRenderer::~CWireframeRenderer()
{
}

void CWireframeRenderer::Render( const matrix4& View, const matrix4& Proj )
{
	void* PositionOffset = (void*)offsetof(VertexType,Position);
	void* ColorOffset = (void*)offsetof(VertexType,Color);

	VertexBuffer.Bind( GL_ARRAY_BUFFER );

	if ( RenderTiles )
	{
		TilesProgram.Use();

		std::size_t PositionLocation = TilesProgram.GetAttribLocation("Position");
		std::size_t ColorLocation = TilesProgram.GetAttribLocation("Color");


		glLineWidth( 1.5f );

		glEnableVertexAttribArray( PositionLocation );
		glVertexAttribPointer(PositionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexType), PositionOffset );
		glEnableVertexAttribArray( ColorLocation );
		glVertexAttribPointer(ColorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexType), ColorOffset );

		glDrawArrays(GL_LINES,VertexCount+NormalCount,TileVertexCount);
	
		glDisableVertexAttribArray( PositionLocation );
		glDisableVertexAttribArray( ColorLocation );
		return;

	}

	Program.Use();


	std::size_t PositionLocation = Program.GetAttribLocation("Position");
	std::size_t ColorLocation = Program.GetAttribLocation("Color");

	glUniformMatrix4fv( Program.GetUniformLocation("ViewMatrix"), 1, GL_FALSE, View.ptr() );
	glUniformMatrix4fv( Program.GetUniformLocation("ProjMatrix"), 1, GL_FALSE, Proj.ptr() );

	glLineWidth( 1.5f );

	glEnableVertexAttribArray( PositionLocation );
	glVertexAttribPointer(PositionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexType), PositionOffset );
	glEnableVertexAttribArray( ColorLocation );
	glVertexAttribPointer(ColorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexType), ColorOffset );

	glDrawArrays(GL_LINES,0,Overlay ? VertexCount : ModelCount);
	
	glDisableVertexAttribArray( PositionLocation );
	glDisableVertexAttribArray( ColorLocation );

	if ( Overlay )
	{
		NormalProgram.Use();
		glEnableVertexAttribArray( PositionLocation );
		PositionLocation = NormalProgram.GetAttribLocation("Position");
		glEnableVertexAttribArray( ColorLocation );
		ColorLocation = NormalProgram.GetAttribLocation("Color");
		glVertexAttribPointer(PositionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexType), PositionOffset );
		glVertexAttribPointer(ColorLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexType), ColorOffset );

		glUniformMatrix4fv( NormalProgram.GetUniformLocation("ViewMatrix"), 1, GL_FALSE, View.ptr() );
		glUniformMatrix4fv( NormalProgram.GetUniformLocation("ProjMatrix"), 1, GL_FALSE, Proj.ptr() );
	
		glLineWidth( 3.0f );
		glDrawArrays(GL_LINES,VertexCount,NormalCount);

		glDisableVertexAttribArray( PositionLocation );
		glDisableVertexAttribArray( ColorLocation );
	}

	GLMM_CHECK();
}

void CWireframeRenderer::SetRenderTiles( bool Rhs )
{
	RenderTiles = Rhs;
}

bool CWireframeRenderer::GetRenderTiles() const
{
	return RenderTiles;
}

void CWireframeRenderer::SetOverlay( bool Rhs )
{
	Overlay = Rhs;
}

bool CWireframeRenderer::GetOverlay() const
{
	return Overlay;
}

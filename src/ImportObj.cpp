
#include "ImportObj.hpp"

#include <list>
#include <vector>
#include <map>
#include <cctype>
#include <sstream>
#include <boost/scoped_array.hpp>
#include <boost/filesystem/fstream.hpp>

namespace
{
	typedef std::string StringT;

	class ParserT
	{

		public:

			typedef std::list< vector3< unsigned int > > FaceT;
			typedef std::list< FaceT >			FaceListT;

			typedef std::pair< std::string, FaceListT > GroupT;
			typedef std::list< GroupT >			GroupListT;

			std::vector< vec3 >					Vertices;
			std::vector< vec2 >					Texcoords;
			std::vector< vec3 >					Normals;

			GroupListT							Groups;

			enum {
				USE_TEXCOORD_BIT = 1 << 1,
				USE_NORMAL_BIT = 1 << 2
			};

												ParserT();
			void								Parse( std::istream& File );

			bool								HasNormals() const { return (State & USE_NORMAL_BIT)!=0; }
			bool								HasTexcoords() const { return (State & USE_TEXCOORD_BIT)!=0; }
	private:

			unsigned int						State;

			void								ProcessLine( std::istream& Line );
			void								ProcessFace( std::istream& Face );
			void								ProcessIndices( std::istream& Indices, FaceT& Face );

	};
	typedef unsigned short uint16;
	typedef vector3< unsigned int > ivec;
	typedef std::map< ivec, uint16, ivec::less > LookupT;

	uint16 GetIndex( const ivec& x, LookupT& m, uint16& vc )
	{
		/*std::pair< LookupT::iterator, bool > r = m.insert( std::make_pair( x, vc ) );
		if ( r.second )
			return vc++;
		else
			return vc;*/

		LookupT::iterator Node = m.find( x );

		if ( m.end() == Node )
		{
			m.insert( std::make_pair( x, vc ) );
			return vc++;
		}
		else
			return Node->second;
	}
}

ParserT::ParserT()
: State( 0 )
{
}

void
ParserT::ProcessLine( std::istream& Line )
{
	StringT      Tag;

	Line.setf( std::ios::skipws );
	Line >> Tag;

	if ( Tag == "v" )
	{
		vec3 x;
		Line >> x[ 0 ] >> x[ 1 ] >> x[ 2 ]; // Parse a vertex.
		Vertices.push_back( x ); // and add it.
	}
	else if ( Tag == "vt" )
	{
		vec2 y;
		Line >> y[ 0 ] >> y[ 1 ]; // Parse a texcoord
		Texcoords.push_back( y );
	}
	else if ( Tag == "vn" )
	{
		vec3 x;
		Line >> x[ 0 ] >> x[ 1 ] >> x[ 2 ];
		Normals.push_back( x );
	}
	else if ( Tag == "g" )
	{
		// Parse a Group tag.
		Groups.push_back( GroupT() );
		Line >> Groups.back().first;
	}
	else if ( Tag == "f" )
	{
		ProcessFace( Line );
	}
	else
	{
		// Ignore other tags
	}
}

void
ParserT::Parse( std::istream& File )
{
	StringT Buffer;
	
	vec3	    x;
	vec2	    y;

	while ( File.eof() == false )
	{
		// see if this is a comment
		if ( File.peek() == '#' )
		{
			// TODO: make sure this really skips a whole line
			File.ignore( 0xFFFF, '\n' );
		}
		else
		{
			std::getline( File, Buffer );
			if ( File.rdstate() & std::ios::failbit )
				break;

			std::istringstream Line( Buffer );

			ProcessLine( Line );
		}
	}
}


void
ParserT::ProcessFace( std::istream& Face )
{
	StringT Buffer;

	// There was no group defined before, so create a default one.
	if ( Groups.empty() )
	{
		Groups.push_back( GroupT() );
		Groups.back().first = "(default)";
	}

	Groups.back().second.push_back( FaceT() );

	while ( Face.good() )
	{
		Face >> Buffer;

		if ( Face.rdstate() & std::ios::failbit )
			break;

		std::istringstream Indices( Buffer );

		ProcessIndices( Indices, Groups.back().second.back() );
	}
}

void
ParserT::ProcessIndices( std::istream& Indices, FaceT& Face )
{
	char temp;
	replay::vector3< int > TempResult( 0, 0, 0 );

	// the first part is always a number
	Indices >> TempResult[ 0 ];

	// check if the delimiter is valid
	Indices >> temp;

	if ( temp == '/' ) // vertex def goes on?
	{
		// check if we have texcoord
		temp = Indices.peek();

		if ( std::isdigit( temp ) || temp == '-' )
		{
			this->State |= USE_TEXCOORD_BIT;
			Indices >> TempResult[ 1 ];
		}

		Indices >> temp;

		// check if normal info is there if there was a delimiter
		if ( temp == '/' )
		{
			if ( Indices.good() && (std::isdigit( Indices.peek() ) || Indices.peek() == '-' ))
			{
				this->State |= USE_NORMAL_BIT;
				Indices >> TempResult[ 2 ];
			}
		}
	}

	// remove all negative indices
	replay::vector3< unsigned int > Result;

	if ( TempResult[ 0 ] < 0 )
		Result[ 0 ] = static_cast< unsigned int >( Vertices.size() ) - TempResult[ 0 ] + 1;
	else
		Result[ 0 ] = TempResult[ 0 ];

	if ( TempResult[ 1 ] < 0 )
		Result[ 1 ] = static_cast< unsigned int >( Texcoords.size() ) - TempResult[ 1 ] + 1;
	else
		Result[ 1 ] = TempResult[ 1 ];

	if ( TempResult[ 2 ] < 0 )
		Result[ 2 ] = static_cast< unsigned int >( Normals.size() )- TempResult[ 2 ] + 1;
	else
		Result[ 2 ] = TempResult[ 2 ];

	Face.push_back( Result );
}

void WavefrontObj::Import( const boost::filesystem::path& Path, CallbackType Callback )
{
	// open the file
	boost::filesystem::ifstream File( Path, std::ios::binary );

	// parse it
	ParserT Parser;
	Parser.Parse( File );

	// convert the data into something useable..
	for ( ParserT::GroupListT::iterator k = Parser.Groups.begin(); k != Parser.Groups.end(); ++k )
	{
		std::size_t IndexCount = 0;
		
		// count the number of indices we are going to need
		for ( ParserT::FaceListT::iterator i = k->second.begin(); i != k->second.end(); ++i )
		{
			// this is not a planar polygon!
			if ( i->size() < 3 )
				throw std::runtime_error( "Non-planar polygon found." );

			// decompose polygons into triangles (by using a triangle-fan)
			IndexCount += (static_cast< unsigned int >(i->size()) - 2) * 3;
		}

		boost::scoped_array< uint16 >	IndexBuffer( new uint16[ IndexCount ] );
		IndexCount = 0;

		LookupT IndexMap;
		uint16 VertexCount = 0;

		for ( ParserT::FaceListT::iterator i = k->second.begin(); i != k->second.end(); ++i )
		{
			ParserT::FaceT::iterator j = i->begin();

			// get the head index
			const uint16 HeadIndex = GetIndex( *j, IndexMap, VertexCount );

			// get the second index
			uint16 LastIndex = GetIndex( *(++j), IndexMap, VertexCount );
			
			for ( ++j; j != i->end(); ++j )
			{
				const uint16 CurrentIndex = GetIndex( *j, IndexMap, VertexCount );

				IndexBuffer[ IndexCount++ ] = HeadIndex;
				IndexBuffer[ IndexCount++ ] = LastIndex;
				IndexBuffer[ IndexCount++ ] = CurrentIndex;

				LastIndex = CurrentIndex;
			}
		}

		CModelGroup Group;

		boost::scoped_array< vec3 >		VertexBuffer( new vec3[ VertexCount ] );

		for ( LookupT::const_iterator i = IndexMap.begin(); i != IndexMap.end(); ++i )
			VertexBuffer[ i->second ] = Parser.Vertices[ i->first[ 0 ] - 1 ];

		boost::scoped_array< vec2 >		TexcoordBuffer;

		if ( Parser.HasTexcoords() )
		{
			TexcoordBuffer.reset( new vec2[ VertexCount ] );

			for ( LookupT::const_iterator i = IndexMap.begin(); i != IndexMap.end(); ++i )
				if ( i->first[ 1 ] ) // FIXME: there really should be a nicer way to handle this
					TexcoordBuffer[ i->second ] = Parser.Texcoords[ i->first[ 1 ] - 1 ];
		}

		boost::scoped_array< vec3 >		NormalBuffer;

		if ( Parser.HasNormals() )
		{
			NormalBuffer.reset( new vec3[ VertexCount ] );

			for ( LookupT::const_iterator i = IndexMap.begin(); i != IndexMap.end(); ++i )
				if ( i->first[ 2 ] ) // FIXME: there should be a nicer way to handle this.
					NormalBuffer[ i->second ] = Parser.Normals[ i->first[ 2 ] - 1 ];
		}


		Group.VertexBuffer = VertexBuffer.get();
		Group.NormalBuffer = NormalBuffer.get();
		Group.TexcoordBuffer = TexcoordBuffer.get();
		Group.VertexCount = VertexCount;

		Group.IndexBuffer = IndexBuffer.get();
		Group.IndexCount = IndexCount;

		Callback( k->first, Group );
	}
}


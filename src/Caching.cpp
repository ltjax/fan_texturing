/*

Cataclysm-Software Disaster Engine
----------------------------------

Repository-Info:
$Id: Caching.cpp 601 2008-05-27 17:35:16Z ltjax $

Copyright:
Marius Elvert (marius.elvert@cataclysm-software.com) 2006-2007

*/

#include "Caching.hpp"
#include <stdexcept>
#include <limits>
#include <cmath>
#include <vector>
#include <boost/scoped_array.hpp>


namespace {
	
	typedef unsigned int uint32;
	typedef unsigned short uint16;
	const float CacheDecayPower = 1.5f;
	const float LastTriScore = 0.75f;
	const float ValenceBoostScale = 2.0f;
	const float ValenceBoostPower = 0.5f;
	
	struct CacheEntryT
	{
		uint16		Value;
		int			LastUsed;

		CacheEntryT() : Value( 0 ), LastUsed( -1 ) {}
	};

	// Implementation of Tom Forsyth's algorithm
	struct ForsythsAlgorithm
	{	


		struct VERTEX
		{
			float		Score;
			uint32		TriangleCount; // triangles that reference this
			uint32		RemainingTriangles;
			uint16*		Triangle;
			uint16		CachePosition;

			VERTEX() : Score( 0 ), TriangleCount( 0 ), RemainingTriangles( 0 ), Triangle( 0 ), CachePosition( 0 ) {}
		};

		struct TRIANGLE
		{
			uint16		Indices[ 3 ];
			float		Score;
			bool		Added;
		};

		std::size_t						TriangleCount;
		boost::scoped_array< VERTEX >	Vertex;
		boost::scoped_array< TRIANGLE >	Triangle;
		boost::scoped_array< uint16 >	IndexReferences;

		void InitPhase( uint16* IndexArray, std::size_t IndexCount, std::size_t VertexCount );
		void MainPhase( uint16* IndexArray, std::size_t IndexCount, std::size_t VertexCount );
	};

	void ForsythsAlgorithm::InitPhase( uint16* IndexArray, std::size_t IndexCount, std::size_t VertexCount )
	{
		this->TriangleCount = IndexCount / 3;

		Vertex.reset( new VERTEX[ VertexCount ] );
		Triangle.reset( new TRIANGLE[ TriangleCount ] );

		// Initialise the triangle indices and count the number of refs each vertex gets
		for ( std::size_t i = 0; i < TriangleCount; ++i )
		{
			const uint32 Current = i * 3;

			std::copy( IndexArray + Current, IndexArray + Current + 3, Triangle[ i ].Indices );

			Triangle[ i ].Score = 0.f;
			Triangle[ i ].Added = false;

			for ( uint32 j = 0; j < 3; ++j )
				Vertex[ Triangle[ i ].Indices[ j ] ].TriangleCount++;
		}

		// Get some space for each vertex referees
		IndexReferences.reset( new uint16[ IndexCount ] );

		uint16* Offset = IndexReferences.get();
		for ( std::size_t i = 0; i < VertexCount; ++i )
		{
			if ( Vertex[ i ].TriangleCount )
			{
				Vertex[ i ].Triangle = Offset;
				Offset += Vertex[ i ].TriangleCount;
			}
		}

		// Setup the references for each triangle
		for ( std::size_t i = 0; i < TriangleCount; ++i )
		{
			for ( uint32 j = 0; j < 3; ++j )
			{
				VERTEX& v = Vertex[ Triangle[ i ].Indices[ j ] ];
				v.Triangle[ v.RemainingTriangles++ ] = i;
			}		
		}

		// Compute the initial vertex scores
		for ( std::size_t i = 0; i < VertexCount; ++i )
		{
			VERTEX& v = Vertex[ i ];
			
			// This vertex isn't used
			// FIXME: does this matter since score is only referenced via a triangle?
			if ( v.RemainingTriangles == 0 )
				v.Score = -1.f;
			else
			{
				// assign score based on how rare this one is
				v.Score = ValenceBoostScale * 
					std::pow( static_cast< float >( v.RemainingTriangles ), -ValenceBoostPower );
			}

			for ( uint32 j = 0; j < v.TriangleCount; ++j )
			{
				Triangle[ v.Triangle[ j ] ].Score += v.Score;
			}
		}
	}

	void ForsythsAlgorithm::MainPhase( uint16* IndexArray, std::size_t IndexCount, std::size_t VertexCount )
	{
		InitPhase( IndexArray, IndexCount, VertexCount );

		// now we need a cache
		uint32 Cache[ 32 ];
		uint32 CacheSize = 0;

		for ( uint32 CurrentTriangle = 0; CurrentTriangle < TriangleCount; ++CurrentTriangle )
		{
			// find the best Triangle
			unsigned int Best = 0;
			while ( Triangle[ Best ].Added )
				++Best;

			for ( uint32 i = Best+1; i < TriangleCount; ++i )
				if ( !Triangle[ i ].Added &&
					Triangle[ i ].Score > Triangle[ Best ].Score )
					Best = i;

			for ( uint32 j = 0; j < 3; ++j )
			{
				const uint32 Index = Triangle[ Best ].Indices[ j ];
				IndexArray[ CurrentTriangle*3 + j ] = Index;

				Vertex[ Index ].RemainingTriangles--;
			}

			Triangle[ Best ].Added = true;

			// update cache and scores for those added and deleted from the cache
			for ( uint32 j = 0; j < 3; ++j )
			{
				const uint32 Index = Triangle[ Best ].Indices[ j ];
				VERTEX& v = Vertex[ Index ];

				// we need to insert this one first
				if ( v.CachePosition == 0 )
				{
					if ( CacheSize < 32 )
					{
						Cache[ CacheSize ] = Index;
						v.CachePosition = ++CacheSize;
					}
					else
					{
						VERTEX& p = Vertex[ Cache[ 31 ] ];

						// replace the last one
						p.CachePosition = 0;
						float ScoreDelta = (ValenceBoostScale * 
							std::pow( static_cast< float >( p.RemainingTriangles ), -ValenceBoostPower )) - p.Score;
						p.Score += ScoreDelta;

						for ( unsigned int k = 0; k < p.RemainingTriangles; ++k )
							Triangle[ p.Triangle[ k ] ].Score += ScoreDelta;

						Cache[ 31 ] = Index;
						v.CachePosition = 32;
					}
				}
				
				// (bubble) sort it *yuck*
				while ( v.CachePosition > 1 )
				{
					VERTEX& p = Vertex[ Cache[ v.CachePosition-2 ] ];
					Cache[ v.CachePosition-1 ] = Cache[ v.CachePosition-2 ];
					Cache[ v.CachePosition-2 ] = Index;
					p.CachePosition++;
					v.CachePosition--;
				}

				float ScoreDelta = LastTriScore + (ValenceBoostScale * 
						std::pow( static_cast< float >( v.RemainingTriangles ), -ValenceBoostPower )) - v.Score;

				v.Score += ScoreDelta;

				for ( unsigned int k = 0; k < v.RemainingTriangles; ++k )
					Triangle[ v.Triangle[ k ] ].Score += ScoreDelta;
			}

			// update scores for all other cache entries and their associated triangles
			for ( unsigned int j = 3; j < CacheSize; ++j )
			{
				VERTEX& v = Vertex[ Cache[ j ] ];
				const float Scaler = 1.0f / ( 32 - 3 );
				float NewScore = std::pow( 1.0f - ( j - 3 ) * Scaler, CacheDecayPower )
					+ (ValenceBoostScale * std::pow( static_cast< float >( v.RemainingTriangles ), -ValenceBoostPower ));
				float ScoreDelta = NewScore - v.Score;
				v.Score = NewScore;

				for ( unsigned int k = 0; k < v.RemainingTriangles; ++k )
					Triangle[ v.Triangle[ k ] ].Score += ScoreDelta;
			}
		}
	}
};


void Caching::OptimizeTriangleIndicesForCache( 
	uint16* IndexArray, std::size_t IndexCount, std::size_t VertexCount )
{

	ForsythsAlgorithm x;
	x.MainPhase( IndexArray, IndexCount, VertexCount );
}

void Caching::BuildVertexOrderTranslationTable( 
		const uint16* IndexArray, std::size_t IndexCount,
		uint16* TranslationTable, std::size_t VertexCount )
{
	std::vector< bool > Tag( VertexCount, false );

	uint16 j = 0;
	for ( std::size_t i = 0; i < IndexCount; ++i )
	{
		if ( !Tag[ IndexArray[ i ] ] )
		{
			TranslationTable[ IndexArray[ i ] ] = j;
			++j;
			Tag[ IndexArray[ i ] ] = true;
		}
	}

	if ( j != VertexCount )
		throw std::runtime_error( "There seem to be unused vertices!" );
}


std::size_t Caching::CountMisses( 
		const uint16* IndexArray, std::size_t IndexCount,
		std::size_t CacheSize, bool UseLRU )
{


	boost::scoped_array< CacheEntryT > Cache( new CacheEntryT[ CacheSize ] );
	
	std::size_t Result = 0; // number of cache misses

	for ( std::size_t i = 0; i < IndexCount; ++i )
	{
		bool InCache = false;

		register std::size_t Index = 0;
		register uint16 Value = IndexArray[ i ];

		// try to find the value in the cache,
		// or figure and the index that wasn't used the longest
		for ( std::size_t j = 0; j < CacheSize; ++j )
		{
			// do we have a hit?
			if ( ( Cache[ j ].LastUsed >= 0 ) && // has the cache entry been used yet?
				 ( Cache[ j ].Value == Value ) )
			{
				InCache = true;
				Index = j;
				break;
			}

			// the current one was used more recently
			if ( Cache[ Index ].LastUsed > Cache[ j ].LastUsed )
				Index = j;
		}

		if ( !InCache )
		{
			++Result;

			Cache[ Index ].Value = Value;
			Cache[ Index ].LastUsed = i;
		}
		else if ( UseLRU )
		{
			Cache[ Index ].LastUsed = i;			
		}
	}

	return Result;
}

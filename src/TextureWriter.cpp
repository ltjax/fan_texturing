
#include "TextureWriter.hpp"
#include "PageAtlas.hpp"
#include "Misc.hpp"
#include <replay/couple.hpp>
#include <replay/vector_math.hpp>
#include <replay/bstream.hpp>
#include <replay/buffer.hpp>
#include "GLmm/Framebuffer.hpp"
#include <boost/bind.hpp>
#include <glsk/glsk.hpp>
#include <squish.h>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

namespace {
	vec2 vabs( const vec2& arg )
	{
		return vec2(std::fabs(arg[0]),std::fabs(arg[1]));
	}

	/** POD to describe the texture.
	*/
	struct CTextureHeader
	{
		CTextureHeader() : GoodFood(0x600DF00D),TileCount(0),TileSize(0),BlockSize(0),BlockAlignment(0) {}

		uint	GoodFood; //< Has to be 0x600DF00D
		uint	TileCount; //< The number of fans
		uint	TileSize; //< The width and height of the pages
		uint	BlockSize; //< The size of one page in bytes (compressed)
		uint	BlockAlignment; //< The alignement of one page in bytes

		void Write( byte* Target )
		{
			ByteCopyAdvance(GoodFood,Target);
			ByteCopyAdvance(TileCount,Target);
			ByteCopyAdvance(TileSize,Target);
			ByteCopyAdvance(BlockSize,Target);
			ByteCopyAdvance(BlockAlignment,Target);
		}
	};

	template < typename RangeType, typename IndexType >
	boost::tuple<RangeType,RangeType> FairSplit( RangeType Size, IndexType BlockID, IndexType BlockCount )
	{
		const RangeType ChunkSize = static_cast<RangeType>(Size / BlockCount);
		const RangeType Rest = static_cast<RangeType>(Size % BlockCount);
		const RangeType Left = static_cast<RangeType>(BlockID*ChunkSize +
			std::min<RangeType>(static_cast<RangeType>(BlockID),Rest));
		const RangeType Right = Left + ChunkSize + ((static_cast<RangeType>(BlockID)<Rest)?1u:0u);
		return boost::make_tuple(Left,Right);
	}


	void ParallelCompressImage( const replay::shared_pixbuf& Image,
		ubyte* Result, int Flags, boost::threadpool::pool& Pool, std::size_t Tasks )
	{
		int w=Image->get_width();
		int h=Image->get_height();

		// We got some weird dimensions, use the fallback
		if ( (h%4) != 0 || Tasks == 1 )
		{
			squish::CompressImage( Image->get_data(), w, h,
				Result, Flags );
			return;
		}

		int BlockRows = h/4;
		
		for ( std::size_t i=0; i<Tasks; ++i )
		{
			boost::tuple<int,int> Range = FairSplit( BlockRows,i,Tasks );
			int sh = (Range.get<1>()-Range.get<0>())*4;

			Pool.schedule( boost::bind( &squish::CompressImage, Image->get_pixel(0,Range.get<0>()*4), w, sh, Result, Flags ) );

			int TargetSize = squish::GetStorageRequirements(w,sh,Flags);
			Result += TargetSize;
		}

		Pool.wait();
	}


};

replay::shared_pixbuf
CTextureWriter::GenerateNode( uint Level, uint Index, byte* Blocks,
	boost::threadpool::pool& Pool, GLmm::Framebuffer& Target )
{
	const uint TileCount = uint(Mesh.GetCoordVector().size());
	replay::shared_pixbuf Result;

	// Out of bounds? Create some empty space...
	if ( Index >= TileCount )
	{
		Result = replay::pixbuf::create( TileSize, TileSize, replay::pixbuf::rgba );
		Result->fill( 0, 0, 0, 0 );
		return Result;
	}

	double StartTime;

	// Recurse...
	if ( Level > 0 )
	{
		Result = replay::pixbuf::create( TileSize, TileSize, replay::pixbuf::rgba );

		replay::shared_pixbuf Children[4];

		for ( uint i=0;i<4; ++i )
			Children[i] = GenerateNode( Level-1, Index | (i<<((Level-1)*2)), Blocks, Pool, Target );

		StartTime = glsk::get_time();

		AlphaDownsampleRgba( *(Children[0]), *(Children[1]), *(Children[2]), *(Children[3]), *Result );
	}
	else
	{
		StartTime = glsk::get_time();

		// Render a leaf in the GL
		const std::vector<vec3>& Coord =
			Mesh.GetCoordVector();

		TextureGenerator.Bind(Coord[Index],Mesh.GetFanBounds(Index));

		Target.Bind();
		glViewport(0,0,TileSize,TileSize);
		FanDrawer.Draw(Index);

		Result = FramebufferTexture.GetImage(0);
		//Result->save_to_file( "Decomp"+boost::lexical_cast<std::string>(Index)+".png" );
	}

	double MiddleTime = glsk::get_time();
	double MiddleDelta = MiddleTime-StartTime;

	if ( Level > 0 )
		DownsamplingTime+=MiddleDelta;
	else
		RenderingTime+=MiddleDelta;

	// Compute the position in the file
	page_id		Page(Level,Index);
	uint		ImageIndex = ContiguousIndex(Page,TileCount);
	byte*		ImageTarget = AlignWith(ByteSize,Alignment)*ImageIndex + Blocks;

	ParallelCompressImage( Result, ImageTarget, CompressionFlags, Pool, Pool.size() );

	CompressionTime += glsk::get_time()-MiddleTime;

	return Result;
}

void CTextureWriter::operator()( const boost::filesystem::path& Filename )
{
	double StartTime = glsk::get_time();

	RenderingTime = 0.0;
	DownsamplingTime = 0.0;
	CompressionTime = 0.0;

	// Namespace and typedefs
	namespace fs = boost::filesystem;
	typedef replay::obstream<fs::ofstream> WriteBinary;
	using replay::matrix4;
	using replay::fcouple;

	// Calculate the depth of the resulting quad tree
	const uint TileCount = uint(Mesh.GetCoordVector().size());
	const uint Depth = GetMaximumLevel( TileCount );

	// Setup the compression buffer
	ByteSize = squish::GetStorageRequirements(
		TileSize,TileSize,CompressionFlags);
	CompressionBuffer.alloc( ByteSize );

	// Setup the rendering buffer
	FramebufferTexture.SetImage( 0, GL_RGBA8, TileSize, TileSize );
	FramebufferTexture.SetWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	FramebufferTexture.SetFilter( GL_NEAREST, GL_LINEAR );

	// Setup the rendering target & viewport
	GLmm::Framebuffer Framebuffer;
	Framebuffer.AttachColorbuffer( 0, FramebufferTexture );
	Framebuffer.TestCompleteness();
	Framebuffer.Bind();
	glViewport( 0, 0, TileSize, TileSize );

	std::size_t PaddedByteSize = AlignWith( ByteSize, Alignment );
	const uint ImageCount = ContiguousIndexCount( TileCount );
	const std::size_t PreambleSize = AlignWith( (5+ImageCount)*sizeof(uint), Alignment );
	const std::size_t TotalSize = PreambleSize + PaddedByteSize*ImageCount;

	// Remove an old file, if it exists
	boost::filesystem::remove(Filename);

	// Open the file
	boost::iostreams::mapped_file_params Params;
	Params.path = Filename.string();
	Params.mode = std::ios::out;
	Params.new_file_size = TotalSize;

	std::cout << "Starting to generate texture file of " << (TotalSize>>20) << " mb" << std::endl;

	boost::iostreams::mapped_file File(Params);
	//File.open( Filename, std::ios::binary );

	// Write a small header
	CTextureHeader Header;
	Header.TileCount=TileCount;
	Header.TileSize=TileSize;
	Header.BlockSize=ByteSize;
	Header.BlockAlignment=Alignment;

	byte* Data = reinterpret_cast<byte*>(File.data());
	Header.Write(Data);

	// setup the index table
	byte* Table = Data+5*sizeof(uint);
	for ( std::size_t i=0; i<ImageCount; ++i )
		ByteCopy( uint(PreambleSize+i*PaddedByteSize),Table+i*sizeof(uint));

	// Start generating the data
	boost::threadpool::pool Pool(4);
	GenerateNode( Depth, 0, Data + PreambleSize, Pool, Framebuffer );
	Pool.wait();

	// Reset rendering settings
	Framebuffer.Unbind();
	GLmm::Program::Disable();
	glActiveTexture( GL_TEXTURE0 );

	double TotalTime = glsk::get_time()-StartTime;
	std::cout << "Wrote texture in " << TotalTime << " seconds." << std::endl;
	std::cout << "- Rendering time: " << RenderingTime << std::endl;
	std::cout << "- Downsampling time: " << DownsamplingTime << std::endl;
	std::cout << "- Compression time: " << CompressionTime << std::endl;
}

CTextureWriter::CTextureWriter(
	CFantexMesh& Mesh,
	CAbstractTextureGenerator& Generator, uint TileSize )
:
	Mesh( Mesh ),TextureGenerator( Generator ),
	TileSize( TileSize ),
	ByteSize(0), Alignment(1024), FanDrawer(Mesh)
{
	CompressionFlags=squish::kDxt1|squish::kColourRangeFit;
}


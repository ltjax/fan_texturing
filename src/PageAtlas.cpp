
#include "PageAtlas.hpp"

#include <iterator>
#include <sstream>

#include <boost/assign.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/lexical_cast.hpp>
#include <replay/vector_math.hpp>
#include <replay/bstream.hpp>
#include <replay/pixbuf.hpp>
#include <replay/buffer.hpp>
#include <replay/table.hpp>
#include <replay/byte_color.hpp>
#include <squish.h>

#include "GLmm/Texture.hpp"
#include "GLmm/Framebuffer.hpp"
#include "GLmm/GL.hpp"
#include "PageScheduler.hpp"
#include "GLSLUtils.hpp"
#include "Misc.hpp"

using boost::scoped_array;

void CPageAtlas::CPageIterator::Init( CPageEntry* Root, std::size_t Count )
{
	this->Depth = Root->Page.miplevel;
	Section.reset( new SectionType[Depth+1] );

	this->Index=0;
	this->Count=Count;

	Section[Depth].Set(Root,1<<(GetMaximumLevel(Count)<<1));

	// Go up the leftmost branch of the tree
	while ( Root->Child[0] && Root->Page.miplevel > 0 )
	{
		uint Subend = Section[Depth].End/4;
		Root=Root->Child[0];
		Depth=Depth-1;
		Section[Depth].Set(Root,Subend);
	}
}

void CPageAtlas::CPageIterator::Advance()
{
	this->Index++;

	// End current sections
	while ( this->Index == Section[Depth].End )
	{
		Section[Depth].Set(0,0);
		Depth++;
	}

	while ( Depth > 0 )
	{
		uint SectionLength=(1<<(Depth<<1));
		uint Begin=Section[Depth].End-SectionLength;

		uint Delta = this->Index-Begin;
		uint Sublength = SectionLength/4;
		uint ChildIndex = Delta/Sublength;

		if ( Delta%Sublength == 0 && Section[Depth].Page->Child[ChildIndex] )
		{
			Section[Depth-1].Set(Section[Depth].Page->Child[ChildIndex],Begin+(ChildIndex+1)*Sublength);
			Depth--;
		}
		else
			break;
	}
}

CPageAtlas::CPageAtlas( const PathT& PageFile, uint Index, uint PageNumSqrt )
: PageNumSqrt(PageNumSqrt), PageFaultLimit(2), IsEditable(false)
{
	this->ElementCount = Index;
	this->MaxLevel = GetMaximumLevel( Index );

	// Create the page loader
	PageLoader.reset( new CPageScheduler( PageFile, this->ElementCount, PageNumSqrt*4, 
		boost::bind( &CPageAtlas::MapReadyPage, this, _1, _2, _3 ),
		boost::bind( &CPageAtlas::MapReadyPageMemory, this, _1, _2 ),
		boost::bind( &CPageAtlas::OnPageWritten, this, _1 ) ) );
	

	const uint CacheSize = PageLoader->GetTileSize() * PageNumSqrt;

	// Create the empty cache texture
	replay::table<replay::byte_color4> Buffer(CacheSize,CacheSize,replay::byte_color4(0,0,0,255));
	GpuCache.SetFilter( GL_LINEAR, GL_LINEAR );
	GpuCache.SetWrap( GL_CLAMP_TO_EDGE );
	GpuCache.SetImage( 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
		CacheSize, CacheSize, GL_RGBA, GL_UNSIGNED_BYTE, Buffer.ptr() );


	// ..and the empty page table for it
	PageTable.resize( PageNumSqrt, PageNumSqrt );

	// Map the lowest mipmap level immediately
	MapPage(page_id(MaxLevel,0));

	// Precompute the position in the base texture
	Pos.reset( new std::pair<uint,uint>[ElementCount] );
	for ( std::size_t i=0; i<ElementCount; ++i )
		SplitIndex(i,Pos[i].first,Pos[i].second);

	// Create the downsampling shader for editing
	CompileShader( boost::filesystem::initial_path()/"Downsample.glsl", DownsampleProgram );
	DownsampleProgram.Link();
}

CPageAtlas::~CPageAtlas()
{
	if ( this->GetEditable() )
		FlushAll();
}

void CPageAtlas::SetEditable( bool Rhs )
{
	if ( Rhs == this->IsEditable )
		return;

	// Make sure we have all changes written out
	if ( IsEditable )
		FlushAll();

	this->IsEditable = Rhs;

	const uint CacheSize = PageLoader->GetTileSize() * PageNumSqrt;

	// Use compression only without editing
	GLint InternalFormat = this->IsEditable ? GL_RGBA : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

	// Reset the cache texture
	GpuCache.SetImage( 0, InternalFormat, CacheSize, CacheSize,
		GL_RGBA, GL_UNSIGNED_BYTE, 0 );

	// Make sure we have a decompression buffer
	if ( this->IsEditable )
	{
		// RGBA8 textures, 4 bytes per texel
		std::size_t TextureSize = PageLoader->GetTileSize()*PageLoader->GetTileSize()*4;

		DecompBuffer.Bind( GL_PIXEL_UNPACK_BUFFER );
		DecompBuffer.SetData( GL_PIXEL_UNPACK_BUFFER, TextureSize, 0, GL_STREAM_DRAW );
		DecompBuffer.Unbind( GL_PIXEL_UNPACK_BUFFER );

		
		ReadFBO.AttachColorbuffer(0,GpuCache);
		ReadFBO.TestCompleteness();
		GLmm::Framebuffer::Unbind();
	}

	// In either case, clear the cache
	for (uint y=0;y<PageNumSqrt;++y)
	for (uint x=0;x<PageNumSqrt;++x)
		PageTable(x,y).IsLoaded = false;

	// Map the lowest mipmap level immediately
	MapPage(page_id(MaxLevel,0));
}

bool CPageAtlas::GetEditable() const
{
	return IsEditable;
}

void CPageAtlas::GetPositionInCache( CPageEntry* Page, uint& x, uint& y )
{
	uint EntryIndex = uint(Page - PageTable.ptr());
	x = EntryIndex % PageNumSqrt;
	y = EntryIndex / PageNumSqrt;
}

CPageAtlas::CPageEntry* CPageAtlas::GetCacheEntry( CPageEntry* Exclude, bool Force )
{
	CPageEntry* BestEntry=0;
	bool	BestDirtyOrLocked=false;

	for (uint y=0;y<PageNumSqrt;++y)
	for (uint x=0;x<PageNumSqrt;++x)
	{
		CPageEntry* Current=&PageTable(x,y);

		if ( !Current->IsLoaded )
			return Current;
		
		// never evict inner nodes
		if ( Current == Exclude || 
			 Current->Child[0] || Current->Child[1] ||
			 Current->Child[2] || Current->Child[3] )
			continue;

		// never evict locked or dirty pages, unless we need it really bad
		bool DirtyOrLocked = (Current->IsModified || Current->IsWriting);
		if ( !Force && DirtyOrLocked )
			continue;


		if ( !BestEntry || (BestDirtyOrLocked && !DirtyOrLocked) ||
			((BestDirtyOrLocked == DirtyOrLocked) && (BestEntry->LastUse <= Current->LastUse)) )
		{
			BestDirtyOrLocked = DirtyOrLocked;
			BestEntry = Current;
		}
	}

	// Unlock it, if we did not find a better one
	if ( BestDirtyOrLocked )
	{
		Flush( BestEntry, true );
	}

	return BestEntry;
}

void CPageAtlas::ReplaceContent( CPageEntry* Page, GLmm::Framebuffer& Fbo )
{
	if ( !GetEditable() )
		throw std::runtime_error( "Trying to replace page in a non-editable atlas." );

	if ( !Page )
		throw std::runtime_error( "Page is null!" );

	if ( Page->Page.miplevel )
		throw std::runtime_error( "Page has non zero miplevel." );

	uint Index = Page->Page.index;

	uint EntryIndex = uint(Page - PageTable.ptr());
	uint x = EntryIndex % PageNumSqrt;
	uint y = EntryIndex / PageNumSqrt;

	uint TileSize = PageLoader->GetTileSize();

	Fbo.Bind();
	glActiveTexture( GL_TEXTURE0 );
	GpuCache.Bind();

	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, TileSize*x,TileSize*y,0,0,TileSize,TileSize);

	Page->JustUsed |= 1;
	Page->IsModified = true;
	Page->LastDraw = 0;
	uint SourceSize = TileSize;

	GLmm::BufferObject			QuadBuffer;
	// Setup Quad geometry
	{
		using namespace boost::assign;
		std::vector<vec2> Quad;
		Quad += vec2(0.f,0.f),vec2(1.f,0.f),vec2(0.f,1.f),vec2(1.f,1.f);
		QuadBuffer.SetData( GL_ARRAY_BUFFER, Quad, GL_DYNAMIC_DRAW );
	}
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_FLOAT, sizeof(float)*2, 0 );

	while ( Page->Parent && SourceSize > 2 )
	{
		glViewport( 0, 0, SourceSize/2, SourceSize/2 );
		DownsampleProgram.Use();
		glUniform1i( DownsampleProgram.GetUniformLocation("CacheTexture"),0);
		glUniform3fv( DownsampleProgram.GetUniformLocation("OffsetAndScale"),1,
			ComputeOffsetAndScale(Index,Page).ptr() );

		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		// Copy back to the cache
		uint dx=0,dy=0;
		ComputePixelOffset(Index,Page->Parent,dx,dy);
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, dx,dy,0,0,SourceSize/2,SourceSize/2);
		Page->Parent->IsModified = true;
		Page->Parent->LastDraw = 0;
		Page->Parent->JustUsed |= 1;

		SourceSize /= 2;
		Page = Page->Parent;
	}

	glDisableClientState( GL_VERTEX_ARRAY );
}

/** Compute the pixel offset in the cache.
*/
void CPageAtlas::ComputePixelOffset( uint Index, const CPageEntry* Entry, uint& rx, uint& ry )
{		
	uint EntryIndex = uint(Entry - PageTable.ptr());
	uint bx = EntryIndex % PageNumSqrt;
	uint by = EntryIndex / PageNumSqrt;

	uint px=Pos[Index].first,py=Pos[Index].second;
	uint Mask = Entry->Page.miplevel ? ((1<<(Entry->Page.miplevel))-1) : 0;
	px &= Mask;
	py &= Mask;

	uint TileSize = PageLoader->GetTileSize();
	uint MipSize = TileSize >> Entry->Page.miplevel;
	rx=bx*TileSize + px*MipSize;
	ry=by*TileSize + py*MipSize;
}

CPageAtlas::CPageEntry* CPageAtlas::FindPage( page_id Page, bool Force )
{
	BOOST_FOREACH( CPageEntry& Entry, PageTable )
	{
		// Did we find the right page?
		if ( Entry.IsLoaded && Entry.Page == Page )
			return &Entry;
	}

	if ( Force )
		return MapPage( Page, true );
	else
		return 0;
}

CPageAtlas::CPageEntry* CPageAtlas::PrepareMap( page_id Page, bool Force )
{
	if ( CPageEntry* Result = FindPage( Page ) )
		return Result;

	CPageEntry* Parent = 0;

	// Check if we have a parent (if we need one)
	if ( Page.miplevel < this->MaxLevel )
	{
		Parent = FindPage( Page.predecessor(), Force );

		if ( !Parent )
			return 0;
	}
	
	// Don't try to unmap the parent for the child - exclude parent
	CPageEntry* Current = GetCacheEntry( Parent, Force );


	// Unbind this
	if ( Current->IsLoaded && Current->Parent )
	{
		// Prevent thrashing
		if ( !Force && Current->LastUse == 0 )
		{
			SkippedPages++;
			return 0;
		}

		uint ChildIndex = (Current->Parent->Page.index^Current->Page.index)>>(Current->Page.miplevel*2);
		Current->Parent->Child[ChildIndex] = 0;
		Current->Parent = 0;
	}

	// Bind this
	if ( Parent )
	{
		uint ChildIndex = (Parent->Page.index^Page.index)>>(Page.miplevel*2);
		Parent->Child[ChildIndex]=Current;
		Current->Parent = Parent;

		BOOST_ASSERT( Parent->Page == Page.predecessor() );
	}

	Current->IsLoaded = false;
	Current->IsModified=false;
	Current->IsWriting=false;
	Current->Page = Page;
	Current->LastDraw=0;
	std::fill_n( Current->Child, 4, (CPageEntry*)0 );
	Current->JustUsed = 1;
	Current->LastUse = 0;
	return Current;
}

void CPageAtlas::AsyncMap( page_id Page )
{
	if ( FindPage( Page ) )
		return;

	if ( Page.miplevel < this->MaxLevel )
	{
		AsyncMap( Page.predecessor() );
		PageLoader->ScheduleLoad( Page, IsEditable );
	}
	else
	{
		// Load lowest resolution at once
		MapPage( Page ); 
	}
}

void CPageAtlas::MapReadyPageMemory( page_id Page, ubyte* Source )
{
	if ( !IsEditable )
		return;

	CPageEntry* PageEntry = PrepareMap( Page );


	// Bump out if we already have this
	if ( !PageEntry || PageEntry->IsLoaded )
		return;

	uint x=0,y=0;
	GetPositionInCache(PageEntry,x,y);

	// RGBA8, 4 bytes per texel
	const uint TileSize = PageLoader->GetTileSize();
	std::size_t TextureSize = TileSize*TileSize*4;
	int Flags = squish::kDxt1;

	// Decompress the data to the GL
	DecompBuffer.Bind( GL_PIXEL_UNPACK_BUFFER );
	DecompBuffer.SetGeneratedSubData( GL_PIXEL_UNPACK_BUFFER,0,TextureSize,
		boost::bind( &squish::DecompressImage, _1, TileSize, TileSize, Source, Flags ) );

	// Copy to the texture
	GpuCache.SetSubImage(0, x*TileSize,y*TileSize,TileSize,TileSize,
		GL_RGBA,GL_UNSIGNED_BYTE,0);

	DecompBuffer.Unbind( GL_PIXEL_UNPACK_BUFFER );

	PageEntry->IsLoaded = true;
}

void CPageAtlas::MapReadyPage( page_id Page, GLmm::BufferObject& Pbo, uint Offset )
{
	// Ignore requests from before a switch into editable mode
	if ( IsEditable )
		return;

	CPageEntry* PageEntry = PrepareMap( Page );

	if ( PageEntry && !PageEntry->IsLoaded )
	{
		uint x=0,y=0;
		GetPositionInCache(PageEntry,x,y);

		const uint TileSize = PageLoader->GetTileSize();
		const unsigned char* NullPtr=0;

		Pbo.Bind( GL_PIXEL_UNPACK_BUFFER );
		GpuCache.Bind();
		glCompressedTexSubImage2D( GL_TEXTURE_2D, 0, x*TileSize,y*TileSize,TileSize,TileSize,
			GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, PageLoader->GetPageByteSize(), NullPtr+Offset );

		PageEntry->IsLoaded = true;
		Pbo.Unbind( GL_PIXEL_UNPACK_BUFFER );
	}
}

CPageAtlas::CPageEntry* CPageAtlas::MapPage( page_id Page, bool Force )
{
	CPageEntry* Result = PrepareMap( Page, Force );

	if ( !Result )
		return 0;

	uint x=0,y=0;
	GetPositionInCache(Result,x,y);

	// Just load it right now!
	replay::buffer<unsigned char> ImageData( PageLoader->GetPaddedPageByteSize() );
	PageLoader->LoadDirectly( Page, ImageData.ptr() );

	GpuCache.Bind();
	
	const uint TileSize = PageLoader->GetTileSize();

	if ( this->IsEditable )
	{
		// RGBA8, 4 bytes per texel
		std::size_t TextureSize = TileSize*TileSize*4;
		int Flags = squish::kDxt1;

		// Decompress the data to the GL
		DecompBuffer.Bind( GL_PIXEL_UNPACK_BUFFER );
		DecompBuffer.SetGeneratedSubData( GL_PIXEL_UNPACK_BUFFER,0,TextureSize,
			boost::bind( &squish::DecompressImage, _1, TileSize, TileSize, ImageData.ptr(), Flags ) );

		// Copy to the texture
		GpuCache.SetSubImage(0, x*TileSize,y*TileSize,TileSize,TileSize,
			GL_RGBA,GL_UNSIGNED_BYTE,0);

		DecompBuffer.Unbind( GL_PIXEL_UNPACK_BUFFER );
	}
	else
	{
		glCompressedTexSubImage2D( GL_TEXTURE_2D, 0, x*TileSize,y*TileSize,TileSize,TileSize,
			GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, PageLoader->GetPageByteSize(), ImageData.ptr() );
	}

	Result->IsLoaded = true;

	return Result;
}

void CPageAtlas::BindTexture()
{
	GpuCache.Bind();
}

void CPageAtlas::UpdatePages()
{
	SkippedPages = 0;

	// Map all pages that have been loaded in the background
	PageLoader->Poll();
}

void CPageAtlas::Flush( CPageEntry* Entry, bool Syncronous )
{
	if ( !Entry->IsModified || !GetEditable() )
		return;

	uint TileSize = GetTileSize();
	uint EntryIndex = uint(Entry - PageTable.ptr());
	uint bx = EntryIndex % PageNumSqrt;
	uint by = EntryIndex / PageNumSqrt;

	replay::buffer<ubyte> Buffer(TileSize*TileSize*4);
	ReadFBO.Bind();
	glReadPixels(bx*TileSize,by*TileSize,TileSize,TileSize,
		GL_RGBA,GL_UNSIGNED_BYTE,Buffer.ptr());
	GLmm::Framebuffer::Unbind();

	if ( Entry->ReCompressed.empty() )
	{
		Entry->ReCompressed.alloc(
			PageLoader->GetPaddedPageByteSize()+PageLoader->GetSectorSize());
	}

	ubyte* Target = AlignWith(Entry->ReCompressed.ptr(),
		PageLoader->GetSectorSize());

	squish::CompressImage(Buffer.ptr(),TileSize,TileSize,Target,
		squish::kColourRangeFit|squish::kDxt1);

	Entry->IsModified = false;
	Entry->IsWriting = true;

#ifdef _DEBUG
	std::cout << "Flushing " << Entry->Page.miplevel << "," << Entry->Page.index << std::endl;
#endif

	PageLoader->Write( Entry->Page, Target, Syncronous );
}

void CPageAtlas::ResetDrawCounter( const CPageIterator& Position )
{
	for ( int i=this->MaxLevel; i>int(Position.GetDepth()); --i )
	{
		CPageEntry* Page = Position.GetPage(i);
		if (Page->IsModified)
			Page->LastDraw=0;
	}
}

void CPageAtlas::FlushAll()
{
#ifdef _DEBUG
	std::cout << "Flushing all!" << std::endl;
#endif

	// Update LRU data
	BOOST_FOREACH( CPageEntry& Current, PageTable )
	{
		if ( !Current.IsLoaded )
			continue;

		if ( Current.IsModified  )
		{
			Flush( &Current, true );
		}

		Current.IsModified = false;
	}
}

int CPageAtlas::UpdateCache()
{
	int Result=0;

	// Update LRU data
	BOOST_FOREACH( CPageEntry& Current, PageTable )
	{
		if ( !Current.IsLoaded )
		{
			Result++;
			continue;
		}


		if ( Current.JustUsed )
			Current.LastUse = 0;
		else
		{
			Current.LastUse++;

			if ( !(Current.Child[0]||Current.Child[1]||Current.Child[2]||Current.Child[3]) )
				Result++;
		}


		if ( Current.IsModified )
		{
			if ( Current.LastDraw > 128 )
			{
				Flush( &Current, false );
			}
			else
			{
				Current.LastDraw++;
			}
		}

		Current.JustUsed = 0;
	}

	if ( Result == 0 )
		Result = -SkippedPages;
	
	SkippedPages = 0;

	return Result;
}


vec3 CPageAtlas::ComputeOffsetAndScale( uint Index, const CPageEntry* Entry )
{
	uint EntryIndex = uint(Entry - PageTable.ptr());
	uint bx = EntryIndex % PageNumSqrt;
	uint by = EntryIndex / PageNumSqrt;

	float Scale = 1.f / float(1u<<Entry->Page.miplevel);
	float Res = 1.f/ PageNumSqrt;
	uint px=Pos[Index].first,py=Pos[Index].second;
	uint mask = Entry->Page.miplevel ? ((1<<(Entry->Page.miplevel))-1) : 0;
	px &= mask;
	py &= mask;

	vec3 Offset( (bx+px*Scale)*Res, (by+py*Scale)*Res, Scale*Res );

	return Offset;
}

void CPageAtlas::OnPageWritten(page_id Page)
{
	CPageEntry* Ptr = FindPage(Page);

	BOOST_ASSERT( Ptr != 0 );

	std::cout << "Flushing " << Page.miplevel << "," << Page.index << " completed!" << std::endl;

	Ptr->ReCompressed.clear();

	Ptr->IsModified = false;
	Ptr->IsWriting = false;
	Ptr->LastDraw = 0;

}

uint CPageAtlas::GetTextureSize() const
{
	return PageNumSqrt*PageLoader->GetTileSize();
}

uint CPageAtlas::GetTileSize() const
{
	return PageLoader->GetTileSize();
}

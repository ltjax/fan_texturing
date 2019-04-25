
#ifndef PAGE_ATLAS_HPP
#define PAGE_ATLAS_HPP

#include <vector>
#include <list>
#include <replay/vector2.hpp>
#include <replay/vector3.hpp>
#include <replay/vector4.hpp>
#include <replay/matrix4.hpp>
#include <replay/plane3.hpp>
#include <replay/vector_math.hpp>
#include <replay/table.hpp>
#include <replay/buffer.hpp>
#include "GLmm/Program.hpp"
#include "GLmm/Texture.hpp"
#include "GLmm/BufferObject.hpp"
#include "GLmm/Framebuffer.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

#include "Common.hpp"
#include "Index.hpp"
#include "Misc.hpp"

typedef unsigned int uint;
typedef replay::vector2f vec2;
typedef replay::vector3f vec3;
typedef replay::vector4f vec4;
using boost::scoped_array;

class CPageScheduler;


/** Manages currently loaded texture pages.
*/
class CPageAtlas
{
public:
	typedef boost::filesystem::path PathT;

	CPageAtlas( const PathT& PageFile, uint Index, uint PageNumSqrt );
	~CPageAtlas();

	uint						GetTileSize() const;
	uint						GetTextureSize() const;

	boost::tuple<vec2,float>	GetOffsetAndScale( uint Index, uint LowestLevel=0 );

	void						BindTexture();


	struct CPageEntry
	{
		page_id		Page;
		bool		IsLoaded;

		bool		IsModified;		// Dirty Flag
		bool		IsWriting;

		replay::buffer<ubyte> ReCompressed;

		CPageEntry*	Parent;
		CPageEntry*	Child[4];

		int			JustUsed;
		int			LastUse;
		int			LastDraw;


		CPageEntry() : IsLoaded(false), /*IsLockedForEditing(false), */IsModified(false), IsWriting(false),
			Parent(0), JustUsed(0), LastUse(0), LastDraw(0)
		{
			std::fill_n( Child,4,(CPageEntry*)0 );
		}
	};

	class CPageIterator;

	CPageIterator				GetBegin();

	/** Set the mipmap loading latency.
		When a page fault occurs, the atlas will request only pages up to a maximum difference in
		miplevels to the currently available ones.
		This prevents flooding of very-high detail requests against lower detail requests.
	*/
	void						SetPageFaultLimit( uint Rhs ) { PageFaultLimit = Rhs; }
	
	/** Get the current mipmap loading latency.
		\see SetPageFaultLimit
	*/
	uint						GetPageFaultLimit() const { return PageFaultLimit; }

	/** Make the texture editable.
		The downside is that the cache will work slower and need more memory.
		If this function actually changes the internal editable status,
		it will flush the cache.
	*/
	void						SetEditable( bool Rhs );

	/** Check whether the texture is editable.
		\see GetEditable
	*/
	bool						GetEditable() const;

	void						SetupLevel( const CPageIterator& Iterator, uint Level, ubyte* Offset );

	/** Asyncronously request the mapping a specific page.
		This can fail for various reasons - including cache overflow and buffer map fails.
		\note This function is usually only internally used by SetupLevel.
	*/
	void						AsyncMap( page_id Page );

	CPageEntry*					FindPage( page_id Page, bool Force=false );

	void						ReplaceContent( CPageEntry* Page, GLmm::Framebuffer& Fbo );

	void						UpdatePages();
	int							UpdateCache();

	vec3						ComputeOffsetAndScale( uint Index, const CPageEntry* Entry );

	void						ResetDrawCounter( const CPageIterator& Position );
	uint						GetPageCountSqrt() { return PageNumSqrt; }

private:
	typedef std::pair<uint,uint> uint_pair;

	scoped_ptr<CPageScheduler>	PageLoader;

	void						ComputePixelOffset( uint Index, const CPageEntry* Entry, uint& rx, uint& ry );

	CPageEntry*					MapPage( page_id Page, bool Force=false );
	void						LoaderWorker();
	void						MapReadyPage( page_id Page, GLmm::BufferObject& Pbo, uint Offset );
	void						MapReadyPageMemory( page_id Page, ubyte* Source );

	void						OnPageWritten( page_id Page );
	CPageEntry*					PrepareMap( page_id Page, bool Force=false );
	CPageEntry* 				GetCacheEntry( CPageEntry* Exclude, bool Force );

	void						GetPositionInCache( CPageEntry* Page, uint& x, uint& y );

	void						Flush( CPageEntry* Entry, bool Syncronous );
	void						FlushAll();

	replay::table<CPageEntry>	PageTable;
	scoped_array<uint_pair>		Pos;

	uint						PageNumSqrt;
	uint						MaxLevel;

	uint						ElementCount;

	uint						PageFaultLimit;

	int							SkippedPages;

	GLmm::Texture2D				GpuCache;
	GLmm::BufferObject			DecompBuffer;
	GLmm::Program				DownsampleProgram;

	GLmm::Framebuffer			ReadFBO;


	bool						IsEditable;

};

/** Forward iterator type to find the pages for each index.
*/
class CPageAtlas::CPageIterator
{
public:

	CPageIterator( CPageEntry* Page, std::size_t Count )
	{
		Init( Page, Count );
	}

    CPageIterator(CPageIterator const&) = delete;
    CPageIterator& operator=(CPageIterator const&) = delete;

    CPageIterator(CPageIterator&& Rhs) = default;
    CPageIterator& operator=(CPageIterator&&) = default;

	void operator++()
	{
		Advance();
	}

	CPageEntry* GetPage( std::size_t d ) const { return Section[d].Page; }

	std::size_t GetDepth() const { return Depth; }
	std::size_t GetIndex() const { return Index; }

	bool IsValid() const { return Index < Count; }

private:
	struct SectionType
	{
		CPageEntry*		Page;
		std::size_t		End;

		SectionType() : Page(0), End(0) {}

		void Set( CPageEntry* Page, std::size_t SectionEnd )
		{
			this->Page=Page; this->End=SectionEnd;
		}
	};

	std::unique_ptr<SectionType[]> Section;
	std::size_t					Depth;
	std::size_t					Index;
	std::size_t					Count;

	void Init( CPageEntry* Root, std::size_t Count );
	void Advance();
};

inline
void CPageAtlas::SetupLevel( const CPageIterator& Iterator, uint Level, ubyte* Offset )
{
	const uint Index=uint(Iterator.GetIndex());
	const uint Depth=uint(Iterator.GetDepth());

	// Do we have a page fault?
	if ( Level < Depth )
	{
		// Limit to a few levels down
		if ( Depth-Level > PageFaultLimit )
			Level=Depth-PageFaultLimit;

		// Order it
		AsyncMap( page_id( Level, Index ) );
		Level = Depth;
	}

	// Mark this page as used
	CPageEntry* SelectedPage = Iterator.GetPage(Level);
	SelectedPage->JustUsed |= 1;

	// Have this vertex point to the selected page
	const vec3 Result=ComputeOffsetAndScale(Index, SelectedPage);
	ByteCopy( Result, Offset+Index*sizeof(vec3) );
}

inline
CPageAtlas::CPageIterator CPageAtlas::GetBegin()
{
    return {PageTable.ptr(),ElementCount};
}


#endif

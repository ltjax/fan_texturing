
#ifndef VOXEDIT_INDEX_HPP
#define VOXEDIT_INDEX_HPP

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <boost/filesystem/path.hpp>

typedef unsigned int uint;

struct page_id
{
	uint miplevel;
	uint index;

	bool operator<( const page_id& rhs ) const { return (miplevel > rhs.miplevel) || (miplevel == rhs.miplevel && index < rhs.index); }
	bool operator==( const page_id& rhs ) const { return (miplevel == rhs.miplevel) && (index == rhs.index); }
	bool operator!=( const page_id& rhs ) const { return (miplevel != rhs.miplevel) && (index != rhs.index); }

	page_id() : miplevel( 0xFFFFFFFF ), index( 0xFFFFFFFF ) {}
	page_id( uint m, uint i ) : miplevel(m), index(i)
	{
		uint mask = miplevel ? ((1<<(miplevel*2))-1) : 0;
		index &= ~mask;
	}

	/** Find the predecessor of this page in the mipmap chain.
	*/
	page_id predecessor() const
	{
		page_id Page=*this;
		Page.miplevel++;
		uint Mask = (1<<(Page.miplevel*2))-1;
		Page.index &= ~Mask;
		return Page;
	}
};


inline uint	ContiguousIndex( page_id Page, uint ElementCount )
{
	uint Base=0;
	uint p=ElementCount;

	// FIXME: should be possible to get rid of the loop here
	for ( uint i=0; i<Page.miplevel; ++i )
	{
		Base += p;
		p=(p+3)/4;
	}

	return Base + (Page.index >> (Page.miplevel*2));
}

inline void SplitIndex( uint mixed, uint& x, uint& y )
{
	x=0;
	y=0;
	for ( uint i=0; i <32; i+=2 )
	{
		x = x << 1;
		y = y << 1;			
		x |= (mixed>>(30u-i))&1;
		y |= (mixed>>(31u-i))&1;
	}
}

inline uint IntegerLog( uint n )
{
#ifdef _MSC_VER
	unsigned long Result=0;
	if ( !_BitScanReverse(&Result,n) )
		return 0;
	return Result+1;
#else
	uint i;
	for ( i=0; i<32; ++i )
	{
		if ( (1u<<i)>=n )
			break;
	}
	return i;
#endif
}

inline uint GetMaximumLevel( uint ElementCount )
{
	uint MaxX, MaxY;
	SplitIndex( ElementCount, MaxX, MaxY );
	return IntegerLog( std::max( MaxX, MaxY ) );
}

inline uint	ContiguousIndexCount( uint ElementCount )
{
	return ContiguousIndex( page_id( GetMaximumLevel(ElementCount),0 ), ElementCount )+1;
}

#endif

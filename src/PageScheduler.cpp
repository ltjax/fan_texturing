
#include "Misc.hpp"
#include "PageScheduler.hpp"
#include <replay/bstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/fstream.hpp>
#include <replay/buffer.hpp>


CPageScheduler::CPageScheduler(
		boost::filesystem::path PageFilename,
		uint ElementCount,
		uint MaxRequests,
		GPUReadCallbackType GPUReadCallback,
		CPUReadCallbackType CPUReadCallback,
		WriteCallbackType WriteCallback
	)
: ElementCount( ElementCount ), MaxRequests( MaxRequests ),
GPUReadCallback( GPUReadCallback ), CPUReadCallback(CPUReadCallback), WriteCallback(WriteCallback)
{


	uint IndexCount = ContiguousIndexCount( ElementCount );
	Offsets.resize( IndexCount );

	// Read the header
	boost::filesystem::ifstream File(PageFilename,std::ios::binary);

	replay::buffer<uint> HeaderData(5);
	File.read( reinterpret_cast<char*>(HeaderData.ptr()), 5*sizeof(uint) );

	// Check the header and copy info
	if ( HeaderData[0] != 0x600DF00D || HeaderData[1] != ElementCount )
		throw std::runtime_error( "Trying to load invalid texture page file." );

	this->TileSize = HeaderData[2];
	this->PageByteSize = HeaderData[3];
	this->Alignment = HeaderData[4];

	// Read the offsets from the header
	File.read( reinterpret_cast<char*>(&(Offsets[0])), IndexCount*sizeof(uint) );
	File.close();

	// Check offsets for alignemnt
	for ( std::size_t i=0; i<IndexCount; ++i )
		if ( Offsets[i] % Alignment )
			throw std::runtime_error( "Page is not aligned." );

	// Check whether sector sizes match
	DWORD SectorsPerCluster=0, BytesPerSector=0,
		NumberOfFreeClusters=0,TotalNumberOfClusters=0;
	GetDiskFreeSpace( PageFilename.root_name().string().c_str(),
		&SectorsPerCluster, &BytesPerSector,
		&NumberOfFreeClusters, &TotalNumberOfClusters );

	if ( BytesPerSector > Alignment || Alignment%BytesPerSector )
		throw std::runtime_error( "Page alignment isn't compatible with the given disk." );

	SectorSize = BytesPerSector;

	// Open the same file for paging with disabled OS buffering
	// For this to work, two conditions have to be met:
	// 1. Read/Write offsets and lengths need to be aligned with the mediums sector size
	// 2. Target/Source addresses have to be aligned with the mediums sector size as well
	DWORD FileFlags = FILE_FLAG_OVERLAPPED|FILE_FLAG_RANDOM_ACCESS|
		FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH;
	HANDLE FileHandle = ::CreateFile( PageFilename.string().c_str(),
		GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FileFlags, NULL );

	if ( FileHandle == INVALID_HANDLE_VALUE )
	{
		DWORD Error = GetLastError();
		throw std::runtime_error( "Unable to open texture page file." );
	}

	PageFile.reset( new boost::asio::windows::random_access_handle( IoService, FileHandle ) );
}

CPageScheduler::~CPageScheduler()
{
	PageFile.reset();
}

uint CPageScheduler::GetPaddedPageByteSize() const
{
	// round up to the next page size multiple
	return AlignWith(PageByteSize,Alignment);
}

void CPageScheduler::ScheduleWrite(page_id Page, ubyte* Input )
{
	Write( Page, Input, false );
}

void CPageScheduler::Write(page_id Page, ubyte* Input, bool Syncronous )
{
	typedef unsigned char byte;

	if ( !IsAligned(Input,SectorSize) )
 		throw std::runtime_error( "Input pointer is not aligned." );

	uint Index = ContiguousIndex( Page,ElementCount );
	uint BlockSize = AlignWith(PageByteSize,Alignment);

	uint CurrentOffset = this->Offsets[Index];

	if ( Syncronous )
	{
		boost::asio::write_at( *PageFile, CurrentOffset,
			boost::asio::buffer( Input, BlockSize ) );
	}
	else
	{
		boost::asio::async_write_at( *PageFile, CurrentOffset,
			boost::asio::buffer( Input, BlockSize ),
			boost::bind( &CPageScheduler::OnAsyncWrite, this, _1, _2, Page ) );
	}
}

void CPageScheduler::ScheduleLoad( page_id Page, bool LoadToMemory )
{
	// Is the maximum number of requests reached yet? - Only check if it's unequal to 0
	if ( MaxRequests && Requests.size() >= MaxRequests )
		return;

	std::pair< RequestIterator, bool > Info = Requests.insert( std::make_pair(Page,CRequestTag()) );

	// Check if we already have this request
	if ( Info.second == false )
		return;

	uint			Index = ContiguousIndex( Page, ElementCount );
	const uint		PaddedPageSize = GetPaddedPageByteSize();

	CRequestTag&	Request( Info.first->second );
	byte*			TargetAddr = 0;

	if ( LoadToMemory )
	{
		Request.Memory.reset( new replay::buffer<byte>( PaddedPageSize+SectorSize ) );
		TargetAddr = Request.Memory->ptr();
	}
	else
	{
		Request.Store.reset( new GLmm::BufferObject );
		Request.Store->SetData( GL_PIXEL_UNPACK_BUFFER, PaddedPageSize+SectorSize, NULL, GL_STREAM_DRAW );

		TargetAddr = Request.Store->Map( GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY );
		GLmm::BufferObject::Unbind( GL_PIXEL_UNPACK_BUFFER );

		// Did the map fail? - Ignore silently
		if ( !TargetAddr )
		{
			Requests.erase( Info.first );
			return;
		}		
	}
	
	// Align the memory destination with the sector size
	uint MemoryOffset = TargetAddr-static_cast<byte*>(0);
	Request.Offset = AlignWith(MemoryOffset, SectorSize)-MemoryOffset;
	TargetAddr += Request.Offset;

	uint CurrentOffset = Offsets[Index];

	// Schedule the actual read
	boost::asio::async_read_at( *PageFile, CurrentOffset,
		boost::asio::buffer( TargetAddr, PaddedPageSize ),
		boost::bind( &CPageScheduler::OnAsyncRead,
		this, _1, _2, Info.first ) );
}

void CPageScheduler::Poll()
{
	IoService.poll();
}

void CPageScheduler::OnAsyncWrite( 
	const boost::system::error_code& Error,
	std::size_t BytesTransferred,
	page_id Page )
{
	if ( Error )
		throw std::runtime_error( "Error writing file!" );

	if ( BytesTransferred == 0 || BytesTransferred != PageByteSize )
		throw std::runtime_error( "No or incorrect number of bytes written!" );
	
	WriteCallback( Page );
}

void CPageScheduler::OnAsyncRead( 
	const boost::system::error_code& Error, // Result of operation.
	std::size_t BytesTransferred,
	RequestIterator Id )
{
	if ( Error )
		throw std::runtime_error( "Error reading file!" );

	if ( BytesTransferred == 0 || BytesTransferred != PageByteSize )
		throw std::runtime_error( "No or incorrect number of bytes read!" );

	page_id Page = Id->first;
	CRequestTag& Tag( Id->second );


	if ( Tag.Memory )
	{
		CPUReadCallback( Page, Tag.Memory->ptr()+Tag.Offset );
	}
	else
	{
		// Make sure the buffer hasn't been corrupted before using it
		if ( Tag.Store->Unmap( GL_PIXEL_UNPACK_BUFFER ) )
		{
			GPUReadCallback( Page, *Tag.Store, Tag.Offset );
		}
	}

	// Remove the request
	Requests.erase( Id );
}


void CPageScheduler::LoadDirectly( page_id Page, unsigned char* Target )
{
	uint Index = ContiguousIndex( Page, ElementCount );
	uint Offset = Offsets[Index];

	replay::buffer<unsigned char> TempTarget( SectorSize+GetPaddedPageByteSize() );

	unsigned char* NullPtr = 0;
	unsigned char* Buffer = AlignWith( TempTarget.ptr()-NullPtr, SectorSize )+NullPtr;

	std::size_t BytesRead = boost::asio::read_at( *PageFile, Offset,
		boost::asio::buffer(Buffer,GetPaddedPageByteSize()) );

	if ( BytesRead != PageByteSize )
		throw std::runtime_error( "Direct loading failed." );

	std::copy( Buffer, Buffer+GetPageByteSize(), Target );
}

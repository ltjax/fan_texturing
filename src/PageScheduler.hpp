
#ifndef ASIO_PAGE_LOADER_HPP
#define ASIO_PAGE_LOADER_HPP

#include <list>
#include <queue>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <replay/pixbuf.hpp>
#include <replay/buffer.hpp>

#include "Common.hpp"
#include "Index.hpp"
#include "GLmm/BufferObject.hpp"

#include <boost/asio/windows/random_access_handle.hpp>
#include <boost/asio.hpp>



class CPageScheduler
{
public:
	typedef boost::function<void(page_id,GLmm::BufferObject&,uint)> GPUReadCallbackType;
	typedef boost::function<void(page_id,ubyte*)> CPUReadCallbackType;
	typedef boost::function<void(page_id)> WriteCallbackType;

	CPageScheduler( 
		boost::filesystem::path PageFilename,
		uint ElementCount,
		uint MaxRequests,
		GPUReadCallbackType GPUReadCallback,
		CPUReadCallbackType CPUReadCallback,
		WriteCallbackType WriteCallback );
	~CPageScheduler();
	
	void						ScheduleLoad( page_id Page, bool LoadToMemory );
	void						ScheduleWrite( page_id Page, ubyte* Input );

	void						Write( page_id Page, ubyte* Input, bool Syncronous=true );

	void						Poll();

	void						LoadDirectly( page_id Page, unsigned char* Target );


	uint						GetPageByteSize() const { return PageByteSize; }

	uint						GetPaddedPageByteSize() const;

	uint						GetAlignment() const { return Alignment; }
	uint						GetSectorSize() const { return SectorSize; }

	uint						GetTileSize() const { return TileSize; }

private:

	struct CRequestTag
	{
		boost::shared_ptr<GLmm::BufferObject>	Store;
		shared_ptr<replay::buffer<ubyte>>		Memory;

		uint									Offset;
	};

	typedef std::map<page_id,CRequestTag> RequestMapType;
	typedef RequestMapType::iterator RequestIterator;

	GPUReadCallbackType			GPUReadCallback;
	CPUReadCallbackType			CPUReadCallback;
	WriteCallbackType			WriteCallback;

	void						OnAsyncRead( 
									const boost::system::error_code& Error, // Result of operation.
									std::size_t BytesTransferred,
									RequestIterator Id );

	void						OnAsyncWrite(
									const boost::system::error_code& Error,
									std::size_t BytesTransferred,
									page_id Page );

	uint						ElementCount;
	uint						MaxRequests;
	RequestMapType				Requests;

	uint						TileSize;
	uint						PageByteSize;
	uint						Alignment;

	uint						SectorSize;

	scoped_ptr<boost::asio::windows::random_access_handle>
								PageFile;

	boost::asio::io_service		IoService;

	std::vector<uint>			Offsets;
};

#endif

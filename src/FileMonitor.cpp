
#include <windows.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/utility.hpp>
#include "FileMonitor.hpp"
#define RESULT_BUFFER_SIZE 4096
namespace pt = boost::posix_time;
namespace ba = boost::algorithm;


struct CFileMonitor::InternalType
{
	HANDLE		DirectoryHandle;
	HANDLE		NotifyEvent;
	OVERLAPPED	OverlappedIO;

	bool		CountdownStarted;
	pt::ptime	CountdownTime;
	pt::time_duration CountdownLength;
	PathT		BasePath;

	std::vector< PathT >	FilesChanged;

	boost::signal< void ( const std::vector< PathT >& ) > SignalChange;

	InternalType()
	: DirectoryHandle( INVALID_HANDLE_VALUE ), NotifyEvent( NULL ), CountdownStarted( false )
	{
		std::fill_n( reinterpret_cast< char* >( &OverlappedIO ), 
			sizeof( OverlappedIO ), 0 );
	}
};

CFileMonitor::CFileMonitor()
: Data( new InternalType )
{
	ResultBuffer.reset( new char[ RESULT_BUFFER_SIZE ] );
	Data->CountdownLength = pt::milliseconds(500);
}

CFileMonitor::~CFileMonitor()
{
	if ( Data->DirectoryHandle != INVALID_HANDLE_VALUE )
		CloseHandle( Data->DirectoryHandle );

	if ( Data->NotifyEvent != NULL )
		CloseHandle( Data->NotifyEvent );
}

void CFileMonitor::SetCountdownLength( int Length )
{
	Data->CountdownLength = pt::milliseconds( Length );
}

boost::signal< void ( const std::vector< CFileMonitor::PathT >& ) >&
CFileMonitor::SignalChange()
{
	return Data->SignalChange;
}

void CFileMonitor::Start( const PathT& Path )
{
	if ( Data->DirectoryHandle != INVALID_HANDLE_VALUE )
		throw std::runtime_error( "FileMonitor already started." );

	Data->BasePath = Path;

	// Get a handle for the directory to watch
	Data->DirectoryHandle = CreateFile( Path.string().c_str(),
		FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

	// Create an event for the polling
	Data->NotifyEvent = CreateEvent(NULL, FALSE, FALSE, "FileChangedEvent");

	if ( Data->NotifyEvent == NULL )
		throw std::runtime_error( "FileMonitor failed to create event." );

	Data->OverlappedIO.hEvent = Data->NotifyEvent;

	Listen();
}

bool CFileMonitor::Listen()
{
	DWORD Unused = 0;

	BOOL Result = ReadDirectoryChangesW( Data->DirectoryHandle, 
		ResultBuffer.get(), RESULT_BUFFER_SIZE,	TRUE,
		FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
		&Unused, &Data->OverlappedIO, NULL );

	if ( Result == 0 )
	{
		LPVOID buffer;

		if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&buffer, 0, NULL) )
			printf("ReadDirectoryChangesW failed with '%s'\n", (const char*)buffer);
		else
			printf("Couldn't format error msg for ReadDirectoryChangesW failure.\n");

		LocalFree(buffer);
	}

	return (Result != 0);
}
void CFileMonitor::AddPath( const char* Filename )
{
	PathT Path( Filename );

	// append the basepath if needed
	if ( !ba::starts_with( Path, Data->BasePath ) )
		Path = Data->BasePath / Path;

	std::vector< PathT >& Vector( Data->FilesChanged );

	if ( std::find( Vector.begin(), Vector.end(), Path ) ==
		Vector.end() )
	{
		Vector.push_back( Path );

		if ( !Data->CountdownStarted )
		{
			Data->CountdownStarted = true;
			Data->CountdownTime = pt::microsec_clock::universal_time();
		}
	}
}

bool CFileMonitor::Update()
{
	if ( Data->CountdownStarted )
	{
		// calculate the time the timer has been running for
		pt::time_duration Time = pt::microsec_clock::universal_time()
			- Data->CountdownTime;

		if ( Time > Data->CountdownLength )
		{
			Data->SignalChange( boost::ref( Data->FilesChanged ) );
			Data->CountdownStarted = false;
			Data->FilesChanged.clear();
		}
	}

	DWORD BytesWritten = 0;
	BOOL Result = GetOverlappedResult( Data->NotifyEvent,
		&Data->OverlappedIO, &BytesWritten, FALSE );

	// No results yet?
	if ( Result == FALSE )
	{
		assert( GetLastError() == ERROR_IO_INCOMPLETE );
		return true;
	}

	// We got something
	const char* CurrentEntry = ResultBuffer.get();
	char Filename[ 256 ];
	for ( bool Done = false; !Done; )
	{
		const FILE_NOTIFY_INFORMATION* FileInfo = 
			reinterpret_cast< const FILE_NOTIFY_INFORMATION* >(CurrentEntry);

		if ( FileInfo->Action == FILE_ACTION_MODIFIED ||
			 FileInfo->Action == FILE_ACTION_RENAMED_NEW_NAME )
		{
			// Convert to ASCII
			wcstombs( Filename, FileInfo->FileName, 256 );
			Filename[ min( FileInfo->FileNameLength / 2, 255 ) ] = 0;

			AddPath( Filename );
		}

		// If there's another one, go there
		Done = ( FileInfo->NextEntryOffset == 0 );
		CurrentEntry += FileInfo->NextEntryOffset;
	}

	return Listen();
}


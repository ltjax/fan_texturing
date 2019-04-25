/*

Cataclysm-Software Disaster Engine
----------------------------------

Repository-Info:
$Id: Model.hpp 750 2009-02-03 16:16:05Z ltjax $

Copyright:
Marius Elvert (marius.elvert@cataclysm-software.com) 2006-2007

*/

#ifndef FILE_MONITOR_HEADER
#define FILE_MONITOR_HEADER

#include <boost/filesystem/path.hpp>
#include <boost/signal.hpp>

/** Watches a directory tree for file changes and reports them.
*/
class CFileMonitor :
	public boost::noncopyable
{
public:
	typedef boost::filesystem::path PathT;

	CFileMonitor();
	~CFileMonitor();

	/** Start the monitoring for a base path.
	*/
	void Start( const PathT& Path );

	/** Get the signal to be called when a file change happens.
	*/
	boost::signal< void ( const std::vector< PathT >& ) >&
		SignalChange();

	/** Poll for changes.
	*/
	bool	Update();

	/** Set the length that the monitor will wait before reporting files in ms.
		The default is 500ms. Duplicates within the countdown time are automatically removed.
	*/
	void	SetCountdownLength( int Length );

private:


	bool	Listen();
	void	AddPath( const char* Filename );

	struct InternalType;

	boost::scoped_ptr< InternalType > Data;
	boost::scoped_array< char >	ResultBuffer;

};


#endif // FILE_MONITOR_HEADER
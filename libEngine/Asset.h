#ifndef Asset_h__
#define Asset_h__

#include "IntegerType.h"
#include "FastDelegate/FastDelegate.h"

class Asset
{
public:

	char const* getPath(){ return mPath.c_str(); }

	virtual void nodifyModify() = 0;
	virtual void nodifyChangePath() = 0;
	virtual void nodifyDelete() = 0;

	std::string mPath;
};

class Dir
{

};
class Win32FileModifyMonitor
{
public:

	typedef fastdelegate::FastDelegate< 
		void ( WatchDir* dir , wchar_t const* fileName , int fileNameLen , int action ) 
	> NotifyCallback;

	Win32FileModifyMonitor()
	{
		mhIOCP = NULL;
	}
	bool init()
	{
		TCHAR const* value[] = 
		{
			SE_BACKUP_NAME ,
			SE_RESTORE_NAME ,
			SE_CHANGE_NOTIFY_NAME ,
		}
		if ( !EnablePrivilegeValue( value , sizeof(value)/sizeof(value[0]) ) )
			return false;

		int nThreads = 3;
		mhIOCP = ::CreateIoCompletionPort((HANDLE)INVALID_HANDLE_VALUE,NULL,0,nThreads);
		if ( !mhIOCP )
			return false;

		return true;
	}

	static int const MAX_WATCH_BUF_SIZE = 10;
	struct WatchDir
	{
		uint8        buf[ 512 ];
		std::wstring dirPath;
		HANDLE       handle;
		bool         bSubTree;
		DWORD        notifyFilters;
		OVERLAPPED   overlap;
	};

	enum Status
	{
		eOK ,
		eOutOfMemory ,
		eOpenDirFail ,
		eReadDirError ,
		eIOCPError ,

		eDirNoChange ,
	};


	Status addPath( wchar_t const* pPath , bool bSubTree )
	{
		if ( !mhIOCP )
			return eIOCPError;

		DWORD notifyFilters = 
			FILE_NOTIFY_CHANGE_FILE_NAME | 
			FILE_NOTIFY_CHANGE_LAST_WRITE |
			FILE_NOTIFY_CHANGE_CREATION;

		WatchDir* pDir = new WatchDir;
		pDir->bSubTree = bSubTree;

		///////////////////////////////////////////////////////////////////////
		// Open handle to the directory to be monitored, note the FILE_FLAG_OVERLAPPED

		pDir->handle = CreateFileW( pPath ,
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
			NULL,                               
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL );
		if ( pDir->handle == INVALID_HANDLE_VALUE )
		{
			return eOpenDirFail;
		}

		///////////////////////////////////////////////////////////////////////

		// Allocate notification buffers (will be filled by the system when a
		// notification occurs

		memset(&pDir->overlap ,  0, sizeof(pDir->overlap));
		///////////////////////////////////////////////////////////////////////

		// Associate directory handle with the IO completion port

		if ( CreateIoCompletionPort(pDir->handle , mhIOCP , (ULONG_PTR)pDir->handle , 0) == NULL )
		{
			CloseHandle( pDir->handle );
			delete pDir;
			return eIOCPError;
		}


		///////////////////////////////////////////////////////////////////////

		// Start monitoring for changes

		DWORD dwBytesReturned	= 0;
		pDir->dirPath = pPath;

		if (!ReadDirectoryChangesW(
				pDir->handle ,
				pDir->buf ,
				sizeof( pDir->buf ) ,
				pDir->bSubTree , //Sub Tree
				notifyFilters , 
				&dwBytesReturned ,
				&pDir->overlap ,
				NULL ) )
		{
			CloseHandle( pDir->handle );
			delete pDir;
			return eReadDirError;
		}

		///////////////////////////////////////////////////

		//success, add to directories vector

		//m_vecDirs.push_back(pDir);
		return eOK;
	}

	int checkDirStatus( NotifyCallback& cb , uint32 timeout )
	{
		DWORD		dwBytesXFered = 0;
		ULONG_PTR	ulKey = 0;
		OVERLAPPED*	pOl;
		///////////////////////////////////////////////////////
		if ( !GetQueuedCompletionStatus( mhIOCP , &dwBytesXFered, &ulKey, &pOl, timeout) )
		{
			if ( GetLastError() == WAIT_TIMEOUT )
				return eDirNoChange;
			return eIOCPError;
		}

		///////////////////////////////////////////////////////

		WatchDir* pDir;

		unsigned uiCount = m_vecDirs.size();
		unsigned nIndex;

		for (nIndex = 0; nIndex < uiCount; ++nIndex)
		{
			pDir = m_vecDirs.at(nIndex);
			if (ulKey == (ULONG_PTR)pDir->hFile)
				break;
		}

		if (nIndex == uiCount) //not found
			return E_FILESYSMON_ERRORUNKNOWN;


		//////////////////////////////////////////////////////
		FILE_NOTIFY_INFORMATION* pIter = reinterpret_cast< FILE_NOTIFY_INFORMATION* >( pDir->buf );

		while (pIter)
		{
			pIter->FileName[pIter->FileNameLength / sizeof(TCHAR)] = 0;

		
			int num = WideCharToMultiByte( );
			cb( pDir , pIter->FileName , pIter->FileNameLength , pIter->Action );

			if( pIter->NextEntryOffset == 0 )
				break;	

			if ((DWORD)((BYTE*)pIter - (BYTE*)pDir->buf) > ( MAX_WATCH_BUF_SIZE * sizeof(FILE_NOTIFY_INFORMATION)) )
				pIter = pDir->buf;

			pIter = (PFILE_NOTIFY_INFORMATION) ((LPBYTE)pIter + pIter->NextEntryOffset);
		}


		//////////////////////////////////////////////////////

		// Continue reading for changes

		DWORD dwBytesReturned = 0;

		if (!ReadDirectoryChangesW(
			pDir->handle ,
			pDir->buf ,
			sizeof( pDir->buf ),
			pDir->bSubTree,
			pDir->dwNotifyFilters, 
			&dwBytesReturned ,
			&pDir->overlap ,
			NULL))
		{
			m_nLastError = GetLastError();
			RemovePath(nIndex);

			return E_FILESYSMON_ERRORREADDIR;
		}



		return E_FILESYSMON_SUCCESS;

	}

	static int EnablePrivilegeValue( LPCTSTR valueNames[], int numValue )
	{
		int result = 0;

		HANDLE hToken = NULL;
		if ( !OpenProcessToken(GetCurrentProcess(),  TOKEN_ADJUST_PRIVILEGES, &hToken) )
			return 0;

		for( int i = 0 ; i < numValue ; ++i )
		{
			TOKEN_PRIVILEGES tp = { 1 };        
			if ( LookupPrivilegeValue( NULL, valueNames[i] ,  &tp.Privileges[0].Luid ) )
			{
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

				AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
				result += ( GetLastError() == ERROR_SUCCESS ) ? 1 : 0;		
			}
		}

		CloseHandle(hToken);
		return result;
	}

	struct DirCmp
	{
		bool operator()( WatchDir const* lhs , WatchDir* const rhs )
		{
			return lhs->dirPath < rhs->dirPath;
		}
	};
	typedef std::set< WatchDir* , DirCmp > WatchDirSet;


	WatchDirSet mDirSet;
	HANDLE      mhIOCP;
};

class AssetManager
{
public:
	void addAsset( Asset* asset ){}
	void removeAsset( Asset* asset ){}




	struct StrCmp
	{
		bool operator ()( char const* s1 , char const* s2 ) const
		{
			return strcmp( s1 , s2 ) < 0;
		}
	};
	typedef std::map< char const* , Asset* , StrCmp >  AssetMap;
	AssetMap mMap;
};


#endif // Asset_h__

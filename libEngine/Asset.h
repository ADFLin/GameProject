#ifndef Asset_h__
#define Asset_h__

#include "PlatformConfig.h"
#include "IntegerType.h"
#include "FastDelegate/FastDelegate.h"

#include <set>
#include <map>
#include <vector>


enum class FileAction
{
	Rename,
	Modify,
	Remove,
	Created,
};

class AssetBase
{
public:
	virtual void getDependentFilePaths(std::vector< std::wstring >& paths) {}
protected:
	friend class AssetManager;
	virtual void postFileModify(FileAction action){}


	static std::wstring CharToWChar(const char *c)
	{
		const size_t cSize = strlen(c)+1;
		wchar_t buff[1024];
		mbstowcs(buff, c, cSize);
		return buff;
	}
};

#if SYS_PLATFORM_WIN

#include "Win32Header.h"

class Win32FileModifyMonitor
{
public:

	Win32FileModifyMonitor()
	{
		mhIOCP = NULL;
	}
	bool init();
	void cleanup();

	static int const MAX_WATCH_BUF_SIZE = 10;
	struct DirMonitorInfo
	{
		static int const ReserveStringLength = 32;
		uint8        buf[ ( sizeof(FILE_NOTIFY_INFORMATION) + ReserveStringLength * sizeof(WCHAR) )* MAX_WATCH_BUF_SIZE ];
		std::wstring dirPath;
		HANDLE       handle;
		bool         bSubTree;
		DWORD        notifyFilters;
		OVERLAPPED   overlap;
		int          refCount;
	};


	void destroyInfo(DirMonitorInfo* dmInfo)
	{
		::CloseHandle(dmInfo->handle);
		delete dmInfo;
	}
	typedef fastdelegate::FastDelegate<
		void( wchar_t const* path , FileAction action )
	> NotifyCallback;

	NotifyCallback onNotify;

	enum Status
	{
		eOK ,
		eOutOfMemory ,
		eOpenDirFail ,
		eReadDirError ,
		eIOCPError ,
		eUnknownError ,
		eDirNoChange ,
	};



	Status addDirectoryPath( wchar_t const* pPath , bool bSubTree );
	bool removeDirectoryPathRef(wchar_t const* pPath);
	Status checkDirectoryStatus(uint32 timeout );

	static int EnablePrivilegeValue( TCHAR const* valueNames[], int numValue );

	struct DirCmp
	{
		bool operator()(DirMonitorInfo const* lhs, DirMonitorInfo* const rhs) const
		{
			return lhs->dirPath < rhs->dirPath;
		}
	};

	struct StrCmp
	{
		bool operator ()(wchar_t const* s1, wchar_t const* s2) const
		{
			return ::wcscmp(s1, s2) < 0;
		}
	};
	typedef std::map< wchar_t const*, DirMonitorInfo* , StrCmp > WatchDirMap;

	DWORD       mLastError;
	WatchDirMap mDirMap;
	HANDLE      mhIOCP;
};

#endif //SYS_PLATFORM_WIN

class AssetManager
{
public:
	bool init();
	void tick();
	bool registerAsset(AssetBase* asset);

	void unregisterAsset(AssetBase* asset);

	
	struct StrCmp
	{
		bool operator ()( wchar_t const* s1 , wchar_t const* s2 ) const
		{
			return ::wcscmp( s1 , s2 ) < 0;
		}
	};
	typedef std::vector< AssetBase* > AssetList;
	typedef std::map< std::wstring , AssetList >  AssetMap;
	AssetMap mAssetMap;

	struct FileModifyInfo
	{
		std::wstring path;
		FileAction  action;
	};

#if SYS_PLATFORM_WIN
	Win32FileModifyMonitor mFileModifyMonitor;
	void procDirModify(wchar_t const* path, FileAction action);
#endif
};


#endif // Asset_h__

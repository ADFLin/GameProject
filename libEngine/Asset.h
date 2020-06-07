#pragma once
#ifndef Asset_H_5448C020_E85D_4A18_B426_922C38A0BB67
#define Asset_H_5448C020_E85D_4A18_B426_922C38A0BB67

#include "AssetViewer.h"
#include "PlatformConfig.h"
#include "Core/IntegerType.h"
#include "FastDelegate/FastDelegate.h"

#include <set>
#include <map>
#include <vector>

#if SYS_PLATFORM_WIN

#include "WindowsHeader.h"

class Win32FileModifyMonitor
{
public:

	Win32FileModifyMonitor()
	{
		mhIOCP = NULL;
		mLastError = NO_ERROR;
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
		void( wchar_t const* path , EFileAction action )
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

class AssetManager : public IAssetViewerRegister
{
public:
	bool init();
	void cleanup();
	void tick(long time);
	bool registerViewer(IAssetViewer* asset) override;
	void unregisterViewer(IAssetViewer* asset) override;

	
	struct StrCmp
	{
		bool operator ()( wchar_t const* s1 , wchar_t const* s2 ) const
		{
			return ::wcscmp( s1 , s2 ) < 0;
		}
	};
	typedef std::vector< IAssetViewer* > AssetList;
	typedef std::map< std::wstring , AssetList >  AssetMap;
	AssetMap mAssetMap;

	struct FileModifyInfo
	{
		std::wstring path;
		EFileAction  action;
	};

#if SYS_PLATFORM_WIN
	Win32FileModifyMonitor mFileModifyMonitor;
	void procDirModify(wchar_t const* path, EFileAction action);
#endif
};


#endif // Asset_H_5448C020_E85D_4A18_B426_922C38A0BB67

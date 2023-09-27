#include "Asset.h"

#include "FileSystem.h"
#include "LogSystem.h"

#include <cassert>

#if SYS_PLATFORM_WIN

#include "Shlwapi.h"
#include "Core/ScopeGuard.h"

#include "Misc/CStringWrapper.h"
#include <unordered_map>
#include <map>

#pragma comment(lib,"Shlwapi.lib")

class WinodwsFileMonitor : public IPlatformFileMonitor
{
public:

	WinodwsFileMonitor()
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
		static int const BufSize = (sizeof(FILE_NOTIFY_INFORMATION) + ReserveStringLength * sizeof(WCHAR))* MAX_WATCH_BUF_SIZE;

		uint8        buf[BufSize];
		std::wstring dirPath;
		HANDLE       handle;
		bool         bIncludeChildDir;
		DWORD        notifyFilters;
		OVERLAPPED   overlap;
		int          refcount;

		void release()
		{
			::CloseHandle(handle);
		}
	};


	void destroyInfo(DirMonitorInfo* dmInfo)
	{
		dmInfo->release();
		delete dmInfo;
	}


	FileNotifyCallback onNotify;

	EFileMonitorStatus::Type addDirectoryPath(wchar_t const* pPath, bool bIncludeChildDir);
	bool   removeDirectoryPath(wchar_t const* pPath, bool bCheckReference);
	EFileMonitorStatus::Type  checkDirectoryStatus(uint32 timeout);

	static int EnablePrivilegeValue(TCHAR const* valueNames[], int numValue);

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
	typedef std::map< wchar_t const*, DirMonitorInfo*, StrCmp> WatchDirMap;

	DWORD       mLastError;
	WatchDirMap mDirMap;
	HANDLE      mhIOCP;

	void tick(long time) override
	{
		checkDirectoryStatus(0);
	}


	void setFileNotifyCallback(FileNotifyCallback callback) override
	{
		onNotify = callback;
	}

	void release() override
	{
		cleanup();
		delete this;
	}

};


bool WinodwsFileMonitor::init()
{
	TCHAR const* value[] =
	{
		SE_BACKUP_NAME ,
		SE_RESTORE_NAME ,
		SE_CHANGE_NOTIFY_NAME ,
	};
	if( !EnablePrivilegeValue(value, sizeof(value) / sizeof(value[0])) )
		return false;

	int nThreads = 3;
	mhIOCP = ::CreateIoCompletionPort((HANDLE)INVALID_HANDLE_VALUE, NULL, 0, nThreads);
	if( !mhIOCP )
		return false;

	return true;
}

void WinodwsFileMonitor::cleanup()
{
	for( auto& pair : mDirMap )
	{
		destroyInfo(pair.second);
	}
	mDirMap.clear();
	::CloseHandle(mhIOCP);
	mhIOCP = NULL;
}

EFileMonitorStatus::Type WinodwsFileMonitor::addDirectoryPath(wchar_t const* pPath, bool bIncludeChildDir)
{
	{
		auto iter = mDirMap.find(pPath);
		if(  iter != mDirMap.end() )
		{
			auto info = iter->second;
			info->refcount += 1;
			return EFileMonitorStatus::OK;
		}
	}

	if( !mhIOCP )
		return EFileMonitorStatus::IOError;


	std::unique_ptr< DirMonitorInfo > info = std::make_unique<DirMonitorInfo>();
	const DWORD notifyFilters =
		FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_LAST_WRITE |
		FILE_NOTIFY_CHANGE_CREATION;
	info->refcount = 1;
	info->notifyFilters = notifyFilters;
	info->bIncludeChildDir = bIncludeChildDir;

	///////////////////////////////////////////////////////////////////////
	// Open handle to the directory to be monitored, note the FILE_FLAG_OVERLAPPED

	info->handle = CreateFileW(pPath,
							   FILE_LIST_DIRECTORY,
							   FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
							   NULL,
							   OPEN_EXISTING,
							   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
							   NULL);
	if(info->handle == INVALID_HANDLE_VALUE )
	{
		return EFileMonitorStatus::OpenDirFail;
	}

	ON_SCOPE_EXIT
	{
		if (info)
		{
			info->release();
		}
	};

	///////////////////////////////////////////////////////////////////////

	// Allocate notification buffers (will be filled by the system when a
	// notification occurs
	memset(&info->overlap, 0, sizeof(info->overlap));
	///////////////////////////////////////////////////////////////////////

	// Associate directory handle with the IO completion port

	if( ::CreateIoCompletionPort(info->handle, mhIOCP, (ULONG_PTR)info->handle, 0) == NULL )
	{
		return EFileMonitorStatus::IOError;
	}


	///////////////////////////////////////////////////////////////////////

	// Start monitoring for changes

	DWORD dwBytesReturned = 0;
	info->dirPath = pPath;

	if( !ReadDirectoryChangesW(
		info->handle,
		info->buf,
		sizeof(info->buf),
		bIncludeChildDir, //Sub Tree
		notifyFilters,
		&dwBytesReturned,
		&info->overlap,
		NULL) )
	{
		return EFileMonitorStatus::ReadDirError;
	}

	auto pDirPath = info->dirPath.c_str();
	///////////////////////////////////////////////////
	mDirMap.emplace(pDirPath, info.release());
	return EFileMonitorStatus::OK;
}

bool WinodwsFileMonitor::removeDirectoryPath(wchar_t const* pPath, bool bCheckReference)
{
	auto iter = mDirMap.find(pPath);
	if( iter == mDirMap.end() )
		return false;

	DirMonitorInfo* dmInfo = iter->second;
	dmInfo->refcount -= 1;
	
	if ( !bCheckReference || dmInfo->refcount <= 0 )
	{
		destroyInfo(dmInfo);
		mDirMap.erase(iter);
		return true;
	}
	return false;
}

EFileMonitorStatus::Type  WinodwsFileMonitor::checkDirectoryStatus(uint32 timeout)
{

	DWORD		dwBytesXFered = 0;
	ULONG_PTR	ulKey = 0;
	OVERLAPPED*	pOl;
	///////////////////////////////////////////////////////
	if( !GetQueuedCompletionStatus(mhIOCP, &dwBytesXFered, &ulKey, &pOl, 0) )
	{
		if( GetLastError() == WAIT_TIMEOUT )
			return EFileMonitorStatus::OK;

		return EFileMonitorStatus::IOError;
	}

	DirMonitorInfo* pDirFind = nullptr;
	for( auto pair : mDirMap )
	{
		DirMonitorInfo* pDir = pair.second;
		if( ulKey == (ULONG_PTR)pDir->handle )
		{
			pDirFind = pDir;
			break;
		}
	}
	if( pDirFind == nullptr )
	{
		return EFileMonitorStatus::UnknownError;
	}

	FILE_NOTIFY_INFORMATION* pIter = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pDirFind->buf);

	while( pIter )
	{

		std::wstring wstrFilename(pIter->FileName , pIter->FileNameLength / sizeof(WCHAR));
		//wstrFilename += L'\0';
		std::wstring filePath = pDirFind->dirPath + wstrFilename;

		// If it could be a short filename, expand it.
		LPCWSTR wszFilename = ::PathFindFileNameW(filePath.c_str());
		int len = lstrlenW(wszFilename);
		// The maximum length of an 8.3 filename is twelve, including the dot.
		if( len <= 12 && wcschr(wszFilename, L'~') )
		{
			// Convert to the long filename form. Unfortunately, this
			// does not work for deletions, so it's an imperfect fix.
			wchar_t wbuf[MAX_PATH];
			if( ::GetLongPathNameW(wstrFilename.c_str(), wbuf, _countof(wbuf)) > 0 )
				filePath = wbuf;
		}

		EFileAction action = EFileAction::Modify;
		switch( pIter->Action )
		{
		case FILE_ACTION_ADDED: action = EFileAction::Created; break;
		case FILE_ACTION_MODIFIED: action = EFileAction::Modify; break;
		case FILE_ACTION_REMOVED: action = EFileAction::Remove; break;
		case FILE_ACTION_RENAMED_OLD_NAME: action = EFileAction::RenameOld; break;
		case FILE_ACTION_RENAMED_NEW_NAME: action = EFileAction::Rename; break;
		}

		onNotify(filePath.c_str(), action);

		if( pIter->NextEntryOffset == 0 )
			break;

		if( (DWORD)((BYTE*)pIter - (BYTE*)pDirFind->buf) > sizeof(pDirFind->buf) )
			pIter = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pDirFind->buf);

		pIter = (PFILE_NOTIFY_INFORMATION)((LPBYTE)pIter + pIter->NextEntryOffset);
	}


	//////////////////////////////////////////////////////

	// Continue reading for changes

	DWORD dwBytesReturned = 0;

	if( !ReadDirectoryChangesW(
		pDirFind->handle,
		pDirFind->buf,
		sizeof(pDirFind->buf),
		pDirFind->bIncludeChildDir,
		pDirFind->notifyFilters,
		&dwBytesReturned,
		&pDirFind->overlap,
		NULL) )
	{
		mDirMap.erase(mDirMap.find(pDirFind->dirPath.c_str()));
		CloseHandle(pDirFind->handle);
		delete pDirFind;
		return EFileMonitorStatus::ReadDirError;
	}

	return EFileMonitorStatus::OK;
}


int WinodwsFileMonitor::EnablePrivilegeValue(TCHAR const* valueNames[], int numValue)
{
	int result = 0;

	HANDLE hToken = NULL;
	if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken) )
		return 0;

	for( int i = 0; i < numValue; ++i )
	{
		TOKEN_PRIVILEGES tp = { 1 };
		if( LookupPrivilegeValue(NULL, valueNames[i], &tp.Privileges[0].Luid) )
		{
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
			result += (GetLastError() == ERROR_SUCCESS) ? 1 : 0;
		}
	}

	CloseHandle(hToken);
	return result;
}


#endif //SYS_PLATFORM_WIN

bool AssetManager::init()
{
	mFileModifyMonitor = IPlatformFileMonitor::Create();
	if (mFileModifyMonitor == nullptr)
		return false;
#if USE_FAST_DELEGATE
	mFileModifyMonitor->setFileNotifyCallback(FileNotifyCallback(this, &AssetManager::handleDirectoryModify));
#else
	mFileModifyMonitor->setFileNotifyCallback(std::bind(&AssetManager::handleDirectoryModify, this, std::placeholders::_1, std::placeholders::_2));
#endif
	return true;
}

void AssetManager::cleanup()
{
	mAssetMap.clear();
	mFileModifyMonitor->release();
}

void AssetManager::tick(long time)
{
	mFileModifyMonitor->tick(time);
}


bool AssetManager::registerViewer(IAssetViewer* asset)
{
	assert(asset);

	TArray< std::wstring > paths;
	asset->getDependentFilePaths(paths);

	for ( auto const& path : paths )
	{
		std::wstring fullPath = FFileSystem::ConvertToFullPath(path.c_str());
		AssetList& assetList = mAssetMap[fullPath];
		assert(std::find(assetList.begin(), assetList.end(), asset) == assetList.end());
		assetList.push_back(asset);

		wchar_t const* pathPos = FFileUtility::GetFileName(fullPath.c_str());
		std::wstring dir = pathPos ? std::wstring(fullPath.c_str(),pathPos - fullPath.c_str()) : std::wstring();
		mFileModifyMonitor->addDirectoryPath(dir.c_str(), false);
	}
	return true;
}

void AssetManager::unregisterViewer(IAssetViewer* asset)
{
	TArray< std::wstring > paths;
	asset->getDependentFilePaths(paths);

	for( auto& path : paths )
	{
		std::wstring fullPath = FFileSystem::ConvertToFullPath(path.c_str());
		auto iter = mAssetMap.find(fullPath);
		if( iter == mAssetMap.end() )
			continue;

		auto assetIter = std::find(iter->second.begin(), iter->second.end(), asset);
		if( assetIter != iter->second.end() )
		{
			iter->second.erase(assetIter);
			wchar_t const* pathPos = FFileUtility::GetFileName(fullPath.c_str());
			std::wstring dir = pathPos ? std::wstring(fullPath.c_str(), pathPos - fullPath.c_str()) : std::wstring();
			mFileModifyMonitor->removeDirectoryPath(dir.c_str(), true);
		}
	}
}

void AssetManager::handleDirectoryModify(wchar_t const* path, EFileAction action)
{
	auto iter = mAssetMap.find(path);

	if( iter != mAssetMap.end() )
	{
		if (action == EFileAction::Modify)
		{
			LogMsg("Asset File Changed : %s", FCString::WCharToChar(path).c_str());
		}

		for( IAssetViewer* asset : iter->second )
		{
			asset->postFileModify(action);
		}
	}
}

IPlatformFileMonitor* IPlatformFileMonitor::Create()
{
#if SYS_PLATFORM_WIN
	WinodwsFileMonitor* monitor = new WinodwsFileMonitor;
	if (!monitor->init())
	{
		delete monitor;
		return nullptr;
	}
	else
	{
		return monitor;
	}
#endif
	return nullptr;
}

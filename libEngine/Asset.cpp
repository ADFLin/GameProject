#include "Asset.h"

#include "FileSystem.h"
#include <cassert>

#if SYS_PLATFORM_WIN

#include "Shlwapi.h"
#pragma comment(lib,"Shlwapi.lib")

bool Win32FileModifyMonitor::init()
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

void Win32FileModifyMonitor::cleanup()
{
	for( auto& pair : mDirMap )
	{
		destroyInfo(pair.second);
	}
	mDirMap.clear();
	::CloseHandle(mhIOCP);
	mhIOCP = NULL;
}

Win32FileModifyMonitor::Status Win32FileModifyMonitor::addDirectoryPath(wchar_t const* pPath, bool bSubTree)
{
	{
		auto iter = mDirMap.find(pPath);
		if(  iter != mDirMap.end() )
		{
			iter->second->refCount += 1;
			return eOK;
		}
	}

	if( !mhIOCP )
		return eIOCPError;

	DWORD notifyFilters =
		FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_LAST_WRITE |
		FILE_NOTIFY_CHANGE_CREATION;

	DirMonitorInfo* pDir = new DirMonitorInfo;
	pDir->refCount = 1;
	pDir->notifyFilters = notifyFilters;
	pDir->bSubTree = bSubTree;

	///////////////////////////////////////////////////////////////////////
	// Open handle to the directory to be monitored, note the FILE_FLAG_OVERLAPPED

	pDir->handle = CreateFileW(pPath,
							   FILE_LIST_DIRECTORY,
							   FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
							   NULL,
							   OPEN_EXISTING,
							   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
							   NULL);
	if( pDir->handle == INVALID_HANDLE_VALUE )
	{
		delete pDir;
		return eOpenDirFail;
	}

	///////////////////////////////////////////////////////////////////////

	// Allocate notification buffers (will be filled by the system when a
	// notification occurs

	memset(&pDir->overlap, 0, sizeof(pDir->overlap));
	///////////////////////////////////////////////////////////////////////

	// Associate directory handle with the IO completion port

	if( ::CreateIoCompletionPort(pDir->handle, mhIOCP, (ULONG_PTR)pDir->handle, 0) == NULL )
	{
		CloseHandle(pDir->handle);
		delete pDir;
		return eIOCPError;
	}


	///////////////////////////////////////////////////////////////////////

	// Start monitoring for changes

	DWORD dwBytesReturned = 0;
	pDir->dirPath = pPath;

	if( !ReadDirectoryChangesW(
		pDir->handle,
		pDir->buf,
		sizeof(pDir->buf),
		pDir->bSubTree, //Sub Tree
		notifyFilters,
		&dwBytesReturned,
		&pDir->overlap,
		NULL) )
	{
		CloseHandle(pDir->handle);
		delete pDir;
		return eReadDirError;
	}

	///////////////////////////////////////////////////

	mDirMap.insert(std::make_pair(pDir->dirPath.c_str(), pDir));
	return eOK;
}

bool Win32FileModifyMonitor::removeDirectoryPathRef(wchar_t const* pPath)
{
	auto iter = mDirMap.find(pPath);
	if( iter == mDirMap.end() )
		return false;
	DirMonitorInfo* dmInfo = iter->second;
	dmInfo->refCount -= 1;
	if ( dmInfo->refCount <= 0 )
	{
		destroyInfo(dmInfo);
		mDirMap.erase(iter);
		return true;
	}
	return false;
}

Win32FileModifyMonitor::Status Win32FileModifyMonitor::checkDirectoryStatus(uint32 timeout)
{

	DWORD		dwBytesXFered = 0;
	ULONG_PTR	ulKey = 0;
	OVERLAPPED*	pOl;
	///////////////////////////////////////////////////////
	if( !GetQueuedCompletionStatus(mhIOCP, &dwBytesXFered, &ulKey, &pOl, 0) )
	{
		if( GetLastError() == WAIT_TIMEOUT )
			return eDirNoChange;
		return eIOCPError;
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
		return eUnknownError;
	}

	FILE_NOTIFY_INFORMATION* pIter = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(pDirFind->buf);

	while( pIter )
	{

		std::wstring wstrFilename(pIter->FileName , pIter->FileNameLength / sizeof(WCHAR));
		//wstrFilename += L'\0';
		std::wstring filePath = pDirFind->dirPath + L"/" + wstrFilename;

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

		FileAction action = FileAction::Modify;
		switch( pIter->Action )
		{
		case FILE_ACTION_ADDED: action = FileAction::Created; break;
		case FILE_ACTION_MODIFIED: action = FileAction::Modify; break;
		case FILE_ACTION_REMOVED: action = FileAction::Remove; break;
		case FILE_ACTION_RENAMED_OLD_NAME: action = FileAction::Rename; break;
		case FILE_ACTION_RENAMED_NEW_NAME: action = FileAction::Rename; break;
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
		pDirFind->bSubTree,
		pDirFind->notifyFilters,
		&dwBytesReturned,
		&pDirFind->overlap,
		NULL) )
	{
		mDirMap.erase(mDirMap.find(pDirFind->dirPath.c_str()));
		CloseHandle(pDirFind->handle);
		delete pDirFind;
		return eReadDirError;
	}

	return eOK;
}

int Win32FileModifyMonitor::EnablePrivilegeValue(TCHAR const* valueNames[], int numValue)
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
#if SYS_PLATFORM_WIN
	mFileModifyMonitor.onNotify = Win32FileModifyMonitor::NotifyCallback(this, &AssetManager::procDirModify);
	if( !mFileModifyMonitor.init() )
		return false;
#endif
	return true;
}

void AssetManager::tick()
{
#if SYS_PLATFORM_WIN
	
	mFileModifyMonitor.checkDirectoryStatus(0);
#endif
}


bool AssetManager::registerAsset(AssetBase* asset)
{
	assert(asset);

	std::vector< std::wstring > paths;
	asset->getDependentFilePaths(paths);

	for ( auto const& path : paths )
	{
		AssetList& assetList = mAssetMap[path];
		assert(std::find(assetList.begin(), assetList.end(), asset) == assetList.end());
		assetList.push_back(asset);
#if SYS_PLATFORM_WIN
		std::wstring dir = std::wstring(path.c_str(),FileUtility::GetDirPathPos(path.c_str()) - path.c_str());
		mFileModifyMonitor.addDirectoryPath(dir.c_str(), false);
#endif
	}
	return true;
}

void AssetManager::unregisterAsset(AssetBase* asset)
{
	std::vector< std::wstring > paths;
	asset->getDependentFilePaths(paths);

	for( auto& path : paths )
	{
		auto iter = mAssetMap.find(path);
		if( iter == mAssetMap.end() )
			continue;

		auto assetIter = std::find(iter->second.begin(), iter->second.end(), asset);
		if( assetIter != iter->second.end() )
		{
			iter->second.erase(assetIter);
#if SYS_PLATFORM_WIN
			mFileModifyMonitor.removeDirectoryPathRef(path.c_str());
#endif
		}
	}
}

void AssetManager::procDirModify(wchar_t const* path, FileAction action)
{
	auto iter = mAssetMap.find(path);

	if( iter != mAssetMap.end() )
	{
		for( AssetBase* asset : iter->second )
		{
			asset->postFileModify(action);
		}
	}
}

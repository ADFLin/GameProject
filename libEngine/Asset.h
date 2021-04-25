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

typedef fastdelegate::FastDelegate<
	void(wchar_t const* path, EFileAction action)
> FileNotifyCallback;


namespace EFileMonitorStatus
{
	enum Type
	{
		OK ,

		OutOfMemory,
		OpenDirFail,
		ReadDirError,
		IOCPError,
		UnknownError,
		DirNoChange,
	};
}

class IPlatformFileMonitor
{
public:
	virtual void tick(long time) = 0;
	virtual EFileMonitorStatus::Type addDirectoryPath(wchar_t const* path, bool bIncludeChildDir) = 0;
	virtual bool removeDirectoryPath(wchar_t const* path, bool bCheckReference) = 0;
	virtual void setFileNotifyCallback(FileNotifyCallback callback) = 0;
	virtual void release() = 0;

	static IPlatformFileMonitor* Create();
};

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

	IPlatformFileMonitor* mFileModifyMonitor;
	void handleDirectoryModify(wchar_t const* path, EFileAction action);
};


#endif // Asset_H_5448C020_E85D_4A18_B426_922C38A0BB67

#pragma once
#ifndef AssetViewer_H_FDEDE486_DF02_4EC4_B0B5_B16F5BB0BE5C
#define AssetViewer_H_FDEDE486_DF02_4EC4_B0B5_B16F5BB0BE5C

#include <string>
#include <vector>

enum class EFileAction
{
	Rename,
	Modify,
	Remove,
	Created,
};

class IAssetViewer
{
public:
	virtual void getDependentFilePaths(std::vector< std::wstring >& paths) {}
protected:
	friend class AssetManager;
	virtual void postFileModify(EFileAction action) {}
};

class IAssetViewerRegister
{
public:
	virtual bool registerViewer(IAssetViewer* asset) = 0;
	virtual void unregisterViewer(IAssetViewer* asset) = 0;
};

#endif // AssetViewer_H_FDEDE486_DF02_4EC4_B0B5_B16F5BB0BE5C

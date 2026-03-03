#pragma once
#ifndef ModuleManager_H_D9659336_CB8E_4DBF_8C95_D6CA336D74E3
#define ModuleManager_H_D9659336_CB8E_4DBF_8C95_D6CA336D74E3

#pragma once

#include "HashString.h"
#include "Platform/PlatformModule.h"
#include "AssetViewer.h"

#include "DataStructure/Array.h"
#include "HotReload.h"

#include <unordered_map>

#define USE_DYNAMIC_MODULE 1


#include "HotReload.h"


class IModuleInterface;

using ModuleHandle = FPlatformModule::Handle;

class ModuleManager
{
public:

	static CORE_API void Initialize();
	static CORE_API void Finalize();
	static CORE_API ModuleManager& Get();

#if USE_HOTRELOAD
	void  enableHotReload(class IAssetViewerRegister& assetViewerRegister);
#endif
	bool  loadModule(char const* path);
	bool  unloadModule(char const* name);
	bool  loadModulesFromFile(char const* path);	

	void  cleanupModuleInstances();
	void  cleanupModuleMemory();

	bool  registerModule(
		IModuleInterface* module,
		char const*       moduleName,
		ModuleHandle hModule,
		char const* path = nullptr,
		char const* loadedPath = nullptr);
#if USE_HOTRELOAD
	bool bHotReloadEnabled = false;
#endif
	IAssetViewerRegister* mAssetViewerRegister = nullptr;
private:

	ModuleManager();
	~ModuleManager();

	IModuleInterface*  findModule(char const* name);

	template< class Visitor >
	void  visitInternal(Visitor&& visitor)
	{
		for (auto* data : mModuleDataList)
		{
			if (!visitor(*data))
				return;
		}
	}

	struct ModuleData 
#if USE_HOTRELOAD
		: public IAssetViewer
#endif
	{
		std::string       path;
		std::string       loadedPath;
		std::string       moduleName;
		IModuleInterface* instance;
		ModuleHandle      hModule;
#if USE_HOTRELOAD
		int               hotReloadCount = 0;
		TArray<ModuleHandle> oldModules;
#endif
		ModuleData(char const* inPath, char const* inLoadedPath, char const* inModuleName, IModuleInterface* inInstance, ModuleHandle inHModule)
			: path(inPath), loadedPath(inLoadedPath), moduleName(inModuleName), instance(inInstance), hModule(inHModule){}
#if USE_HOTRELOAD
		// IAssetViewer
		void getDependentFilePaths(TArray< std::wstring >& paths) override;
		void postFileModify(EFileAction action) override;
#endif
	};
	typedef std::unordered_map< HashString, IModuleInterface* > ModuleMap;
	TArray< ModuleData* >   mModuleDataList;
	ModuleMap              mNameToModuleMap;
};

#endif // ModuleManager_H_D9659336_CB8E_4DBF_8C95_D6CA336D74E3

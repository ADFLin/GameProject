#include "ModuleManager.h"

#include "ModuleInterface.h"
#include "FileSystem.h"
#include "Core/ScopeGuard.h"

#include "StringParse.h"
#include "Module/ModularFeature.h"
#include "StdUtility.h"
#include "LogSystem.h"
#include "InlineString.h"
#include "CString.h"
#include <string>


ModuleManager::ModuleManager()
{

}

ModuleManager::~ModuleManager()
{
	CHECK(mModuleDataList.empty());
	CHECK(mNameToModuleMap.empty());
}


#if CORE_SHARE_CODE

void ModuleManager::Initialize()
{

}

void ModuleManager::Finalize()
{

}

ModuleManager& ModuleManager::Get()
{
	static ModuleManager StaticInstance;
	return StaticInstance;
}


#endif


#if USE_HOTRELOAD
void ModuleManager::enableHotReload(class IAssetViewerRegister& assetViewerRegister)
{
	bHotReloadEnabled = true;
	mAssetViewerRegister = &assetViewerRegister;
	for (auto* info : mModuleDataList)
	{
		mAssetViewerRegister->registerViewer(info);
	}
}
#endif


bool ModuleManager::registerModule(IModuleInterface* module, char const* moduleName, ModuleHandle hModule, char const* path, char const* loadedPath)
{
	CHECK(module);

	if (findModule(moduleName))
	{
		LogError(0, "Module % is Reigsted !", moduleName);
		return false;
	}

	bool bInserted = mNameToModuleMap.insert(std::make_pair(HashString(moduleName), module)).second;
	CHECK(bInserted);
	ModuleData* data = new ModuleData(path ? path : "", loadedPath ? loadedPath : "", moduleName, module, hModule);
	mModuleDataList.push_back(data);

#if USE_HOTRELOAD
	if (bHotReloadEnabled && mAssetViewerRegister)
	{
		mAssetViewerRegister->registerViewer(mModuleDataList.back());
	}
#endif

	module->startupModule();

	return true;
}

void ModuleManager::cleanupModuleInstances()
{
	visitInternal([](ModuleData& info) ->bool
	{
		info.instance->shutdownModule();
		info.instance->release();
		info.instance = nullptr;
		return true;
	});
	mNameToModuleMap.clear();
}

void ModuleManager::cleanupModuleMemory()
{
	for (auto* info : mModuleDataList)
	{
#if USE_HOTRELOAD
		if (bHotReloadEnabled && mAssetViewerRegister)
		{
			mAssetViewerRegister->unregisterViewer(info);
		}
#endif

		CHECK(info->instance == nullptr);
		if (info->hModule)
		{
			FPlatformModule::Release(info->hModule);
		}
#if USE_HOTRELOAD
		for (auto oldMod : info->oldModules)
		{
			if (oldMod)
				FPlatformModule::Release(oldMod);
		}
		if (info->loadedPath.find("_HotReload.") != std::string::npos)
		{
			FFileSystem::DeleteFile(info->loadedPath.c_str());
		}

		for (int i = 1; i <= info->hotReloadCount; ++i)
		{
			std::string versionPath = info->path;
			auto pos = versionPath.find_last_of('.');
			if (pos != std::string::npos)
			{
				std::string insertStr = "_HotReload_";
				insertStr += std::to_string(i);
				versionPath.insert(pos, insertStr);
			}
			else
			{
				versionPath += "_HotReload_";
				versionPath += std::to_string(i);
			}

			FFileSystem::DeleteFile(versionPath.c_str());
		}
#endif

		delete info;
	}
	mModuleDataList.clear();
}

IModuleInterface* ModuleManager::findModule(char const* name)
{
	auto iter = mNameToModuleMap.find(name);
	if (iter != mNameToModuleMap.end())
		return iter->second;
	return nullptr;
}

#if USE_HOTRELOAD
void ModuleManager::ModuleData::getDependentFilePaths(TArray<std::wstring>& paths)
{
	if (!path.empty())
	{
		paths.push_back(FCString::CharToWChar(path.c_str()));
	}
}

void ModuleManager::ModuleData::postFileModify(EFileAction action)
{
	if (action == EFileAction::Modify)
	{
		if (hModule)
		{
			if (instance)
			{
				instance->shutdownModule();
				instance->release();
				instance = nullptr;
			}
			
			oldModules.push_back(hModule);
			hModule = nullptr;
		}

		hotReloadCount++;

		std::string newLoadedPath = path;
		auto pos = newLoadedPath.find_last_of('.');
		if (pos != std::string::npos)
		{
			std::string insertStr = "_HotReload_";
			insertStr += std::to_string(hotReloadCount);
			newLoadedPath.insert(pos, insertStr);
		}
		else
		{
			newLoadedPath += "_HotReload_";
			newLoadedPath += std::to_string(hotReloadCount);
		}

		if (FFileSystem::CopyFile(path.c_str(), newLoadedPath.c_str(), false))
		{
			hModule = FPlatformModule::Load(newLoadedPath.c_str());
			if (hModule)
			{
				auto createFunc = (CreateModuleFunc)FPlatformModule::GetFunctionAddress(hModule, CREATE_MODULE_STR);
				if (createFunc)
				{
					instance = (*createFunc)();
					if (instance)
					{
						instance->startupModule();
						ModuleManager::Get().mNameToModuleMap[HashString(moduleName.c_str())] = instance;
					}
				}
				HotReloadRegistry::Get().applyPatches();
			}
		}
	}
}
#endif

std::string& operator += (std::string& str, StringView view)
{
	str.append(view.data(), view.size());
	return str;
}

bool ModuleManager::loadModule(char const* path)
{
	auto loadName = FFileUtility::GetBaseFileName(path).toCString();

	if (mNameToModuleMap.find((char const*)loadName) != mNameToModuleMap.end())
		return true;

	FPlatformModule::Handle hModule;
	std::string loadedPath = path;
#if USE_HOTRELOAD
	if (bHotReloadEnabled)
	{
		auto fileName = FFileUtility::GetBaseFileName(path);
		auto dir = FFileUtility::GetDirectory(path);

		std::string hotReloadPath;
		hotReloadPath += FFileUtility::GetDirectory(path);
		hotReloadPath += FFileUtility::GetBaseFileName(path);
		hotReloadPath += "_HotReload.";
		hotReloadPath += FFileUtility::GetExtension(path);
		FFileSystem::CopyFile(path, hotReloadPath.c_str(), false);

		loadedPath = hotReloadPath;
		hModule = FPlatformModule::Load(hotReloadPath.c_str());
	}
	else
#endif
	{
		hModule = FPlatformModule::Load(path);
	}
	if (hModule == NULL)
		return false;

	ON_SCOPE_EXIT
	{
		if (hModule)
		{
			FPlatformModule::Release(hModule);
		}
	};

	auto createFunc = (CreateModuleFunc)FPlatformModule::GetFunctionAddress(hModule, CREATE_MODULE_STR);
	if (!createFunc)
		return false;

	IModuleInterface* module = (*createFunc)();
	if (!module)
		return false;

	if (registerModule(module, loadName, hModule, path, loadedPath.c_str()))
	{
		hModule = NULL;
	}
	else
	{
		module->release();
		return false;
	}
	LogMsg("Module Loaded: %s", (char const*)loadName);
	return true;
}

bool ModuleManager::unloadModule(char const* name)
{

	return false;
}

bool ModuleManager::loadModulesFromFile(char const* path)
{
	TArray< uint8 > buffer;
	if (!FFileUtility::LoadToBuffer(path, buffer, true, false))
	{
		LogWarning(0, "Can't Load Module File : %s", path);
		return false;
	}

	StringView moduleDir = FFileUtility::GetDirectory(path);
	char const* text = (char const*)buffer.data();
	bool result = true;
	for (;;)
	{
		StringView moduleName = FStringParse::StringTokenLine(text);
		moduleName.trimStartAndEnd();
		if (moduleName.size() == 0)
			break;

		if (moduleDir.size())
		{
			InlineString<MAX_PATH + 1> path = moduleDir;
			path += "/";
			path += moduleName;
			result &= loadModule(path);
		}
		else
		{
			result &= loadModule(moduleName.toMutableCString());
		}
	}

	return result;
}

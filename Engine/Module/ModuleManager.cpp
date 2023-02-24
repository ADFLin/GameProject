#include "ModuleManager.h"

#include "ModuleInterface.h"
#include "FileSystem.h"
#include "Core/ScopeGuard.h"

#include "StringParse.h"
#include "Module/ModularFeature.h"
#include "StdUtility.h"
#include "LogSystem.h"
#include "InlineString.h"


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

bool ModuleManager::registerModule(IModuleInterface* module, char const* moduleName, ModuleHandle hModule)
{
	CHECK(module);

	if (findModule(moduleName))
	{
		LogError(0, "Module % is Reigsted !", moduleName);
		return false;
	}

	bool bInserted = mNameToModuleMap.insert(std::make_pair(moduleName, module)).second;
	CHECK(bInserted);
	mModuleDataList.push_back({ module ,hModule });

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
	visitInternal([](ModuleData& info) ->bool
	{
		CHECK(info.instance == nullptr);
		FPlatformModule::Release(info.hModule);
		return true;
	});
	mModuleDataList.clear();
}

IModuleInterface* ModuleManager::findModule(char const* name)
{
	auto iter = mNameToModuleMap.find(name);
	if (iter != mNameToModuleMap.end())
		return iter->second;
	return nullptr;
}

bool ModuleManager::loadModule(char const* path)
{
	StringView::TCStringConvertible<> loadName = FFileUtility::GetBaseFileName(path).toCString();

	if (mNameToModuleMap.find((char const*)loadName) != mNameToModuleMap.end())
		return true;

	FPlatformModule::Handle hModule = FPlatformModule::Load(path);
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

	if (registerModule(module, loadName, hModule))
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

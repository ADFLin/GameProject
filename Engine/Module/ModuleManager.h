#pragma once

#include "HashString.h"
#include "Platform/PlatformModule.h"

#include "DataStructure/Array.h"

#include <unordered_map>

#define USE_DYNAMIC_MODULE 1
#define USE_HOTRELOAD 1


class IModuleInterface;

using ModuleHandle = FPlatformModule::Handle;

class ModuleManager
{
public:

	static CORE_API void Initialize();
	static CORE_API void Finalize();
	static CORE_API ModuleManager& Get();

	bool  loadModule(char const* path);
	bool  unloadModule(char const* name);
	bool  loadModulesFromFile(char const* path);

	void  cleanupModuleInstances();
	void  cleanupModuleMemory();

	bool  registerModule(
		IModuleInterface* module,
		char const*       moduleName,
		ModuleHandle hModule);

	bool bHotReloadEnabled = false;
private:

	ModuleManager();
	~ModuleManager();

	IModuleInterface*  findModule(char const* name);

	template< class Visitor >
	void  visitInternal(Visitor&& visitor)
	{
		for (auto& data : mModuleDataList)
		{
			if (!visitor(data))
				return;
		}
	}

	struct ModuleData
	{
		IModuleInterface* instance;
		ModuleHandle      hModule;
	};
	typedef std::unordered_map< HashString, IModuleInterface* > ModuleMap;
	TArray< ModuleData >   mModuleDataList;
	ModuleMap              mNameToModuleMap;
};


struct VTableHelper {};


#define DECLARE_HOTRELOAD_CLASS(CLASS)\
	CLASS(VTableHelper const& helper);

#define DEFINE_HOTRELOAD_CLASS(CLASS)\
	CLASS::CLASS(VTableHelper const& helper){}

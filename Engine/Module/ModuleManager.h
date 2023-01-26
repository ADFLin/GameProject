#pragma once

#include "HashString.h"
#include "Platform/PlatformModule.h"

#include <unordered_map>
#include <vector>


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
	std::vector< ModuleData >   mModuleDataList;
	ModuleMap        mNameToModuleMap;
};
#include "HotReload.h"


#if USE_HOTRELOAD

#include "StdUtility.h"
#include "CString.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif
#include "CString.h"

#if CORE_SHARE_CODE

HotReloadRegistry& HotReloadRegistry::Get()
{
	static HotReloadRegistry StaticInstance;
	return StaticInstance;
}

HotReloadRegistry::~HotReloadRegistry()
{
	for (auto* info : mClassInfos)
	{
		delete info;
	}
}

void HotReloadRegistry::updateClassInfo(char const* name, PatchFunc func)
{
	auto* info = findOrCreateClassInfo(name);
	info->patchFunc = func;
}

void HotReloadRegistry::registerObject(void* obj, char const* name)
{
	auto* info = findOrCreateClassInfo(name);
	mEntries.push_back({ obj, info });
}

void HotReloadRegistry::unregisterObject(void* obj)
{
	mEntries.removePred([obj](Entry const& e) { return e.obj == obj; });
}

void HotReloadRegistry::applyPatches()
{
	for (auto const& entry : mEntries)
	{
		if (entry.classInfo->patchFunc)
		{
			entry.classInfo->patchFunc(entry.obj);
		}
	}
}


int ScanVTablePatch(void* specimen, size_t size, VTablePatchInfo* outInfos, int maxInfos)
{
	int count = 0;
#if SYS_PLATFORM_WIN
	for (size_t i = 0; i <= size - sizeof(void*); i += sizeof(void*))
	{
		void* ptrSpecimen = *(void**)((char*)specimen + i);

		if (ptrSpecimen == nullptr)
			continue;
		MEMORY_BASIC_INFORMATION mbi;
		// 1. First heuristic: vtable must be in MEM_IMAGE and readable
		if (!::VirtualQuery(ptrSpecimen, &mbi, sizeof(mbi)))
			continue;

		DWORD const ReadFlags = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
		if (mbi.Type != MEM_IMAGE || !(mbi.Protect & ReadFlags))
			continue;
		
		// 2. Second heuristic: the first entry of vtable must be an executable function pointer
		void* firstVirtualFunc = *(void**)ptrSpecimen;
		if (firstVirtualFunc == nullptr)
			continue;

		MEMORY_BASIC_INFORMATION funcMbi;
		if (!::VirtualQuery(firstVirtualFunc, &funcMbi, sizeof(funcMbi)))
			continue;

		DWORD const ExecFlags = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
		if (funcMbi.Type != MEM_IMAGE || !(funcMbi.Protect & ExecFlags) )
			continue;

		if (count < maxInfos)
		{
			outInfos[count].offset = i;
			outInfos[count].ptr = ptrSpecimen;
			count++;
		}
	}
#endif
	return count;
}

#endif

HotReloadClassInfo* HotReloadRegistry::findOrCreateClassInfo(char const* name)
{
	for (auto* info : mClassInfos)
	{
		if (FCString::Compare(info->className, name) == 0)
			return info;
	}

	HotReloadClassInfo* newInfo = new HotReloadClassInfo;
	newInfo->className = name;
	newInfo->patchFunc = nullptr;
	mClassInfos.push_back(newInfo);
	return newInfo;
}

#endif //USE_HOTRELOAD

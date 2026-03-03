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


int ScanVTableOffsets_Impl(void* specimen, size_t size, size_t* outOffsets, int maxOffsets)
{
	int count = 0;
#if SYS_PLATFORM_WIN
	for (size_t i = 0; i <= size - sizeof(void*); i += sizeof(void*))
	{
		void* ptrSpecimen = *(void**)((char*)specimen + i);

		if (ptrSpecimen)
		{
			MEMORY_BASIC_INFORMATION mbi;
			if (::VirtualQuery(ptrSpecimen, &mbi, sizeof(mbi)) && mbi.Type == MEM_IMAGE)
			{
				if (count < maxOffsets)
				{
					outOffsets[count++] = i;
				}
			}
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

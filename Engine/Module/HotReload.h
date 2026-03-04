#pragma once
#ifndef HotReload_H_E530FE41_ACA5_4EEA_B3A9_C52AF32EF34E
#define HotReload_H_E530FE41_ACA5_4EEA_B3A9_C52AF32EF34E

#ifndef USE_HOTRELOAD
#define USE_HOTRELOAD 1
#endif

#if USE_HOTRELOAD

#include "CoreShare.h"
#include "DataStructure/Array.h"

typedef void (*PatchFunc)(void*);

struct HotReloadClassInfo
{
	char const* className;
	PatchFunc   patchFunc;
};

class HotReloadRegistry
{
public:
	CORE_API static HotReloadRegistry& Get();

	CORE_API void updateClassInfo(char const* name, PatchFunc func);
	CORE_API void registerObject(void* obj, char const* name);
	CORE_API void unregisterObject(void* obj);
	CORE_API void applyPatches();

	CORE_API ~HotReloadRegistry();

	struct Entry
	{
		void* obj;
		HotReloadClassInfo* classInfo;
	};

	TArray<Entry> mEntries;
	TArray<HotReloadClassInfo*> mClassInfos;

private:
	HotReloadClassInfo* findOrCreateClassInfo(char const* name);
};

struct VTableHelper {};

struct VTablePatchInfo
{
	size_t offset;
	void*  ptr;
};

CORE_API int ScanVTablePatch(void* specimen, size_t size, VTablePatchInfo* outInfos, int maxInfos);

template< typename T >
void PatchVTable(void* target)
{
	struct StaticLocal
	{
		StaticLocal()
		{
			T specimen{ VTableHelper() };
			numInfos = ScanVTablePatch(&specimen, sizeof(T), cachedInfos, ARRAY_SIZE(cachedInfos));
		}
		VTablePatchInfo cachedInfos[16];
		int numInfos;
	};
	static StaticLocal StaticInstance;
	
	for (int i = 0; i < StaticInstance.numInfos; ++i)
	{
		size_t offset = StaticInstance.cachedInfos[i].offset;
		*(void**)((char*)target + offset) = StaticInstance.cachedInfos[i].ptr;
	}
}

template< typename T >
struct THotReloadObjectRegisterHelper
{
	T* obj;
	THotReloadObjectRegisterHelper(T* inObj, char const* name) : obj(inObj)
	{
		if (obj)
		{
			HotReloadRegistry::Get().registerObject(static_cast<void*>(obj), name);
		}
	}
	~THotReloadObjectRegisterHelper()
	{
		if (obj)
		{
			HotReloadRegistry::Get().unregisterObject(static_cast<void*>(obj));
		}
	}
	THotReloadObjectRegisterHelper(THotReloadObjectRegisterHelper const&) : obj(nullptr) {}
	THotReloadObjectRegisterHelper& operator=(THotReloadObjectRegisterHelper const&) { return *this; }
};

#define DECLARE_HOTRELOAD_CLASS(CLASS)\
	CLASS(VTableHelper const& helper);\
	THotReloadObjectRegisterHelper<CLASS> __ReighsterHelper{ static_cast<CLASS*>(this), #CLASS };

#define DEFINE_HOTRELOAD_CLASS(CLASS, BASE)\
	CLASS::CLASS(VTableHelper const& helper) : BASE(helper), __ReighsterHelper(nullptr, nullptr){}\
	static struct CLASS##_HotReloadReg {\
		CLASS##_HotReloadReg() {\
			HotReloadRegistry::Get().updateClassInfo(#CLASS, &PatchVTable<CLASS>);\
		}\
	} CLASS##_HotReloadRegInstance;

#define DEFINE_HOTRELOAD_CLASS_ROOT(CLASS, BASE)\
	CLASS::CLASS(VTableHelper const& helper) : BASE(), __ReighsterHelper(nullptr, nullptr){}\
	static struct CLASS##_HotReloadReg {\
		CLASS##_HotReloadReg() {\
			HotReloadRegistry::Get().updateClassInfo(#CLASS, &PatchVTable<CLASS>);\
		}\
	} CLASS##_HotReloadRegInstance;

#else

#define DECLARE_HOTRELOAD_CLASS(CLASS)
#define DEFINE_HOTRELOAD_CLASS(CLASS, BASE)
#define DEFINE_HOTRELOAD_CLASS_ROOT(CLASS, BASE)

#endif


#endif // HotReload_H_E530FE41_ACA5_4EEA_B3A9_C52AF32EF34E

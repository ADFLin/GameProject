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

CORE_API int ScanVTableOffsets_Impl(void* specimen, size_t size, size_t* outOffsets, int maxOffsets);

template< typename T >
void PatchVTable_Dynamic(void* target)
{
	struct StaticLocal
	{
		StaticLocal()
		{
			numOffsets = ScanVTableOffsets_Impl(&specimen, sizeof(T), cachedOffsets, 32);
		}
		T specimen{ VTableHelper() };
		size_t cachedOffsets[32];
		int numOffsets;
	};
	static StaticLocal StaticInstance;
	void* specimen = &StaticInstance.specimen;
	for (int i = 0; i < StaticInstance.numOffsets; ++i)
	{
		size_t offset = StaticInstance.cachedOffsets[i];
		*(void**)((char*)target + offset) = *(void**)((char*)specimen + offset);
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
			HotReloadRegistry::Get().updateClassInfo(#CLASS, &PatchVTable_Dynamic<CLASS>);\
		}\
	} CLASS##_HotReloadRegInstance;

#define DEFINE_HOTRELOAD_CLASS_ROOT(CLASS, BASE)\
	CLASS::CLASS(VTableHelper const& helper) : BASE(), __ReighsterHelper(nullptr, nullptr){}\
	static struct CLASS##_HotReloadReg {\
		CLASS##_HotReloadReg() {\
			HotReloadRegistry::Get().updateClassInfo(#CLASS, &PatchVTable_Dynamic<CLASS>);\
		}\
	} CLASS##_HotReloadRegInstance;

#else

#define DECLARE_HOTRELOAD_CLASS(CLASS)
#define DEFINE_HOTRELOAD_CLASS(CLASS, BASE)
#define DEFINE_HOTRELOAD_CLASS_ROOT(CLASS, BASE)

#endif


#endif // HotReload_H_E530FE41_ACA5_4EEA_B3A9_C52AF32EF34E

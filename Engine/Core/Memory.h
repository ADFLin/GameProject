#pragma once
#ifndef Memory_H_8EDA60D7_C2B5_4E9C_B36D_8554BAB1DF86
#define Memory_H_8EDA60D7_C2B5_4E9C_B36D_8554BAB1DF86

#include <cstring>
#include "PlatformConfig.h"
#if SYS_PLATFORM_WIN
#include <malloc.h>
#endif

struct FMemory
{
	static void* Move(void* dest, void const* src , size_t size)
	{
		return ::memmove(dest, src, size);
	}

	static void* Copy(void* dest, void const* src, size_t size)
	{
		return ::memcpy(dest, src, size);
	}

	static void* Set(void* ptr, int val , size_t size)
	{
		return ::memset(ptr, val, size);
	}

	static void* Zero(void* ptr, size_t size)
	{
		return ::memset(ptr, 0, size);
	}

	static void* Realloc(void* ptr, size_t size)
	{
		return ::realloc(ptr, size);
	}

	static void* Expand(void* ptr, size_t size)
	{
#if SYS_PLATFORM_WIN
		return ::_expand(ptr, size);
#else
		return nullptr;
#endif
	}
	static void* Alloc(size_t size)
	{
		return ::malloc(size);
	}

	static void Free(void* ptr)
	{
		::free(ptr);
	}
};

#endif // Memory_H_8EDA60D7_C2B5_4E9C_B36D_8554BAB1DF86

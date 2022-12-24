#pragma once
#ifndef PlatformProcess_H_88895360_F56B_439A_8271_D53BE97E4C4F
#define PlatformProcess_H_88895360_F56B_439A_8271_D53BE97E4C4F

#include "PlatformConfig.h"

class FGenericModule
{
public:
	using Handle = void*;
	static Handle Load(char const* path) { return nullptr; }
	static void   Release(Handle handle){}
	static void*  GetFunctionAddress(Handle handle, char const* name) { return nullptr; }
};


#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
class FWindowsModule : FGenericModule
{
public:
	using Handle = HMODULE;
	static Handle Load(char const* path);
	static void   Release(Handle handle);
	static void*  GetFunctionAddress(Handle handle, char const* name);
};
using FPlatformModule = FWindowsModule;
#else
using FPlatformModule = FGenericModule;
#endif

#endif // PlatformProcess_H_88895360_F56B_439A_8271_D53BE97E4C4F

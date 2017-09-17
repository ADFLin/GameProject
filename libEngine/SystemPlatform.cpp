#include "SystemPlatform.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <intrin.h>
#endif

#undef InterlockedExchange

int SystemPlatform::GetProcessorNumber()
{
#if SYS_PLATFORM_WIN
	SYSTEM_INFO sysinfo;
	::GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
#error no impl
#endif
}

void SystemPlatform::MemoryBarrier()
{
#if SYS_PLATFORM_WIN
	::MemoryBarrier();
#else
#error no impl
#endif

}

void SystemPlatform::Sleep(uint32 millionSecond)
{
#if SYS_PLATFORM_WIN
	::Sleep(millionSecond);
#else
#error no impl
#endif
}

int32 SystemPlatform::InterlockedExchange(volatile int32* value, int32 exchange)
{
#if SYS_PLATFORM_WIN
	return (int32)::_InterlockedExchange((long*)value, (long)exchange);
#else
#error no impl
#endif
}

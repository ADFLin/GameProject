#include "SystemPlatform.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <intrin.h>
#else
#include <ctime>
#include "MemorySecurity.h"
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

double SystemPlatform::GetHighResolutionTime()
{
#if SYS_PLATFORM_WIN
	LARGE_INTEGER Time;
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&Time);
	return double( Time.QuadPart  ) / Frequency.QuadPart;
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

DateTime SystemPlatform::GetUTCTime()
{
#if SYS_PLATFORM_WIN
	SYSTEMTIME WinSysTime;
	::GetSystemTime(&WinSysTime);
	return DateTime(WinSysTime.wYear, WinSysTime.wMonth, WinSysTime.wDay, WinSysTime.wHour, WinSysTime.wMinute, WinSysTime.wSecond, WinSysTime.wMilliseconds);
#else
	struct timeval time;
	gettimeofday(&time, nullptr);

	struct tm localTime;
	::gmtime_s(&time.tv_sec, &localTime);
	return DateTime(localTime.tm_year, localTime.tm_mon, localTime.tm_mday, localTime.tm_min, localTime.tm_sec, time.tv_usec / 1000);
#endif
}

DateTime SystemPlatform::GetLocalTime()
{
#if SYS_PLATFORM_WIN
	SYSTEMTIME WinSysTime;
	::GetLocalTime(&WinSysTime);
	return DateTime(WinSysTime.wYear, WinSysTime.wMonth, WinSysTime.wDay, WinSysTime.wHour, WinSysTime.wMinute, WinSysTime.wSecond, WinSysTime.wMilliseconds);
#else
	struct timeval time;
	gettimeofday(&time, nullptr);

	struct tm localTime;
	::localtime_s(&localTime , &time.tv_sec);
	return DateTime(localTime.tm_year, localTime.tm_mon, localTime.tm_mday, localTime.tm_min, localTime.tm_sec, time.tv_usec / 1000 );
#endif
}

#include "SystemPlatform.h"

#include "MarcoCommon.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <Commdlg.h>
#include <intrin.h>
//#include <xatomic.h>
#include <winnt.h>
#else
#include <ctime>
#include "MemorySecurity.h"
#endif


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

void SystemPlatform::MemoryStoreFence()
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

char const* SystemPlatform::GetEnvironmentVariable(char const* key)
{
#if SYS_PLATFORM_WIN
	static thread_local char buffer[32767];
	DWORD num = ::GetEnvironmentVariableA(key, buffer , ARRAY_SIZE(buffer));
	if( num == 0 )
		return nullptr;
	return buffer;
#else
	return std::getenv();
#endif
}

int32 SystemPlatform::AtomExchange(volatile int32* ptr, int32 value)
{
#if SYS_PLATFORM_WIN
	return (int32)::_InterlockedExchange((volatile long*)ptr, (long)value);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomExchange(volatile int64* ptr, int64 value)
{
#if SYS_PLATFORM_WIN
	return (int64)InterlockedExchange64((volatile __int64*)ptr, (__int64)value);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomExchangeAdd(volatile int32* ptr, int32 value)
{
#if SYS_PLATFORM_WIN
	return (int32)::_InterlockedExchangeAdd((volatile long*)ptr, (long)value);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomExchangeAdd(volatile int64* ptr, int64 value)
{
#if SYS_PLATFORM_WIN
	return (int64)InterlockedExchangeAdd64((volatile __int64*)ptr, (__int64)value);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomAdd(volatile int32* ptr, int32 value)
{
#if SYS_PLATFORM_WIN
	return (int32)InterlockedAdd((volatile long*)ptr, (long)value);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomAdd(volatile int64* ptr, int64 value)
{
#if SYS_PLATFORM_WIN
	return (int64)InterlockedAdd64((volatile __int64*)ptr, (__int64)value);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomCompareExchange(volatile int32* ptr, int32 exchange, int32 comperand)
{
#if SYS_PLATFORM_WIN
	return (int32)InterlockedCompareExchange((volatile long*)ptr, (long)exchange,(long)comperand);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomCompareExchange(volatile int64* ptr, int64 exchange, int64 comperand)
{
#if SYS_PLATFORM_WIN
	return (int64)InterlockedCompareExchange64((volatile __int64*)ptr, (__int64)exchange, (__int64)comperand);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomIncrement(volatile int32* ptr)
{
#if SYS_PLATFORM_WIN
	return _InterlockedIncrement((volatile long*)ptr);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomIncrement(volatile int64* ptr)
{
#if SYS_PLATFORM_WIN
	return InterlockedIncrement64((volatile __int64*)ptr);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomDecrement(volatile int32* ptr)
{
#if SYS_PLATFORM_WIN
	return _InterlockedDecrement((volatile long*)ptr);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomDecrement(volatile int64* ptr)
{
#if SYS_PLATFORM_WIN
	return InterlockedDecrement64((volatile __int64*)ptr);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomicRead(volatile int32* ptr)
{
#if SYS_PLATFORM_WIN
	return _InterlockedCompareExchange((volatile long*)ptr ,0 ,0);
#else
#error no impl
#endif
}

int64 SystemPlatform::AtomicRead(volatile int64* ptr)
{
#if SYS_PLATFORM_WIN
	return (int64)_InterlockedCompareExchange64((volatile __int64*)ptr, 0, 0);
#else
#error no impl
#endif
}

DateTime SystemPlatform::GetUTCTime()
{
#if SYS_PLATFORM_WIN
	SYSTEMTIME systemTime = { 0 };
	::GetSystemTime(&systemTime);
	return DateTime(systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
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
	SYSTEMTIME WinSysTime = { 0 };
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

#include "FileSystem.h"

bool SystemPlatform::OpenFileName(char inoutPath[], int pathSize, char const* initDir)
{
#if SYS_PLATFORM_WIN
	OPENFILENAME ofn;
	// a another memory buffer to contain the file name
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = inoutPath;
	ofn.nMaxFile = pathSize;
	ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = initDir;
	if( inoutPath )
		ofn.nFileOffset = FileUtility::GetFileName(inoutPath) - inoutPath;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	
	if( !GetOpenFileNameA(&ofn) )
	{
		DWORD error = CommDlgExtendedError();
		switch( error )
		{
		default:
			break;
		}
		return false;
	}

	return true;
#else
	return false;
#endif
}

int64 SystemPlatform::GetTickCount()
{
#if SYS_PLATFORM_WIN
	return ::GetTickCount64();
#else
#error no impl
#endif
}

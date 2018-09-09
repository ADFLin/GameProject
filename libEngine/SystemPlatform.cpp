#include "SystemPlatform.h"

#include "MarcoCommon.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <Commdlg.h>
#include <intrin.h>
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

int32 SystemPlatform::InterlockedExchange(volatile int32* ptr, int32 value)
{
#if SYS_PLATFORM_WIN
	return (int32)::_InterlockedExchange((volatile long*)ptr, (long)value);
#else
#error no impl
#endif
}

int32 SystemPlatform::InterlockedExchangeAdd(volatile int32* ptr, int32 value)
{
#if SYS_PLATFORM_WIN
	return (int32)::_InterlockedExchangeAdd((volatile long*)ptr, (long)value);
#else
#error no impl
#endif
}

int32 SystemPlatform::InterlockedAdd(volatile int32* ptr, int32 value)
{
#if SYS_PLATFORM_WIN
	return (int32)::_InterlockedAdd((volatile long*)ptr, (long)value);
#else
#error no impl
#endif
}

int32 SystemPlatform::AtomicRead(volatile int32* ptr)
{
#if SYS_PLATFORM_WIN
	return ::_InterlockedCompareExchange((volatile long*)ptr ,0 ,0);
#else
#error no impl
#endif
}

DateTime SystemPlatform::GetUTCTime()
{
#if SYS_PLATFORM_WIN
	SYSTEMTIME systemTime;
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
		ofn.nFileOffset = FileUtility::GetDirPathPos(inoutPath) - inoutPath + 1;
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

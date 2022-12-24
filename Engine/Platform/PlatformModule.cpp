#include "PlatformModule.h"
#include "LogSystem.h"

#if SYS_PLATFORM_WIN
FWindowsModule::Handle FWindowsModule::Load(char const* path)
{
	Handle hModule = ::LoadLibraryA(path);
	if (hModule == NULL)
	{
		char* lpMsgBuf;
		DWORD message = ::GetLastError();
		::FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			message,
			MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		LogWarning(0, "Load Module %s Fail : %s", path, lpMsgBuf);
		::LocalFree(lpMsgBuf);
	}
	return hModule;
}

void FWindowsModule::Release(Handle handle)
{
	::FreeLibrary(handle);
}

void* FWindowsModule::GetFunctionAddress(Handle handle, char const* name)
{
	return ::GetProcAddress(handle, name);
}
#endif
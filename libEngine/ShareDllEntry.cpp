#include "PlatformConfig.h"
#include "CoreShare.h"
#include "LogSystem.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"

BOOL WINAPI DllMain( _In_  HMODULE hModule, _In_  DWORD fdwReason, _In_  LPVOID lpvReserved)
{
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH :
		DisableThreadLibraryCalls(hModule);
		CoreShareInitialize();
		break;
	case DLL_PROCESS_DETACH:
		CoreShareFinalize();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

#endif //SYS_PLATFORM_WIN
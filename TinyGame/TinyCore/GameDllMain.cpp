#include "TinyGamePCH.h"

#ifdef TG_DLL

BOOL WINAPI DllMain(
					_In_  HINSTANCE hinstDLL,
					_In_  DWORD fdwReason,
					_In_  LPVOID lpvReserved)
{
#if 0
	char name[256];
	GetModuleFileNameA(HMODULE(hinstDLL), name, ARRAY_SIZE(name));
	LogMsg("run %s dll ", name);
#endif
	return TRUE;
}

#endif
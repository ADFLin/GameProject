#include "TinyGamePCH.h"

#ifdef TG_DLL

BOOL WINAPI DllMain(
					_In_  HINSTANCE hinstDLL,
					_In_  DWORD fdwReason,
					_In_  LPVOID lpvReserved)
{
	return TRUE;
}

#endif
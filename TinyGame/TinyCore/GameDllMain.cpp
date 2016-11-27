#include "TinyGamePCH.h"

#ifdef TG_DLL

#include "Phy2D/Phy2D.h"

BOOL WINAPI DllMain(
					_In_  HINSTANCE hinstDLL,
					_In_  DWORD fdwReason,
					_In_  LPVOID lpvReserved)
{
	return TRUE;
}

#endif
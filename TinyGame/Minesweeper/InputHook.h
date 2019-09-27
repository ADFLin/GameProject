#ifndef InputHook_h__
#define InputHook_h__

#ifdef USE_HOOK
#	define HOOKAPI __declspec(dllimport)
#else
#	define HOOKAPI __declspec(dllexport)
#endif //USE_HOOK

#include <Windows.h>
typedef LRESULT __stdcall CALLBACK HookProc( int , WPARAM , LPARAM );
extern "C"
{
	BOOL HOOKAPI InstallMouseHook( HookProc* userPorc );
	BOOL HOOKAPI UninstallMouseHook();
	BOOL HOOKAPI InstallKeyBoardHook( HookProc* userPorc );
	BOOL HOOKAPI UninstallKeyBoardHook();
};

#endif // InputHook_h__

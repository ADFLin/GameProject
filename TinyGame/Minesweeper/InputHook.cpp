#include "InputHook.h"

static HookProc* gMouseHookProc;
static HookProc* gKeyboardHookProc;
static HHOOK ghMouseHook = NULL;
static HHOOK ghKeyboardHook = NULL;

HINSTANCE gHinstance = NULL;

LRESULT __stdcall CALLBACK MouseHookProc( int nCode ,WPARAM wParam,  LPARAM lParam )
{
	(*gMouseHookProc)( nCode , wParam , lParam );
	LRESULT RetVal = CallNextHookEx( ghMouseHook , nCode, wParam, lParam );
	return RetVal;
}

LRESULT __stdcall CALLBACK KeyBoardHookProc( int nCode ,WPARAM wParam,  LPARAM lParam )
{
	(*gKeyboardHookProc)( nCode , wParam , lParam );
	LRESULT RetVal = CallNextHookEx( ghMouseHook , nCode, wParam, lParam );
	return RetVal;
}

extern "C"
{
	BOOL HOOKAPI InstallMouseHook( HookProc* userPorc )
	{
		if ( userPorc == NULL )
			return FALSE;
		ghMouseHook = ::SetWindowsHookEx( WH_MOUSE ,(HOOKPROC)MouseHookProc, gHinstance ,0 );
		gMouseHookProc = userPorc;
		return TRUE;
	}

	BOOL HOOKAPI UninstallMouseHook()
	{
		UnhookWindowsHookEx( ghMouseHook );
		ghMouseHook = NULL;
		gMouseHookProc = NULL;
		return TRUE;
	}

	BOOL HOOKAPI InstallKeyBoardHook( HookProc* userPorc )
	{
		if ( userPorc == NULL )
			return FALSE;
		ghKeyboardHook = ::SetWindowsHookEx( WH_KEYBOARD ,(HOOKPROC)KeyBoardHookProc , gHinstance ,0 );
		gKeyboardHookProc = userPorc;
		return TRUE;
	}

	BOOL HOOKAPI UninstallKeyBoardHook()
	{
		UnhookWindowsHookEx( ghKeyboardHook );
		ghKeyboardHook = NULL;
		gKeyboardHookProc = NULL;
		return TRUE;
	}


};


BOOL WINAPI DllMain(  __in  HINSTANCE hinstDLL,
					  __in  DWORD fdwReason,
					  __in  LPVOID lpvReserved ) 
{
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		gHinstance = hinstDLL;
		break;
	case DLL_PROCESS_DETACH:
		UninstallMouseHook();
		UninstallKeyBoardHook();
		break;
	}
	return TRUE;
}
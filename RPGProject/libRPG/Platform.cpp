#include "ProjectPCH.h"
#include "Platform.h"

CWinPlatform::CWinPlatform( HINSTANCE hInstance , HWND hWnd ) 
	:mhInstance( hInstance )
	,mhWnd( hWnd )
{
	OSVERSIONINFOEX osv;
	::ZeroMemory( &osv , sizeof(osv));
	osv.dwOSVersionInfoSize = sizeof( osv );

	mPlatformID = PSI_WIN_OTHER;
	if ( ::GetVersionEx( (LPOSVERSIONINFO)&osv ) )
	{
		DWORD dwVersion = MAKEWORD( osv.dwMajorVersion , osv.dwMinorVersion );
		switch( dwVersion )
		{
		case MAKEWORD(6,1):
			if ( osv.wProductType == VER_NT_WORKSTATION )
				mPlatformID = PSI_WIN_7;
			break;
		case MAKEWORD(6,0):
			if ( osv.wProductType == VER_NT_WORKSTATION )
				mPlatformID = PSI_WIN_VISTA;
			break;
		case MAKEWORD(5,1):
			mPlatformID = PSI_WIN_XP;
			break;
		}
	}
}


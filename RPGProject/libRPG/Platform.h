#ifndef Platform_h__
#define Platform_h__

#include "ISystem.h"

class CWinPlatform : public IOSPlatform
{
public:
	CWinPlatform( HINSTANCE hInstance , HWND hWnd );
	PlatformID getPlatformID(){ return mPlatformID;  }
private:
	PlatformID mPlatformID;
	HINSTANCE  mhInstance;
	HWND       mhWnd;
};

#endif // Platform_h__

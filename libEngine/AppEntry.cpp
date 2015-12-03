#include "PlatformConfig.h"

#ifdef SYS_PLATFORM_WIN

#include "Win32Header.h"

#include <cstring>
extern int main( int argc , char* argv[] );

int WINAPI WinMain ( HINSTANCE hInstance , HINSTANCE hPrevInstance , LPSTR lpCmdLine , int nCmdShow )
{
	char* argv[ 64 ];
	int   argc = 0;

	
	char* context;
	char* ptrCmd = strtok_s( lpCmdLine , " " , &context );
	while ( ptrCmd )
	{
		argv[ argc++ ] = ptrCmd;
		ptrCmd = strtok_s( NULL , " " , &context );
	}

	int ret = main( argc , argv );

	return ret;
}

#endif SYS_PLATFORM_WIN

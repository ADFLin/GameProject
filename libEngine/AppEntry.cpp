#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN

#include "WindowsHeader.h"
#include "MemorySecurity.h"
#include <cstring>

extern int main( int argc , char* argv[] );

int WINAPI WinMain ( HINSTANCE hInstance , HINSTANCE hPrevInstance , LPSTR lpCmdLine , int nCmdShow )
{
	lpCmdLine = GetCommandLineA();

	char* argv[ 128 ];
	int   argc = 0;

	char* context;
	char* ptrCmd;

	if( *lpCmdLine == '\"' )
	{
		++lpCmdLine;
		ptrCmd = strtok_s(lpCmdLine, "\"", &context);
	}
	else
	{
		ptrCmd = strtok_s(lpCmdLine, " ", &context);
	}

	while ( ptrCmd )
	{
		argv[ argc++ ] = ptrCmd;
		ptrCmd = strtok_s( NULL , " " , &context );
	}

	int ret = main( argc , argv );
	return ret;
}

#endif //SYS_PLATFORM_WIN

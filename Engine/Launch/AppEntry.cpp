#include "PlatformConfig.h"

#include "CommandlLine.h"

#if SYS_PLATFORM_WIN

#include "WindowsHeader.h"


extern int main( int argc , char const* argv[] );

int WINAPI WinMain ( HINSTANCE hInstance , HINSTANCE hPrevInstance , LPSTR cmd , int nCmdShow )
{
	FCommandLine::Initialize(GetCommandLineA());
	int argc = 0;
	char const** argv = FCommandLine::GetArgs(argc);
	int ret = main( argc , argv );
	return ret;
}

#endif //SYS_PLATFORM_WIN

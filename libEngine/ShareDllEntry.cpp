#include "PlatformConfig.h"

#include "LogSystem.h"
#include "ProfileSystem.h"
#include "Phy2D/Phy2D.h"
#include "UnitTest.h"
#include "MarcoCommon.h"
#include "HashString.h"


#define EXPORT_FUN(fun) static auto MARCO_NAME_COMBINE_2( GExportFun , __LINE__ ) = &fun;

EXPORT_FUN( LogOutput::addChannel );
EXPORT_FUN( UnitTest::Component::RunTest );
//EXPORT_FUN( static_cast< void (*)(char const*, ...) >( &LogMsgF ));

#undef EXPORT_FUN

#include "WindowsHeader.h"

BOOL WINAPI DllMain( _In_  HINSTANCE hinstDLL, _In_  DWORD fdwReason, _In_  LPVOID lpvReserved)
{
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH :
		CoreShareInitialize();
		break;
	case DLL_PROCESS_DETACH:
		CoreShareFinalize();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

#include "Guid.h"

#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "Objbase.h"
#pragma comment (lib,"Ole32.lib")
#endif

Guid Guid::New()
{
	Guid result;
#if SYS_PLATFORM_WIN
	static_assert(sizeof(Guid) == sizeof(GUID) , "Guid and GUID size not match");
	HRESULT hCreateGuid = ::CoCreateGuid((GUID*)&result);
	if( hCreateGuid != S_OK )
	{

	}
#else
#error not impl yet!
#endif
	return result;
}

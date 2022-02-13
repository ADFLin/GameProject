#include "Guid.h"

#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "Objbase.h"
#pragma comment (lib,"Ole32.lib")
#else
#include <stdlib.h>
#include "MarcoCommon.h"
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
	for (int i = 0; i < ARRAY_SIZE(result.mData); ++i)
	{
		result.mData[i] = rand();
	}
#endif
	return result;
}

#ifndef CoreShare_h__
#define CoreShare_h__

#include "PlatformConfig.h"

#ifdef CORE_SHARE
#	ifdef CORE_EXPORT
#		define CORE_API __declspec( dllexport )
#	else
#		define CORE_API __declspec( dllimport )
#	endif
#else
#	define CORE_API __declspec( dllimport )
#endif

CORE_API void CoreShareInitialize();
CORE_API void CoreShareFinalize();


#endif // CoreShare_h__

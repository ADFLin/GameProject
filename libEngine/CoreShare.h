#ifndef CoreShare_h__
#define CoreShare_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#ifdef CORE_SHARE
#	ifdef CORE_EXPORT
#		define CORE_API DLLEXPORT
#	else
#		define CORE_API DLLIMPORT
#	endif
#else
#	define CORE_API DLLIMPORT
#endif

CORE_API void CoreShareInitialize();
CORE_API void CoreShareFinalize();


#endif // CoreShare_h__

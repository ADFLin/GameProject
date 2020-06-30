#ifndef CoreShare_h__
#define CoreShare_h__

#include "PlatformConfig.h"
#include "CompilerConfig.h"

#ifndef CORE_SHARE
#define CORE_SHARE 0
#endif

#ifndef CORE_EXPORT
#define CORE_EXPORT 0
#endif


#if CORE_SHARE
#	if CORE_EXPORT
#		define CORE_API DLLEXPORT
#		define CORE_SHARE_CODE 1
#	else
#		define CORE_API DLLIMPORT
#		define CORE_SHARE_CODE 0
#	endif
#else
#	define CORE_API DLLIMPORT
#	define CORE_SHARE_CODE 0
#endif

CORE_API void EngineInitialize();
CORE_API void EngineFinalize();

CORE_API void CoreShareInitialize();
CORE_API void CoreShareFinalize();


#endif // CoreShare_h__

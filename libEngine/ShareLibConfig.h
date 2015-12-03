#ifndef ShareLibConfig_h__
#define ShareLibConfig_h__

#include "PlatformConfig.h"

#ifdef CORE_SHARE
#	ifdef CORE_EXPORT
#		define CORE_API __declspec( dllexport )
#	else
#		define CORE_API __declspec( dllimport )
#	endif
#else
#	define CORE_API __declspec( dllexport )
#endif


#endif // ShareLibConfig_h__

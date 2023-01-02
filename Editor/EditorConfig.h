#ifndef EditorConfig_h__
#define EditorConfig_h__

#include "CompilerConfig.h"

#ifndef EDITOR_EXPORT
#define EDITOR_EXPORT 0
#endif

#if EDITOR_EXPORT
#	define EDITOR_API DLLEXPORT
#else
#	define EDITOR_API DLLIMPORT
#endif

#endif // EditorConfig_h__

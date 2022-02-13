#ifndef PlatformConfig_h__
#define PlatformConfig_h__

#if defined ( _WIN32 ) || defined ( _WIN64 ) || defined(__WIN32__)
#	define SYS_PLATFORM_WIN 1
#elif defined ( _XBOX )
#   define SYS_PLATFORM_XBOX 1
#elif defined(linux) || defined(__linux)
#	define SYS_PLATFORM_LINUX 1
#elif defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh)
#	define SYS_PLATFORM_MACOS 1
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#	define SYS_PLATFORM_FREEBSD 1
#elif  defined(__EMSCRIPTEN__)
#	define SYS_PLATFORM_HTML5 1 
#else
#   define SYS_PLATFORM_UNKNOWN 1
#endif

#ifndef SYS_PLATFORM_WIN
#	define SYS_PLATFORM_WIN 0
#endif

#ifndef SYS_PLATFORM_XBOX
#	define SYS_PLATFORM_XBOX 0
#endif

#ifndef SYS_PLATFORM_LINUX
#	define SYS_PLATFORM_LINUX 0
#endif

#ifndef SYS_PLATFORM_MACOS
#	define SYS_PLATFORM_MACOS 0
#endif

#ifndef SYS_PLATFORM_FREEBSD
#	define SYS_PLATFORM_FREEBSD 0
#endif

#ifndef SYS_PLATFORM_HTML5
#	define SYS_PLATFORM_HTML5 0 
#endif

#if SYS_PLATFORM_WIN
#	if defined( _WIN64 )
#		define TARGET_PLATFORM_64BITS					1
#	else
#		define TARGET_PLATFORM_64BITS					0
#	endif
#elif SYS_PLATFORM_LINUX
#	if defined(_LINUX64) || defined(_LP64)
#		define TARGET_PLATFORM_64BITS					1
#	else
#		define TARGET_PLATFORM_64BITS					0
#	endif
#endif


#if SYS_PLATFORM_WIN
#define PLATFORM_WIN_VALUE( VALUE , NOT_PLATFORM_VALUE ) VALUE
#else
#define PLATFORM_WIN_VALUE( VALUE , NOT_PLATFORM_VALUE ) NOT_PLATFORM_VALUE
#endif

#endif // PlatformConfig_h__

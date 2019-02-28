#ifndef GameConfig_h__
#define GameConfig_h__

#include "MarcoCommon.h"
#include "CoreShare.h"
#include "CompilerConfig.h"

#define LAST_VERSION  MAKE_VERSION( 0xff , 0xff , 0xffff )
#define MAX_PLAYER_NAME_LENGTH 32

#ifdef TG_DLL
#	ifdef TG_EXPORT
#		define TINY_API DLLEXPORT
#	else
#		define TINY_API DLLIMPORT
#	endif
#endif //TG_DLL

#ifndef TINY_API
#	define TINY_API
#endif

#define TINY_USE_NET_THREAD 1

#ifndef TINY_USE_NET_THREAD
#define TINY_USE_NET_THREAD 0
#endif

#if TINY_USE_NET_THREAD
#define DEFINE_MUTEX( MUTEX ) Mutex MUTEX;
#define MUTEX_LOCK( MUTEX ) Mutex::Locker MARCO_NAME_COMBINE_2( locker , __LINE__ )( MUTEX );
#else
#define DEFINE_MUTEX( MUTEX )
#define MUTEX_LOCK( MUTEX ) 
#endif //HAVE_NET_THREAD


#endif // GameConfig_h__

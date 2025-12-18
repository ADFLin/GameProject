#ifndef GameConfig_h__
#define GameConfig_h__

#include "MarcoCommon.h"
#include "CoreShare.h"
#include "CompilerConfig.h"

#define LAST_VERSION  MAKE_VERSION( 0xff , 0xff , 0xffff )
#define MAX_PLAYER_NAME_LENGTH 32

#ifndef TINY_WITH_EDITOR
#define TINY_WITH_EDITOR 1
#endif

#ifdef TG_DLL
#	ifdef TG_EXPORT
#		define TINY_API DLLEXPORT
#	else
#		define TINY_API DLLIMPORT
#	endif
#else
#	define TINY_API
#endif //TG_DLL

#define TINY_USE_NET_THREAD 0

#ifndef TINY_USE_NET_THREAD
#define TINY_USE_NET_THREAD 0
#endif

#if TINY_USE_NET_THREAD
#define NET_MUTEX( MUTEX ) Mutex MUTEX;
#define NET_MUTEX_LOCK( MUTEX ) Mutex::Locker MARCO_NAME_COMBINE_2( locker , __LINE__ )( MUTEX );
#define NET_RWLOCK( RWLOCK ) RWLock RWLOCK;
#define NET_RWLOCK_READ( RWLOCK ) RWLock::ReadLocker MARCO_NAME_COMBINE_2( readLocker , __LINE__ )( RWLOCK );
#define NET_RWLOCK_WRITE( RWLOCK ) RWLock::WriteLocker MARCO_NAME_COMBINE_2( writeLocker , __LINE__ )( RWLOCK );
#else
#define NET_MUTEX( MUTEX )
#define NET_MUTEX_LOCK( MUTEX ) 
#define NET_RWLOCK( RWLOCK )
#define NET_RWLOCK_READ( RWLOCK )
#define NET_RWLOCK_WRITE( RWLOCK )
#endif //HAVE_NET_THREAD


#endif // GameConfig_h__

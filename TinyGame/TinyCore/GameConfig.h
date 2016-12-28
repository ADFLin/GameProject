#ifndef GameConfig_h__
#define GameConfig_h__

#include "CommonMarco.h"
#include "CoreShare.h"

#define LAST_VERSION  MAKE_VERSION( 0xff , 0xff , 0xffff )
#define MAX_PLAYER_NAME_LENGTH 32

#ifdef TG_DLL
#	ifdef TG_EXPORT
#		define GAME_API __declspec( dllexport )
#	else
#		define GAME_API __declspec( dllimport )
#	endif
#endif //TG_DLL

#ifndef GAME_API
#	define GAME_API
#endif

#define F_INLINE __forceinline


#ifndef SINGLE_THREAD
#define DEFINE_MUTEX( MUTEX ) Mutex MUTEX;
#define MUTEX_LOCK( MUTEX ) Mutex::Locker MARCO_NAME_COMBINE_2( locker , __LINE__ )( MUTEX );
#else
#define DEFINE_MUTEX( MUTEX )
#define MUTEX_LOCK( MUTEX ) 
#endif


#endif // GameConfig_h__

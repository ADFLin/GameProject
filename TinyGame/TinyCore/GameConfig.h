#ifndef GameConfig_h__
#define GameConfig_h__

#define VERSION( a , b , c ) ( ( unsigned char(a)<< 24 ) | ( unsigned char(b)<< 16 ) | unsigned short(c) )
#define LAST_VERSION  VERSION( 0xff , 0xff , 0xff )
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

#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)

#ifndef SINGLE_THREAD
#define DEFINE_MUTEX( MUTEX ) Mutex MUTEX;
#define MUTEX_LOCK( MUTEX ) Mutex::Locker CONCATENATE( locker , __LINE__ )( MUTEX );
#else
#define DEFINE_MUTEX( MUTEX )
#define MUTEX_LOCK( MUTEX ) 
#endif


#endif // GameConfig_h__

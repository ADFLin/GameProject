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

#endif // GameConfig_h__

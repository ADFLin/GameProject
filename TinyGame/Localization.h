#ifndef Localization_h__
#define Localization_h__


#include <string>
#include <map>
#include <list>

enum Language
{
	LAN_SYSTEM ,
	LAN_CHINESE_T ,
	LAN_ENGLISH ,
	LANGUAGE_NUM ,
};

GAME_API void initLanguage( Language lan );
GAME_API void setLanguage( Language lan );
GAME_API char const* translateString( char const* str );
GAME_API void saveTranslateAsset( char const* path );

#ifdef USE_TRANSLATE
#	define LAN( str ) ( translateString( str ) )
#else
#	define LAN( str ) ( str )
#endif


#endif // Localization_h__
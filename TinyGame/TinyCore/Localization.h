#ifndef Localization_h__
#define Localization_h__

#include <string>
#include <map>
#include <list>

#include "HashString.h"

enum Language
{
	LAN_SYSTEM ,
	LAN_CHINESE_T ,
	LAN_ENGLISH ,
	LANGUAGE_NUM ,
};

typedef HashString LocName;

class ILocalization
{
public:
	virtual void initialize(Language lan) = 0;
	virtual void changeLanguage(Language lan) = 0;
	virtual char const* translateText(char const* str) = 0;
	virtual bool saveTranslateAsset(char const* path) = 0;

	TINY_API static ILocalization& Get();
};

#ifdef USE_TRANSLATE
#	define LOCTEXT( str ) ( ILocalization::Get().translateText( str ) )
#else
#	define LOCTEXT( str ) ( str )
#endif


#endif // Localization_h__
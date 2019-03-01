#pragma once
#ifndef TypeFormatTraits_H_6B3580E8_D568_4EC7_9A6F_1FDE130FF8B5
#define TypeFormatTraits_H_6B3580E8_D568_4EC7_9A6F_1FDE130FF8B5

#include "LogSystem.h"

template< class T >
struct TTypeFormatTraits
{
	static char const* GetString();
};

#define DEFINE_TYPE_FORMAT( TYPE , STR )\
template<>\
struct TTypeFormatTraits< TYPE >\
{\
	enum { TypeSize = sizeof(TYPE) };\
	static char const* GetString() { return STR; }\
};

DEFINE_TYPE_FORMAT(bool, "%d")
DEFINE_TYPE_FORMAT(float, "%f")
DEFINE_TYPE_FORMAT(double, "%lf")
DEFINE_TYPE_FORMAT(unsigned, "%u")
DEFINE_TYPE_FORMAT(long unsigned, "%lu")
DEFINE_TYPE_FORMAT(long long unsigned, "%llu")
DEFINE_TYPE_FORMAT(int, "%d")
DEFINE_TYPE_FORMAT(long int, "%ld")
DEFINE_TYPE_FORMAT(long long int, "%lld")
DEFINE_TYPE_FORMAT(unsigned char, "%u")
DEFINE_TYPE_FORMAT(char, "%d")
DEFINE_TYPE_FORMAT(char*, "%s")
DEFINE_TYPE_FORMAT(char const*, "%s")

#undef DEFINE_TYPE_FORMAT

template< class T , class ...Args >
bool CheckForamtString(char const* format, T ,  Args... args)
{
	for(;;)
	{
		format = FStringParse::FindChar(format, '%');
		if( *format = 0 )
		{
			LogWarning(0, "Format string args is less than inpput args");
			return false;
		}
		if( format[1] != '%' )
			break;

		format += 2;
	}


}

#endif // TypeFormatTraits_H_6B3580E8_D568_4EC7_9A6F_1FDE130FF8B5

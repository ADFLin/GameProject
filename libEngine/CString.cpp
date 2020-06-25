#include "CString.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

void FCString::Stricpy(char * dest, char const* src)
{
	assert(dest && src);
	while( *src )
	{
		int c = ::tolower(*src);
		*dest = c;
		++dest;
		++src;
	}
}

uint32 FCString::StriHash(char const* str)
{
	uint32 result = 5381;
	while( *str )
	{
		uint32 c = (uint32)tolower(*str);
		result = ((result << 5) + result) + c; /* hash * 33 + c */
		++str;
	}
	return result;
}

uint32 FCString::StriHash(char const* str, int len)
{
	uint32 result = 5381;
	while( len )
	{
		uint32 c = (uint32)tolower(*str);
		result = ((result << 5) + result) + c; /* hash * 33 + c */
		++str;
		--len;
	}
	return result;
}

wchar_t buff[1024 * 256];
std::wstring FCString::CharToWChar(const char *c)
{
	const size_t cSize = FCString::Strlen(c) + 1;
#if SYS_PLATFORM_WIN
	::MultiByteToWideChar(0,0, c, cSize + 1 , buff , ARRAY_SIZE(buff));
#else
	//setlocale(LC_ALL, "cht");
	mbstowcs(buff, c, cSize);
#endif
	return buff;
}


template< class CharT >
CharT const* FCString::FindChar(CharT const* str, CharT c)
{
	while (*str != 0)
	{
		if (*str == c)
			break;
		++str;
	}
	return str;
}

#define FUNCTION_LIST( CharT )\
	template CharT const* FCString::FindChar<CharT>(CharT const* str, CharT c);


FUNCTION_LIST(char)
FUNCTION_LIST(wchar_t)
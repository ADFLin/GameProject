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

thread_local wchar_t GWCharBuff[1024 * 256];
std::wstring FCString::CharToWChar(const char *str)
{
	const size_t cSize = FCString::Strlen(str) + 1;
#if SYS_PLATFORM_WIN
	::MultiByteToWideChar(0,0, str, cSize + 1 , GWCharBuff, ARRAY_SIZE(GWCharBuff));
#else
	//setlocale(LC_ALL, "cht");
	mbstowcs(GWCharBuff, str, cSize);
#endif
	return GWCharBuff;
}

thread_local char GCharBuff[1024 * 256];
std::string FCString::WCharToChar(const wchar_t* str)
{
#if SYS_PLATFORM_WIN
	const size_t cSize = FCString::Strlen(str) + 1;
	::WideCharToMultiByte(CP_ACP, 0, str, cSize, GCharBuff, ARRAY_SIZE(GCharBuff), NULL, NULL);
#else
	wcstombs(GCharBuff, str, cSize);
#endif
	return GCharBuff;
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
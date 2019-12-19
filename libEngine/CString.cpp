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

std::wstring FCString::CharToWChar(const char *c)
{
	const size_t cSize = strlen(c) + 1;
	wchar_t buff[1024 * 256];
#if SYS_PLATFORM_WIN
	::MultiByteToWideChar(0,0, c, cSize + 1 , buff , ARRAY_SIZE(buff));
#else
	//setlocale(LC_ALL, "cht");
	mbstowcs(buff, c, cSize);
#endif
	return buff;
}

#include "CString.h"
#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#include <Psapi.h>
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

template< class CharT >
uint32 FCString::StriHash(CharT const* str)
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

template< class CharT >
uint32 FCString::StriHash(CharT const* str, int len)
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

template< class CharT >
uint32 FCString::StrHash(CharT const* str)
{
	uint32 result = 5381;
	while (*str)
	{
		uint32 c = (uint32)*str;
		result = ((result << 5) + result) + c; /* hash * 33 + c */
		++str;
	}
	return result;
}

template< class CharT >
uint32 FCString::StrHash(CharT const* str, int len)
{
	uint32 result = 5381;
	while (len)
	{
		uint32 c = (uint32)*str;
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
	const size_t cSize = FCString::Strlen(str) + 1;
#if SYS_PLATFORM_WIN
	::WideCharToMultiByte(CP_ACP, 0, str, cSize, GCharBuff, ARRAY_SIZE(GCharBuff), NULL, NULL);
#else
	wcstombs(GCharBuff, str, cSize);
#endif
	return GCharBuff;
}

#if SYS_PLATFORM_WIN
inline HMODULE GetCurrentModuleHandle()
{
#if 0
	EXTERN_C IMAGE_DOS_HEADER __ImageBase;
	return (HMODULE)&__ImageBase;
#else
	DWORD flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
	HMODULE hModule = 0;
	::GetModuleHandleEx(flags, reinterpret_cast<LPCTSTR>(GetCurrentModuleHandle), &hModule);
	return hModule;
#endif
}
#endif

bool FCString::IsConstSegment(void const* ptr)
{
#if SYS_PLATFORM_WIN
	struct Local
	{
		Local()
		{
			::GetModuleInformation(GetCurrentProcess(), GetCurrentModuleHandle(), &moduleInfo, sizeof(moduleInfo));
		}

		bool Test(void const* ptr)
		{
			return uintptr_t(moduleInfo.lpBaseOfDll) < uintptr_t(ptr) && uintptr_t(ptr) < uintptr_t(moduleInfo.lpBaseOfDll) + uintptr_t(moduleInfo.SizeOfImage);
		}
		MODULEINFO moduleInfo;
	};

	static Local StaticLocal;
	return StaticLocal.Test(ptr);
#else
	static char const* Test = "TestAddr";
	return Math::Abs(intptr_t(ptr) - intptr_t(Test)) < 1 * 1024 * 1024;
#endif
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
	template CharT const* FCString::FindChar<CharT>(CharT const* str, CharT c);\
	template uint32 FCString::StriHash<CharT>(CharT const* str);\
	template uint32 FCString::StriHash<CharT>(CharT const* str, int len);\
	template uint32 FCString::StrHash<CharT>(CharT const* str);\
	template uint32 FCString::StrHash<CharT>(CharT const* str, int len);\


FUNCTION_LIST(char)
FUNCTION_LIST(wchar_t)
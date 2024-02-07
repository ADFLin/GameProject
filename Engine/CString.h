#pragma once
#ifndef CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912
#define CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912

#include "MemorySecurity.h"
#include "Meta/MetaBase.h"
#include "Core/IntegerType.h"
#include "Template/TypeFormatTraits.h"

#include <string>
#include <cassert>
#include <stdarg.h>
#define USE_UNICODE 0

#if USE_UNICODE
typedef wchar_t TChar;
#define TSTR( str ) L##str
#else
typedef char TChar;
#define TSTR( str ) str
#endif

template < class CharT >
class TStringTraits {};

template <>
class TStringTraits< char >
{
public:
	using CharT = char;
	using StdString = std::string;
};

template<>
struct TStringTraits< wchar_t >
{
	using CharT = wchar_t;
	using StdString = std::wstring;
};

template <class CharT>
struct TStringLiteral
{
	static constexpr CharT Select(char c, wchar_t) { return c; }
	static constexpr CharT const* Select(char const* c, wchar_t const*) { return c; }
};

template <>
struct TStringLiteral< wchar_t >
{
	static constexpr wchar_t Select(char, wchar_t c) { return c; }
	static constexpr wchar_t const* Select(char const*, wchar_t const* c) { return c; }
};

#define STRING_LITERAL( TYPE , LITERAL ) TStringLiteral< TYPE >::Select( LITERAL , L##LITERAL )

#if CPP_COMPILER_MSVC
#define STRING_FUNC_S_SUPPORTED 1
#else
#define STRING_FUNC_S_SUPPORTED 0
#endif

struct FCString
{
	template< int N >
	FORCEINLINE static int    PrintfV(char(&str)[N], char const* fmt, va_list al) 
	{ 
#if STRING_FUNC_S_SUPPORTED
		return ::vsprintf_s(str, fmt, al);
#else
		return ::vsprintf(str, fmt, al);
#endif
	}
	template< int N >
	FORCEINLINE static int    PrintfV(wchar_t(&str)[N], wchar_t const* fmt, va_list al) 
	{ 
#if STRING_FUNC_S_SUPPORTED
		return ::vswprintf_s(str, fmt, al); 
#else
		return ::vswprintf(str, fmt, al);
#endif
	}

	FORCEINLINE static int    PrintfV(char* str, int size , char const* fmt, va_list al) 
	{ 
#if STRING_FUNC_S_SUPPORTED
		return ::vsprintf_s(str, size, fmt, al); 
#else
		return ::vsprintf(str, fmt, al);
#endif
	}
	template< int N >
	FORCEINLINE static int    PrintfV(wchar_t* str, int size, wchar_t const* fmt, va_list al) 
	{
#if STRING_FUNC_S_SUPPORTED
		return ::vswprintf_s(str, size, fmt, al); 
#else
		return ::vswprintf(str,size, fmt, al);
#endif
	}

	template< int N >
	FORCEINLINE static void   Copy(char(&dst)[N], char const* src) 
	{ 
#if STRING_FUNC_S_SUPPORTED
		::strcpy_s(dst, src);
#else
		::strcpy(dst, src);
#endif
	}
	template< int N >
	FORCEINLINE static void   Copy(wchar_t(&dst)[N], wchar_t const* src) 
	{
#if STRING_FUNC_S_SUPPORTED
		::wcscpy_s(dst, src);
#else
		::wcscpy(dst, src);
#endif
	}

	template< int N >
	FORCEINLINE static void   CopyN(char(&dst)[N], char const* src, int num) 
	{
#if STRING_FUNC_S_SUPPORTED
		::strncpy_s(dst, src, num);
#else
		::strncpy(dst, src, num);
#endif
	}
	template< int N >
	FORCEINLINE static void   CopyN(wchar_t(&dst)[N], wchar_t const* src, int num)
	{ 
#if STRING_FUNC_S_SUPPORTED
		::wcsncpy_s(dst, src, num); 
#else
		::wcsncpy(dst, src, num);
#endif
	}

	FORCEINLINE static void   CopyN_Unsafe(char* dst, char const* src, int num)
	{
		::strncpy(dst, src, num);
	}

	FORCEINLINE static void   CopyN_Unsafe(wchar_t* dst, wchar_t const* src, int num)
	{
		::wcsncpy(dst, src, num);
	}

	template< int N >
	FORCEINLINE static void   Cat(char(&dst)[N], char const* src) 
	{ 
#if STRING_FUNC_S_SUPPORTED
		::strcat_s(dst, src); 
#else
		::strcat(dst, src);
#endif
	}
	template< int N >
	FORCEINLINE static void   Cat(wchar_t(&dst)[N], wchar_t const* src) 
	{ 
#if STRING_FUNC_S_SUPPORTED
		::wcscat_s(dst, src);
#else
		::wcscat(dst, src);
#endif
	}

	template< int N >
	FORCEINLINE static void   CatN(char(&dst)[N], char const* src, int num)
	{
#if STRING_FUNC_S_SUPPORTED
		::strncat_s(dst, src, num);
#else
		::strncat(dst, src, num);
#endif
	}
	template< int N >
	FORCEINLINE static void   CatN(wchar_t(&dst)[N], wchar_t const* src, int num)
	{
#if STRING_FUNC_S_SUPPORTED
		::wcsncat_s(dst, src, num);
#else
		::wcsncat(dst, src, num);
#endif
	}

	FORCEINLINE static int    Compare(char const* s1, char const* s2) { return ::strcmp(s1, s2); }
	FORCEINLINE static int    Compare(wchar_t const* s1, wchar_t const* s2) { return ::wcscmp(s1, s2); }

	FORCEINLINE static int    CompareIgnoreCase(char const* s1, char const* s2) 
	{
#if CPP_COMPILER_MSVC
		return ::_stricmp(s1, s2); 
#else
		return ::strcasecmp(s1, s2);
#endif
	}
	FORCEINLINE static int    CompareIgnoreCase(wchar_t const* s1, wchar_t const* s2) 
	{ 
#if CPP_COMPILER_MSVC
		return ::_wcsicmp(s1, s2); 
#else
		return ::wcscasecmp(s1, s2);
#endif
	}

	FORCEINLINE static int    CompareIgnoreCaseN(char const* s1, char const* s2, size_t num)
	{ 
#if CPP_COMPILER_MSVC
		return ::_strnicmp(s1, s2, num);
#else
		return ::strncasecmp(s1, s2, num);
#endif
	}
	FORCEINLINE static int    CompareIgnoreCaseN(wchar_t const* s1, wchar_t const* s2, size_t num)
	{ 
#if CPP_COMPILER_MSVC
		return ::_wcsnicmp(s1, s2, num); 
#else
		return ::wcsncasecmp(s1, s2, num);
#endif
	}

	FORCEINLINE static int    CompareN(char const* s1, char const* s2 , size_t num ) { return ::strncmp(s1, s2 , num ); }
	FORCEINLINE static int    CompareN(wchar_t const* s1, wchar_t const* s2 , size_t num ) { return ::wcsncmp(s1, s2 , num ); }

	FORCEINLINE static  char const* Strrchr(char const* str, char c) { return ::strrchr(str, c); }
	FORCEINLINE static  wchar_t const* Strrchr(wchar_t const* str, wchar_t c) { return ::wcsrchr(str, c); }

	FORCEINLINE static char const* StrStr(char const* s, char const* subStr) { return ::strstr(s, subStr); }
	FORCEINLINE static wchar_t const* StrStr(wchar_t const* s, wchar_t const* subStr) { return ::wcsstr(s, subStr); }

	FORCEINLINE static size_t  Strlen(char const* s) { return ::strlen(s); }
	FORCEINLINE static size_t  Strlen(wchar_t const* s) { return ::wcslen(s); }

	FORCEINLINE static float   Strtof(char const* s, char** end) { return ::strtof(s, end); }
	FORCEINLINE static float   Strtof(wchar_t const* s, wchar_t** end) { return ::wcstof(s, end); }

	FORCEINLINE static bool	   IsSpace(char c) { return ::isspace(c); }
	FORCEINLINE static bool	   IsSpace(wchar_t c) { return ::iswspace(c); }

	FORCEINLINE static bool	   IsAlpha(char c) { return ::isalpha(c); }
	FORCEINLINE static bool	   IsAlpha(wchar_t c) { return ::iswalpha(c); }

	FORCEINLINE static bool    IsDigit(char c) { return ::isdigit(c); }
	FORCEINLINE static bool    IsDigit(wchar_t c) { return ::iswdigit(c); }

	FORCEINLINE static bool    IsAlphaNumeric(char c) { return ::isalnum(c); }
	FORCEINLINE static bool    IsAlphaNumeric(wchar_t c) { return ::iswalnum(c); }

	template< typename CharT >
	FORCEINLINE static constexpr CharT ToLower(CharT c)
	{
		if (STRING_LITERAL(CharT, 'A') <= c && c <= STRING_LITERAL(CharT, 'Z'))
			return STRING_LITERAL(CharT, 'a') + (c - STRING_LITERAL(CharT, 'A'));
		return c;
	}

	template< typename CharT >
	FORCEINLINE static constexpr CharT ToUpper(CharT c)
	{
		if (STRING_LITERAL(CharT, 'a') <= c && c <= STRING_LITERAL(CharT, 'z'))
			return STRING_LITERAL(CharT, 'A') + (c - STRING_LITERAL(CharT, 'a'));
		return c;
	}

	template< typename CharT >
	static CharT const* FindChar(CharT const* str, CharT c);

	template< typename CharT >
	static CharT const* FindLastChar(CharT const* str, CharT c);

	template< typename CharT, typename T >
	static bool CheckForamtStringInternal(CharT const*& format, T&& t);

	template< typename CharT >
	static bool CheckForamtString(CharT const* format)
	{
		return true;
	}

	template< typename CharT, typename T >
	static bool CheckForamtString(CharT const* format, T&& t)
	{
		return CheckForamtStringInternal(format, std::forward<T>(t));
	}

	template< typename CharT , typename T, typename ...Args >
	static bool CheckForamtString(CharT const* format, T&& t , Args&& ...args )
	{
		if (!CheckForamtStringInternal(format, std::forward<T>(t)))
			return false;

		return CheckForamtString(format, std::forward<Args>(args)...);
	}

	template< typename CharT , int N , typename ...Args>
	FORCEINLINE static int  PrintfT(CharT(&str)[N], CharT const* fmt, Args&& ...args)
	{
		static_assert(Meta::And< TIsValidFormatType< Args >... >::Value == true , "Arg Type Error");
		//assert(CheckForamtString(fmt, args...));
		return FCString::PrintfImpl(str, N , fmt, args...);
	}

	template< typename CharT >
	FORCEINLINE static int  PrintfImpl(CharT* str , int size , CharT const* fmt, ...)
	{
		va_list argptr;
		va_start(argptr, fmt);
		int result = FCString::PrintfV(str, size, fmt, argptr);
		va_end(argptr);
		return result;
	}

	static void Stricpy(char * dest, char const* src);

	template< typename CharT >
	static uint32 StriHash(CharT const* str);
	template< typename CharT >
	static uint32 StriHash(CharT const* str, int len);
	template< typename CharT >
	static uint32 StrHash(CharT const* str);
	template< typename CharT >
	static uint32 StrHash(CharT const* str, int len);

	static std::wstring CharToWChar(const char *c);
	static std::string WCharToChar(const wchar_t* str);

	static bool IsConstSegment(void const* ptr);
};

template< typename CharT, typename T >
bool FCString::CheckForamtStringInternal(CharT const*& format, T&& t)
{
	format = FindChar(format, '%');
	if (*format == 0)
	{
		//LogWarning(0, "Format string args is less than inpput args");
		return false;
	}
	char const* strFormat = TTypeFormatTraits<std::remove_reference_t<T>>::GetString();
	int lenFormat = FCString::Strlen(strFormat);
	if (FCString::CompareN(strFormat, format, lenFormat) != 0)
	{
		//LogWarning(0, "Format string args is less than inpput args");
		return false;
	}
	format += lenFormat;
	return true;
}



#endif // CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912

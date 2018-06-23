#pragma once
#ifndef CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912
#define CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912

#include "MemorySecurity.h"
#include "MetaBase.h"
#include "Core/IntegerType.h"

#include <string>
#include <cassert>

template < class CharT >
class TStringTraits {};

template <>
class TStringTraits< char >
{
public:
	typedef char CharT;
	typedef std::string StdString;

};

template<>
struct TStringTraits< wchar_t >
{
	typedef wchar_t CharT;
	typedef std::wstring StdString;
};


struct FCString
{
	template< int N >
	static int    PrintfV(char(&str)[N], char const* fmt, va_list al) { return ::vsprintf_s(str, fmt, al); }
	template< int N >
	static void   PrintfV(wchar_t(&str)[N], wchar_t const* fmt, va_list al) { ::vswprintf_s(str, fmt, al); }

	template< int N >
	static void   Copy(char(&dst)[N], char const* src) { ::strcpy_s(dst, src); }
	template< int N >
	static void   Copy(wchar_t(&dst)[N], wchar_t const* src) { ::wcscpy_s(dst, src); }

	template< int N >
	static void   CopyN(char(&dst)[N], char const* src, int num) { ::strncpy_s(dst, src, num); }
	template< int N >
	static void   CopyN(wchar_t(&dst)[N], wchar_t const* src, int num) { ::wcsncpy_s(dst, src, num); }

	template< int N >
	static void   Cat(char(&dst)[N], char const* src) { ::strcat_s(dst, src); }
	template< int N >
	static void   Cat(wchar_t(&dst)[N], wchar_t const* src) { ::wcscat_s(dst, src); }

	static int    Compare(char const* s1, char const* s2) { return ::strcmp(s1, s2); }
	static int    Compare(wchar_t const* s1, wchar_t const* s2) { return ::wcscmp(s1, s2); }

	static int    CompareN(char const* s1, char const* s2 , size_t num ) { return ::strncmp(s1, s2 , num ); }
	static int    CompareN(wchar_t const* s1, wchar_t const* s2 , size_t num ) { return ::wcsncmp(s1, s2 , num ); }

	static  char const* Strrchr(char const* str, char c) { return ::strrchr(str, c); }
	static  wchar_t const* Strrchr(wchar_t const* str, wchar_t c) { return ::wcsrchr(str, c); }

	static size_t  Strlen(char const* s) { return ::strlen(s); }
	static size_t  Strlen(wchar_t const* s) { return ::wcslen(s); }

	static float   Strtof(char const* s, char** end) { return ::strtof(s, end); }
	static float   Strtof(wchar_t const* s, wchar_t** end) { return ::wcstof(s, end); }

	template <typename T>
	struct TIsValidFormatType
	{
	private:
		static uint32 Tester(uint32);
		static uint32 Tester(uint16);
		static uint32 Tester(uint8);
		static uint32 Tester(int32);
		static uint32 Tester(uint64);
		static uint32 Tester(int64);
		static uint32 Tester(short);
		static uint32 Tester(double);
		static uint32 Tester(float);
		static uint32 Tester(long);
		static uint32 Tester(unsigned long);
		static uint32 Tester(char);
		static uint32 Tester(bool);
		static uint32 Tester(const void*);
		static uint8  Tester(...);

		static T DeclValT();

	public:
		enum { Value = sizeof(Tester(DeclValT())) == sizeof(uint32) };
	};

	template< class CharT , int N , class ...Args>
	static void  PrintfT(CharT(&str)[N], CharT const* fmt, Args ...args)
	{
		static_assert(Meta::And< TIsValidFormatType< Args >... >::Value == true , "Arg Type Error");
		FCString::PrintfImpl(str, fmt, args...);
	}
	template< class CharT, int N>
	static void  PrintfImpl(CharT(&str)[N], CharT const* fmt, ...)
	{
		va_list argptr;
		va_start(argptr, fmt);
		FCString::PrintfV(str, fmt, argptr);
		va_end(argptr);
	}

	static void Stricpy(char * dest, char const* src)
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

	static uint32 StriHash(char const* str)
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

	
	static std::wstring CharToWChar(const char *c)
	{
		const size_t cSize = strlen(c) + 1;
		wchar_t buff[1024 * 256];
		setlocale(LC_ALL, "cht");
		mbstowcs(buff, c, cSize);
		return buff;
	}
};



#endif // CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912

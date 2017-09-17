#pragma once
#ifndef CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912
#define CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912

#include <string>
#include "MemorySecurity.h"

template < class CharT >
class TCString {};

template <>
class TCString< char >
{
public:
	typedef char CharT;
	typedef std::string StdString;
	template< int N >
	static int    PrintfV(CharT(&str)[N], CharT const* fmt, va_list al) { return ::vsprintf_s(str, fmt, al); }
	template< int N >
	static void   Copy(CharT(&dst)[N], CharT const* src) { ::strcpy_s(dst, src); }
	template< int N >
	static void   Cat(CharT(&dst)[N], CharT const* src) { ::strcat_s(dst, src); }
	static int    Compare(CharT const* s1, CharT const* s2) { return ::strcmp(s1, s2); }

	static  CharT const* Strrchr(CharT const* str, CharT c) { return ::strrchr(str, c); }
};

template<>
struct TCString< wchar_t >
{
	typedef wchar_t CharT;
	typedef std::wstring StdString;
	template< int N >
	static void   PrintfV(CharT(&str)[N], CharT const* fmt, va_list al) { ::vswprintf_s(str, fmt, al); }
	template< int N >
	static void   Copy(CharT(&dst)[N], CharT const* src) { ::wcscpy_s(dst, src); }
	template< int N >
	static void   Cat(CharT(&dst)[N], CharT const* src) { ::wcscat_s(dst, src); }
	static int    Compare(CharT const* s1, CharT const* s2) { return ::wcscmp(s1, s2); }

	static  CharT const* Strrchr(CharT const* str, CharT c) { return ::wcsrchr(str, c); }
};



#endif // CString_H_68B1FD83_57D7_4E26_B8F4_683C9097A912

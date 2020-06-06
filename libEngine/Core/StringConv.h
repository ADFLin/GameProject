#pragma once
#ifndef StringConv_H_1F1C04BF_8321_4E8A_BFBA_5C015EE4EAE4
#define StringConv_H_1F1C04BF_8321_4E8A_BFBA_5C015EE4EAE4

#include "CppVersion.h"
#include "CString.h"
#include "Template/TypeFormatTraits.h"

#if CPP_CHARCONV_SUPPORT
#include <charconv>
#endif

template< class T >
struct TStringFormatInitializer
{
	T value;
	template< class CharT, uint32 N >
	void operator()( CharT(&data)[N] ) const
	{
#if CPP_CHARCONV_SUPPORT
		CharT* ptr = std::to_chars(data, data + N, value).ptr;
		assert(ptr < data + N );
		*ptr = 0;
#else
		FCString::PrintfT(data, TTypeFormatTraits<T>::GetString(), value);
#endif
		
	}
};


template< class CharT, uint32 N >
class TInlineStringData
{
public:
	template< class T >
	TInlineStringData(TStringFormatInitializer<T> const& initializer) { initializer(mData); }

	TInlineStringData() = default;
	TInlineStringData(TInlineStringData const& other) = default;
	TInlineStringData& operator = (TInlineStringData const& other) = default;

	operator CharT const*() const { return mData; }
	operator CharT*      () { return mData; }
private:
	friend class FStringConv;
	CharT mData[N];
};

class FStringConv
{
public:

	template< class CharT = TChar, int N = 32 , class T >
	FORCEINLINE static TInlineStringData< CharT, N > From(T value)
	{
		return TStringFormatInitializer<T>{ value };
	}

	template< class CharT = TChar >
	FORCEINLINE static CharT const* From(bool value) { return (value) ? STRING_LITERAL(CharT, "1") : STRING_LITERAL(CharT, "0"); }

	template< class CharT = TChar >
	FORCEINLINE static CharT const* From(CharT const* value) { return value; }

	template< class T , class CharT = TChar >
	FORCEINLINE static bool ToCheck(CharT const* value, int len , T& outValue )
	{
#if CPP_CHARCONV_SUPPORT
		return std::from_chars(value, value + len, outValue).ec == std::errc();
#else
#error "No impl"
#endif

	}

	template< class CharT = TChar >
	FORCEINLINE static bool ToCheck(CharT const* value, int len, bool& outValue)
	{
#if CPP_CHARCONV_SUPPORT
		if (FCString::CompareIgnoreCase(STRING_LITERAL(CharT, "true"), value) == 0)
		{
			outValue = true;
			return true;
		}
		if (FCString::CompareIgnoreCase(STRING_LITERAL(CharT, "false"), value) == 0)
		{
			outValue = false;
			return true;
		}
		int temp = 0;
		if (std::from_chars(value, value + len, temp).ec == std::errc())
		{
			outValue = temp != 0;
			return true;
		}
		return false;
#else
#error "No impl"
#endif
	}

	template< class T, class CharT = TChar >
	FORCEINLINE static T To(CharT const* value, int len)
	{
#if CPP_CHARCONV_SUPPORT
		T result = T();
		std::from_chars(value, value +  len, result);
		return result;
#else
#error "No impl"
#endif
	}

	template< class T, class CharT = TChar >
	FORCEINLINE static T To(CharT const* value)
	{
#if CPP_CHARCONV_SUPPORT
		T result = T();
		std::from_chars(value, value + FCString::Strlen(value), result);
		return result;
#else
#error "No impl"
#endif
	}
};

#endif // StringConv_H_1F1C04BF_8321_4E8A_BFBA_5C015EE4EAE4

#pragma once
#ifndef StringConv_H_1F1C04BF_8321_4E8A_BFBA_5C015EE4EAE4
#define StringConv_H_1F1C04BF_8321_4E8A_BFBA_5C015EE4EAE4

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
		std::to_chars(data, data + N, value);
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

	TInlineStringData() {};
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
	FORCEINLINE CharT const* From(bool value) { return (value) ? STRING_LITERAL(CharT, "1") : STRING_LITERAL(CharT, "0"); }

};
#endif // StringConv_H_1F1C04BF_8321_4E8A_BFBA_5C015EE4EAE4

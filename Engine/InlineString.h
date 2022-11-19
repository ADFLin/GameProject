#ifndef InlineString_h__
#define InlineString_h__

#include "CString.h"
#include "Template/StringView.h"
#include "Meta/EnableIf.h"
#include "Meta/MetaBase.h"

#define INLINE_STRING_USE_LENGTH_MEMBER 0

template< int CHAR_COUNT, class T >
class TInlineString
{
	typedef T CharT;
	typedef typename TStringTraits< CharT >::StdString StdString;
public:
	FORCEINLINE TInlineString()
	{
		mData[0] = 0;
#if INLINE_STRING_USE_LENGTH_MEMBER
		mLength = 0;
#endif
	}

	FORCEINLINE explicit TInlineString(CharT const* str, int num) { assign(str, num); }

	template< typename Q , TEnableIf_Type< Meta::IsSameType< Q, T const* >::Value , bool > = true >
	TInlineString(Q str)
	{
		assign(str);
	}

	template< size_t M >
	explicit TInlineString(T const (&str)[M])
	{
		static_assert(M <= CHAR_COUNT);
		assign(str, M);
	}

	template< int M >
	FORCEINLINE TInlineString(TInlineString< M, CharT > const& other)
	{ 
		if constexpr (CHAR_COUNT < M)
		{
			CHECK(CHAR_COUNT >= other.length());
		}
#if INLINE_STRING_USE_LENGTH_MEMBER
		assign(other.mData , other.mLength);
#else
		assign(other.mData);
#endif
	}
	FORCEINLINE TInlineString(StdString const& str) { assign(str.c_str(), str.length()); }
	FORCEINLINE TInlineString(TStringView<T> const& str) { assign(str.data(), str.length()); }
	FORCEINLINE TInlineString(CharT const* str) { assign(str); }

	FORCEINLINE TInlineString& operator = (CharT const* str) { assign(str); return *this; }
	FORCEINLINE TInlineString& operator = (TInlineString< CHAR_COUNT, CharT > const& other)
	{
#if INLINE_STRING_USE_LENGTH_MEMBER
		assign(other.mData, other.mLength);
#else
		assign(other.mData);
#endif
		return *this; 
	}
	template< int M >
	FORCEINLINE TInlineString& operator = (TInlineString< M, CharT > const& other)
	{
#if INLINE_STRING_USE_LENGTH_MEMBER
		assign(other.mData, other.mLength);
#else
		assign(other.mData);
#endif 
		return *this; 
	}
	FORCEINLINE TInlineString& operator = (StdString const& str) { assign(str.c_str(), str.length()); return *this; }
	FORCEINLINE TInlineString& operator = (TStringView<T> const& str) { assign(str.data(), str.length());  return *this; }

	FORCEINLINE bool operator == (CharT const* str) const 
	{
		return compare(str) == 0;
	}
	FORCEINLINE bool operator != (CharT const* str) const
	{
		return compare(str) != 0; 
	}

	FORCEINLINE TInlineString& operator += (CharT const* str)
	{ 
#if INLINE_STRING_USE_LENGTH_MEMBER
		FCString::Cat(mData + mLength, str); 
		mLength = FCString::Strlen(str);
#else
		FCString::Cat(mData, str);
#endif
		return *this; 
	}
	FORCEINLINE TInlineString& operator += (StdString const& str)
	{ 
		append(str.c_str(), str.length());
		return *this; 
	}
	FORCEINLINE TInlineString& operator += (TStringView<CharT> const& str)
	{
		append(str.data(), str.length());
		return *this;
	}

	FORCEINLINE bool  empty() const 
	{ 
		return mData[0] == 0; 
	}
	FORCEINLINE void  assign(CharT const* str) 
	{ 
		FCString::Copy(mData, str); 
#if INLINE_STRING_USE_LENGTH_MEMBER
		mLength = FCString::Strlen(mData);
#endif
	}
	FORCEINLINE void  assign(CharT const* str, int num)
	{ 
		FCString::CopyN(mData, str, num);
#if INLINE_STRING_USE_LENGTH_MEMBER
		mLength = num;
#endif
	}

	FORCEINLINE void append(CharT const* str, int num)
	{
#if INLINE_STRING_USE_LENGTH_MEMBER
		FCString::CopyN_Unsafe(mData + mLength, str, num);
		mLength += num;
		mData[mLength] = 0;
#else
		FCString::CatN(mData, str, num);
#endif
	}

	FORCEINLINE void  clear()
	{ 
		mData[0] = 0;
#if INLINE_STRING_USE_LENGTH_MEMBER
		mLength = 0; 
#endif
	}
	FORCEINLINE size_t length() const 
	{ 
#if INLINE_STRING_USE_LENGTH_MEMBER
		return mLength;
#else
		return FCString::Strlen(mData);
#endif
	}
	
	template< class ...Args>
	FORCEINLINE int format(CharT const* fmt, Args&& ...args)
	{
		return FCString::PrintfT(mData, fmt , std::forward<Args>(args)...);
	}

	FORCEINLINE int formatVA(CharT const* fmt, va_list arg)
	{
		return FCString::PrintfV(mData, fmt, arg);
	}

	FORCEINLINE void replace(CharT from, CharT to)
	{
		CharT* ptr = mData;
		while(  *ptr != 0 )
		{
			if( *ptr == from )
				*ptr = to;
			++ptr;
		}
	}

	FORCEINLINE CharT const* c_str() const { return mData; }
	FORCEINLINE CharT* data() { return mData; }


	FORCEINLINE operator CharT const*() const { return mData; }
	FORCEINLINE operator CharT*      () { return mData; }

	FORCEINLINE int   compare(CharT const* str) const { return FCString::Compare(mData, str); }
	FORCEINLINE int   compareN(CharT const* str, int len) const { return FCString::CompareN(mData, str, len); }
	FORCEINLINE size_t max_size() const{  return CHAR_COUNT;  }


	template < typename ...Args >
	static TInlineString Make(CharT const* format, Args&& ...args)
	{
		TInlineString< CHAR_COUNT, CharT > str;
		int len = FCString::PrintfT(str.mData, format, std::forward<Args>(args)...);
#if INLINE_STRING_USE_LENGTH_MEMBER
		str.mLength = len;
#endif
		return str;
	}
private:

	template< int M , class Q >
	friend class TInlineString;
#if INLINE_STRING_USE_LENGTH_MEMBER
	size_t mLength;
#endif
	CharT  mData[CHAR_COUNT];
};

template< int CHAR_COUNT = 256 >
using InlineStringA = TInlineString< CHAR_COUNT, char >;

template< int CHAR_COUNT = 256 >
using InlineStringW = TInlineString< CHAR_COUNT, wchar_t >;

template< int CHAR_COUNT = 256 >
using InlineString = TInlineString< CHAR_COUNT, TChar >;

#endif // InlineString_h__

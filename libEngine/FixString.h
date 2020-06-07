#ifndef FixString_h__
#define FixString_h__

#include "CString.h"
#include "Template/StringView.h"
#include "Meta/EnableIf.h"
#include "Meta/MetaBase.h"

#define FIXSTRING_USE_LENGTH_MEMBER 0

template< int N , class T = TChar >
class FixString
{
	typedef T CharT;
	typedef typename TStringTraits< CharT >::StdString StdString;
public:
	FORCEINLINE FixString()
	{
		mData[0] = 0;
#if FIXSTRING_USE_LENGTH_MEMBER
		mLength = 0;
#endif
	}

	FORCEINLINE explicit FixString(CharT const* str, int num) { assign(str, num); }

	template< typename Q >
	explicit FixString(Q str, typename TEnableIf< Meta::IsSameType< T const*, Q >::Value >::Type* = 0)
	{
		assign(str);
	}

	template< size_t N >
	explicit FixString(T const (&str)[N])
	{
		assign(str, N);
	}

	template< int M >
	FORCEINLINE FixString(FixString< M, CharT > const& other) 
	{ 
#if FIXSTRING_USE_LENGTH_MEMBER
		assign(other.mData , other.mLength);
#else
		assign(other.mData);
#endif
	}
	FORCEINLINE FixString(StdString const& str) { assign(str.c_str(), str.length()); }

	FORCEINLINE FixString& operator = (CharT const* str) { assign(str); return *this; }
	FORCEINLINE FixString& operator = (FixString< N, CharT > const& other) { assign(other.mData);  return *this; }
	template< int M >
	FORCEINLINE FixString& operator = (FixString< M, CharT > const& other) { assign(other.mData);  return *this; }
	FORCEINLINE FixString& operator = (StdString const& str) { assign(str.c_str(), str.length()); return *this; }
	FORCEINLINE FixString& operator = (TStringView<T> const& str) { assign(str.data(), str.length());  return *this; }

	FORCEINLINE bool operator == (CharT const* str) const { return compare(str) == 0; }
	FORCEINLINE bool operator != (CharT const* str) const { return compare(str) != 0; }

	FORCEINLINE FixString& operator += (CharT const* str) { FCString::Cat(mData, str); return *this; }
	FORCEINLINE FixString& operator += (StdString const& str) { FCString::Cat(mData, str.c_str()); return *this; }

	FORCEINLINE bool  empty() const { return mData[0] == 0; }
	FORCEINLINE void  assign(CharT const* str) { FCString::Copy(mData, str); }
	FORCEINLINE void  assign(CharT const* str, int num)
	{ 
		FCString::CopyN(mData, str, num);
#if FIXSTRING_USE_LENGTH_MEMBER
		mLength = len; 
#endif
	}
	FORCEINLINE void  clear()
	{ 
		mData[0] = 0;
#if FIXSTRING_USE_LENGTH_MEMBER
		mLength = 0; 
#endif
	}
	FORCEINLINE size_t length() const 
	{ 
#if FIXSTRING_USE_LENGTH_MEMBER
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
	FORCEINLINE size_t max_size() const{  return N;  }


	template < int N = 512, typename CharT = TChar, typename ...Args >
	friend FixString< N > MakeString(CharT const* format, Args&& ...args);
private:
	
	template< int M , class T >
	friend  class FixString;
#if FIXSTRING_USE_LENGTH_MEMBER
	size_t mLength;
#endif
	CharT  mData[ N ];
};


template < int N = 512, typename CharT = TChar, typename ...Args >
FORCEINLINE FixString< N > MakeString(CharT const* format, Args&& ...args)
{
	FixString< N > str;
	int len = FCString::PrintfT(str.mData, format, std::forward<Args>(args)...);
#if FIXSTRING_USE_LENGTH_MEMBER
	str.mLength = len;
#endif
	return str;
}



#endif // FixString_h__

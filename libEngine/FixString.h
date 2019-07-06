#ifndef FixString_h__
#define FixString_h__

#include "CString.h"
#include "Template/StringView.h"

template< int N , class T = TChar >
class FixString
{
	typedef T CharT;
	typedef typename TStringTraits< CharT >::StdString StdString;
public:
	FORCEINLINE FixString() { mData[0] = 0; }
	FORCEINLINE explicit FixString(CharT const* str) { assign(str); }
	FORCEINLINE explicit FixString(CharT const* str, int num) { FCString::CopyN(mData, str, num); }
	template< int M >
	FORCEINLINE FixString(FixString< M, CharT > const& other) { assign(other.mData); }
	FORCEINLINE FixString(StdString const& str) { assign(str.c_str()); }

	FORCEINLINE FixString& operator = (CharT const* str) { assign(str); return *this; }
	FORCEINLINE FixString& operator = (FixString< N, CharT > const& other) { assign(other.mData);  return *this; }
	template< int M >
	FORCEINLINE FixString& operator = (FixString< M, CharT > const& other) { assign(other.mData);  return *this; }
	FORCEINLINE FixString& operator = (StdString const& str) { assign(str.c_str()); return *this; }
	FORCEINLINE FixString& operator = (TStringView<T> const& str) { assign(str.data(), str.length());  return *this; }

	FORCEINLINE bool operator == (CharT const* str) const { return compare(str) == 0; }
	FORCEINLINE bool operator != (CharT const* str) const { return compare(str) != 0; }

	FORCEINLINE FixString& operator += (CharT const* str) { FCString::Cat(mData, str); return *this; }
	FORCEINLINE FixString& operator += (StdString const& str) { FCString::Cat(mData, str.c_str()); return *this; }

	FORCEINLINE bool  empty() const { return mData[0] == 0; }
	FORCEINLINE void  assign(CharT const* str) { FCString::Copy(mData, str); }
	FORCEINLINE void  assign(CharT const* str, int num) { FCString::CopyN(mData, str, num); }
	FORCEINLINE void  clear() { mData[0] = 0; }
	FORCEINLINE size_t length() const { return FCString::Strlen(mData); }
	
	template< class ...Args>
	FORCEINLINE FixString& format(CharT const* fmt, Args&& ...args)
	{
		FCString::PrintfT(mData, fmt , args...);
		return *this;
	}

	FORCEINLINE FixString& formatVA(CharT const* fmt, va_list arg)
	{
		FCString::PrintfV(mData, fmt, arg);
		return *this;
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
	FORCEINLINE size_t max_size() const{  return N;  }

private:
	
	template< int M , class T >
	friend  class FixString;

	CharT mData[ N ];
};


#endif // FixString_h__

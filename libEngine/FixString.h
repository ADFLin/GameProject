#ifndef FixString_h__
#define FixString_h__

#include "CString.h"
#include "Template/StringView.h"

template< int N , class T = char >
class FixString
{
	typedef T CharT;
	typedef typename TStringTraits< CharT >::StdString StdString;
public:
	FixString() { mStr[0] = 0; }
	explicit FixString(CharT const* str) { assign(str); }
	explicit FixString(CharT const* str, int num) { FCString::CopyN(mStr, str, num); }
	template< int M >
	FixString(FixString< M, CharT > const& other) { assign(other.mStr); }
	FixString(StdString const& str) { assign(str.c_str()); }

	FixString& operator = (CharT const* str) { assign(str); return *this; }
	FixString& operator = (FixString< N, CharT > const& other) { assign(other.mStr);  return *this; }
	template< int M >
	FixString& operator = (FixString< M, CharT > const& other) { assign(other.mStr);  return *this; }
	FixString& operator = (StdString const& str) { assign(str.c_str()); return *this; }
	FixString& operator = (TStringView<T> const& str) { assign(str.data(), str.length());  return *this; }

	bool operator == (CharT const* str) const { return compare(str) == 0; }
	bool operator != (CharT const* str) const { return compare(str) != 0; }

	FixString& operator += (CharT const* str) { FCString::Cat(mStr, str); return *this; }
	FixString& operator += (StdString const& str) { FCString::Cat(mStr, str.c_str()); return *this; }

	bool  empty() const { return mStr[0] == 0; }
	void  assign(CharT const* str) { FCString::Copy(mStr, str); }
	void  assign(CharT const* str, int num) { FCString::CopyN(mStr, str, num); }
	void  clear() { mStr[0] = 0; }
	
	template< class ...Args>
	FixString& format(CharT const* fmt, Args ...args)
	{
		FCString::PrintfT(mStr, fmt , args...);
		return *this;
	}

	FixString& formatVA(CharT const* fmt, va_list arg)
	{
		FCString::PrintfV(mStr, fmt, arg);
		return *this;
	}

	void replace(CharT from, CharT to)
	{
		CharT* ptr = mStr;
		while(  *ptr != 0 )
		{
			if( *ptr == from )
				*ptr = to;
			++ptr;
		}
	}

	CharT const* c_str() const { return mStr; }
	CharT* data() { return mStr; }


	operator CharT const*() const { return mStr; }
	operator CharT*      () { return mStr; }

	int   compare(CharT const* str) const { return FCString::Compare(mStr, str); }
	size_t max_size() const{  return N;  }

private:
	
	template< int M , class T >
	friend  class FixString;

	CharT mStr[ N ];
};

#endif // FixString_h__

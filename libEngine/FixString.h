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
	FixString() { mData[0] = 0; }
	explicit FixString(CharT const* str) { assign(str); }
	explicit FixString(CharT const* str, int num) { FCString::CopyN(mData, str, num); }
	template< int M >
	FixString(FixString< M, CharT > const& other) { assign(other.mData); }
	FixString(StdString const& str) { assign(str.c_str()); }

	FixString& operator = (CharT const* str) { assign(str); return *this; }
	FixString& operator = (FixString< N, CharT > const& other) { assign(other.mData);  return *this; }
	template< int M >
	FixString& operator = (FixString< M, CharT > const& other) { assign(other.mData);  return *this; }
	FixString& operator = (StdString const& str) { assign(str.c_str()); return *this; }
	FixString& operator = (TStringView<T> const& str) { assign(str.data(), str.length());  return *this; }

	bool operator == (CharT const* str) const { return compare(str) == 0; }
	bool operator != (CharT const* str) const { return compare(str) != 0; }

	FixString& operator += (CharT const* str) { FCString::Cat(mData, str); return *this; }
	FixString& operator += (StdString const& str) { FCString::Cat(mData, str.c_str()); return *this; }

	bool  empty() const { return mData[0] == 0; }
	void  assign(CharT const* str) { FCString::Copy(mData, str); }
	void  assign(CharT const* str, int num) { FCString::CopyN(mData, str, num); }
	void  clear() { mData[0] = 0; }
	size_t length() const { return FCString::Strlen(mData); }
	
	template< class ...Args>
	FixString& format(CharT const* fmt, Args&& ...args)
	{
		FCString::PrintfT(mData, fmt , args...);
		return *this;
	}

	FixString& formatVA(CharT const* fmt, va_list arg)
	{
		FCString::PrintfV(mData, fmt, arg);
		return *this;
	}

	void replace(CharT from, CharT to)
	{
		CharT* ptr = mData;
		while(  *ptr != 0 )
		{
			if( *ptr == from )
				*ptr = to;
			++ptr;
		}
	}

	CharT const* c_str() const { return mData; }
	CharT* data() { return mData; }


	operator CharT const*() const { return mData; }
	operator CharT*      () { return mData; }

	int   compare(CharT const* str) const { return FCString::Compare(mData, str); }
	size_t max_size() const{  return N;  }

private:
	
	template< int M , class T >
	friend  class FixString;

	CharT mData[ N ];
};


#endif // FixString_h__

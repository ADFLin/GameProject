#ifndef FixString_h__
#define FixString_h__

#include "CString.h"


template< int N , class T = char >
class FixString
{
	typedef T CharT;
	typedef TCString< CharT > FCString;
	typedef typename FCString::StdString StdString;
public:
	FixString(){ mStr[0] = 0; }
	explicit FixString( CharT const* str ){ assign( str ); }
	explicit FixString(CharT const* str, int num) { FCString::CopyN(mStr, str, num); }
	template< int M >
	FixString( FixString< M , CharT > const& other ){ assign( other.mStr );  }
	FixString( StdString const& str ){ assign( str.c_str() ); }

	FixString& operator = ( CharT const* str ){ assign( str ); return *this; }
	FixString& operator = ( FixString< N , CharT > const& other ){ assign( other.mStr );  return *this;  }
	template< int M >
	FixString& operator = ( FixString< M , CharT > const& other ){ assign( other.mStr );  return *this;  }
	FixString& operator = ( StdString const& str ){ assign( str.c_str() ); return *this; }

	bool operator == ( CharT const* str ) const { return compare( str ) == 0; }
	bool operator != ( CharT const* str ) const { return compare( str ) != 0; }

	FixString& operator += ( CharT const* str ){ FCString::Cat( mStr , str ); return *this; }
	FixString& operator += ( StdString const& str ){ FCString::Cat( mStr , str.c_str() ); return *this; }


	void       clear(){ mStr[0] = 0; }
	FixString& format( CharT const* fmt , ... )
	{
		va_list argptr;
		va_start( argptr, fmt );
		FCString::PrintfV( mStr , fmt , argptr );
		va_end( argptr );
		return *this;
	}

	FixString& formatVA( CharT const* fmt , va_list arg )
	{
		FCString::PrintfV( mStr , fmt , arg );
		return *this;
	}


	CharT const* c_str() const { return mStr; }
	

	operator CharT const*( ) const { return mStr; }
	operator CharT*      ( )       { return mStr; }

	int   compare(CharT const* str) const { return FCString::Compare(mStr, str); }

private:
	
	void  assign( CharT const* str ){ FCString::Copy( mStr , str );  }

	template< int M , class T >
	friend  class FixString;

	CharT mStr[ N ];
};

#endif // FixString_h__

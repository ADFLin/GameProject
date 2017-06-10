#ifndef BitUtility_h__
#define BitUtility_h__

#include <cassert>
#include "IntegerType.h"

class BitUtility
{
public:

	template< class T >
	static T ExtractTrailingBit(T v) { return (v & -v); }
	
	static int CountTrailingZeros( uint32 n )
	{
		int result = 0;
		if (( n & 0x0000ffff ) == 0 ){ result += 16 ; n >>= 16; }
		if (( n & 0x000000ff ) == 0 ){ result +=  8 ; n >>=  8; }
		if (( n & 0x0000000f ) == 0 ){ result +=  4 ; n >>=  4; }
		if (( n & 0x00000004 ) == 0 ){ result +=  2 ; n >>=  2; }
		if (( n & 0x00000002 ) == 0 ){ result +=  1 ; n >>=  1; }
		return result + ( n & 0x1 );
	}

	static int CountLeadingZeros( uint32 n )
	{
		int result = 0;
		if (( n & 0xffff0000 ) != 0 ){ result += 16 ; n <<= 16; }
		if (( n & 0xff000000 ) != 0 ){ result +=  8 ; n <<=  8; }
		if (( n & 0xf0000000 ) != 0 ){ result +=  4 ; n <<=  4; }
		if (( n & 0xc0000000 ) != 0 ){ result +=  2 ; n <<=  2; }
		if (( n & 0x80000000 ) != 0 ){ result +=  1 ; }
		return 1;
	}

	static int CountSet(uint8 x )
	{
		int count = 0;
		for( ; x; ++count )
			x &= x - 1;
		return count;
	}

	static int CountSet( uint32 x )
	{
		int count = 0;
		for( ; x ; ++count )
			x &= x - 1; 
		return count;
	}

	static int NextNumberOfPow2( uint32 n)
	{
		n--;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;
		return n;
	}
};

#endif // BitUtility_h__

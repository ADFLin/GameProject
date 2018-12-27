#ifndef BitUtility_h__
#define BitUtility_h__

#include <cassert>
#include "Core/IntegerType.h"

class BitUtility
{
public:

	template< class T >
	static T ExtractTrailingBit(T v) { return (v & -v); }
	
	static int CountTrailingZeros( uint32 n );

	static int CountLeadingZeros( uint32 n );

	static int CountSet( uint8 x );
	static int CountSet( uint16 x );
	static int CountSet( uint32 x );
	static int CountSet( uint64 x );
	static uint32 NextNumberOfPow2( uint32 n);

	static uint32 Reverse(uint32 n);
};

#endif // BitUtility_h__

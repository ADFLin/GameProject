#ifndef BitUtility_h__
#define BitUtility_h__

#include "PlatformConfig.h"
#include "Core/IntegerType.h"

#include <cassert>

class FBitUtility
{
public:

	template< class T >
	static T ExtractTrailingBit(T v) { return (v & -v); }

	template< class T >
	static bool IsOneBitSet(T v) { return !(v & (v - 1)); }
	
	static int CountTrailingZeros( uint32 n );

	static int CountLeadingZeros( uint32 n );

	static int CountSet( uint8 x );
	static int CountSet( uint16 x );
	static int CountSet( uint32 x );
	static int CountSet( uint64 x );
	static uint32 NextNumberOfPow2( uint32 n);

	static uint32 Reverse(uint32 n);

	static int ToIndex4(unsigned bit);
	static int ToIndex8(unsigned bit);
	static int ToIndex32(unsigned bit);
#if TARGET_PLATFORM_64BITS
	static int ToIndex64(unsigned bit);
#endif
	static unsigned RotateRight(unsigned bits, unsigned offset, unsigned numBit);
	static unsigned RotateLeft(unsigned bits, unsigned offset, unsigned numBit);

	template< unsigned NumBits >
	static int ToIndex(unsigned bit)
	{
		return ToIndex32(bit);
	}
	template<>
	static int ToIndex<4>(unsigned bit) { return ToIndex4(bit); }
	template<>
	static int ToIndex<8>(unsigned bit) { return ToIndex8(bit); }

	template< unsigned NumBits >
	static bool IterateMask(unsigned& mask, int& index)
	{
		if (mask == 0)
			return false;
		unsigned bit = FBitUtility::ExtractTrailingBit(mask);
		index = FBitUtility::ToIndex< NumBits >(bit);
		mask &= ~bit;
		return true;
	}
};

#endif // BitUtility_h__

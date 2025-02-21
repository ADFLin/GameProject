#ifndef BitUtility_h__
#define BitUtility_h__

#include "PlatformConfig.h"
#include "Core/IntegerType.h"
#include "Meta/EnableIf.h"

#include <cassert>

template< typename T >
static T AlignArbitrary(T size, T alignment)
{
	return ((size + alignment - 1) / alignment) * alignment;
}

class FBitUtility
{
public:

	template< class T >
	static T ExtractTrailingBit(T v) { return v & (~v + 1); }

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
	static uint64 Reverse(uint64 n);

	static int ToIndex4(unsigned bit);
	static int ToIndex8(unsigned bit);
	static int ToIndex16(unsigned bit);
	static int ToIndex32(unsigned bit);
	static int ToIndex64(uint64 bit);
	static unsigned RotateRight(unsigned bits, unsigned offset, unsigned numBit);
	static unsigned RotateLeft(unsigned bits, unsigned offset, unsigned numBit);
	template< unsigned NumBits, TEnableIf_Type< NumBits <= 4, bool > = true >
	static int ToIndex(unsigned bit)
	{
		return ToIndex4(bit);
	}
	template< unsigned NumBits, TEnableIf_Type< 4 < NumBits && NumBits <= 8, bool > = true >
	static int ToIndex(unsigned bit) 
	{ 
		return ToIndex8(bit); 
	}
	template< unsigned NumBits, TEnableIf_Type< 8 < NumBits && NumBits <= 16, bool > = true >
	static int ToIndex(unsigned bit)
	{ 
		return ToIndex16(bit); 
	}
	template< unsigned NumBits , TEnableIf_Type< 16 < NumBits && NumBits <= 32 , bool > = true >
	static int ToIndex(unsigned bit)
	{
		return ToIndex32(bit);
	}
#if TARGET_PLATFORM_64BITS
	template< unsigned NumBits , TEnableIf_Type< 32 < NumBits , bool > = true >
	static int ToIndex(uint64 bit)
	{
		return ToIndex64(bit);
	}
#endif


	template< unsigned NumBits , typename MaskType >
	static bool IterateMask(MaskType& mask, int& index)
	{
		if (mask == 0)
			return false;
		MaskType bit = FBitUtility::ExtractTrailingBit(mask);
		index = FBitUtility::ToIndex< NumBits >(bit);
		mask &= ~bit;
		return true;
	}

	template< unsigned NumBits, typename MaskType >
	static bool IterateMaskRange(MaskType& mask, int& index, int& count)
	{
		if (mask == 0)
			return false;
		MaskType bit = FBitUtility::ExtractTrailingBit(mask);
		index = FBitUtility::ToIndex< NumBits >(bit);
		mask &= ~bit;
		count = 1;
		for (;;)
		{
			bit <<= 1;
			if (!(mask & bit))
				break;

			mask &= ~bit;
			count += 1;
		}
		return true;
	}
};

#endif // BitUtility_h__

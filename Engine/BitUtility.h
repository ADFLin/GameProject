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
	template< unsigned NumBits, typename T >
	static int ToIndex(T bit)
	{
		if constexpr (NumBits <= 4)
		{
			return ToIndex4(static_cast<unsigned>(bit));
		}
		else if constexpr (NumBits <= 8)
		{
			return ToIndex8(static_cast<unsigned>(bit));
		}
		else if constexpr (NumBits <= 16)
		{
			return ToIndex16(static_cast<unsigned>(bit));
		}
		else if constexpr (NumBits <= 32)
		{
			return ToIndex32(static_cast<unsigned>(bit));
		}
		else
		{
#if TARGET_PLATFORM_64BITS
			return ToIndex64(static_cast<uint64>(bit));
#else
			static_assert(NumBits <= 32, "Bit Index > 32 not supported on 32-bit platform");
			return 0;
#endif
		}
	}


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
		MaskType added = mask + bit;
		MaskType changed = added ^ mask;
		mask &= ~changed;

		count = FBitUtility::CountSet(changed);
		if constexpr (NumBits < sizeof(MaskType) * 8)
		{
			count -= 1;
		}
		else
		{
			if ((changed + bit) != 0)
			{
				count -= 1;
			}
		}
		return true;
	}
};

#endif // BitUtility_h__

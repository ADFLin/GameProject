#include "BitUtility.h"
#include "CompilerConfig.h"
#include "MarcoCommon.h"

#if CPP_COMPILER_MSVC
#include <intrin.h>
#endif

int FBitUtility::CountTrailingZeros(uint32 n)
{
#if CPP_COMPILER_MSVC
	constexpr int TotalBitCount = sizeof(uint32) * 8;
	unsigned long index = 0;
	if (!_BitScanForward(&index, n))
		return TotalBitCount;
	return index;
#else
	int result = 0;
	if( !(n & 0x0000ffff) ) { result += 16; n >>= 16; }
	if( !(n & 0x000000ff) ) { result += 8; n >>= 8; }
	if( !(n & 0x0000000f) ) { result += 4; n >>= 4; }
	if( !(n & 0x00000003) ) { result += 2; n >>= 2; }
	if( !(n & 0x00000001) ) { result += 1; n >>= 1; }
	return result;
#endif
}

int FBitUtility::CountLeadingZeros(uint32 n)
{
#if CPP_COMPILER_MSVC
	constexpr int TotalBitCount = sizeof(uint32) * 8;
	unsigned long index = 0;
	if (!_BitScanReverse(&index, n))
		return TotalBitCount;
	return TotalBitCount - index - 1;
#else
	int result = 0;
	if( !(n & 0xffff0000) ) { result += 16; n <<= 16; }
	if( !(n & 0xff000000) ) { result += 8; n <<= 8; }
	if( !(n & 0xf0000000) ) { result += 4; n <<= 4; }
	if( !(n & 0xc0000000) ) { result += 2; n <<= 2; }
	if( !(n & 0x80000000) ) { result += 1; }
	return result;
#endif
}


template< class T >
int BitCountSetGentic(T x)
{
	int count = 0;
	for( ; x; ++count )
		x &= x - 1;
	return count;
}

#define TWO(c)     (0x1u << (c))
#define MASK(c)    (((unsigned int)(-1)) / (TWO(TWO(c)) + 1u))
#define COUNT(x,c) ((x) & MASK(c)) + (((x) >> (TWO(c))) & MASK(c))

#define MASK64(c)    (((uint64)(-1)) / (TWO(TWO(c)) + 1u))
#define COUNT64(x,c) ((x) & MASK64(c)) + (((x) >> (TWO(c))) & MASK64(c))
int FBitUtility::CountSet(uint8 x)
{
#if CPP_COMPILER_MSVC
	return __popcnt16(x);
#else
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	return x;
#endif
}


int FBitUtility::CountSet(uint16 x)
{
#if CPP_COMPILER_MSVC
	return __popcnt16(x);
#else
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	x = COUNT(x, 3);
	return x;
#endif
}

int FBitUtility::CountSet(uint32 x)
{
#if CPP_COMPILER_MSVC
	return __popcnt(x);
#else
	x = COUNT(x, 0);
	x = COUNT(x, 1);
	x = COUNT(x, 2);
	x = COUNT(x, 3);
	x = COUNT(x, 4);
	return x;
#endif
}


int FBitUtility::CountSet(uint64 x)
{
#if CPP_COMPILER_MSVC && TARGET_PLATFORM_64BITS
	return __popcnt64(x);
#else
	x = COUNT64(x, 0);
	x = COUNT64(x, 1);
	x = COUNT64(x, 2);
	x = COUNT64(x, 3);
	x = COUNT64(x, 4);
	x = COUNT64(x, 5);
	return x;
#endif
}

uint32 FBitUtility::NextNumberOfPow2(uint32 n)
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

uint32 FBitUtility::Reverse(uint32 n)
{
	n = ((n & 0xaaaaaaaa) >> 1) | ((n & 0x55555555) << 1);
	n = ((n & 0xcccccccc) >> 2) | ((n & 0x33333333) << 2);
	n = ((n & 0xf0f0f0f0) >> 4) | ((n & 0x0f0f0f0f) << 4);
	n = ((n & 0xff00ff00) >> 8) | ((n & 0x00ff00ff) << 8);
	return (n >> 16) | (n << 16);
}

uint64 FBitUtility::Reverse(uint64 n)
{
	n = ((n & 0xaaaaaaaaaaaaaaaallu) >> 1) | ((n & 0x5555555555555555llu) << 1);
	n = ((n & 0xccccccccccccccccllu) >> 2) | ((n & 0x3333333333333333llu) << 2);
	n = ((n & 0xf0f0f0f0f0f0f0f0llu) >> 4) | ((n & 0x0f0f0f0f0f0f0f0fllu) << 4);
	n = ((n & 0xff00ff00ff00ff00llu) >> 8) | ((n & 0x00ff00ff00ff00ffllu) << 8);
	n = ((n & 0xffff0000ffff0000llu) >> 16) | ((n & 0x0000ffff0000ffffllu) << 16);
	return (n >> 32) | (n << 32);
}
static int const gBitIndexMap[9] = 
{ 
	0 , 
	0, 1, 1, 2, 2, 2, 2, 3, 
};

int FBitUtility::ToIndex4(unsigned bit)
{
	CHECK((bit & 0xf) == bit);
	CHECK(IsOneBitSet(bit));
#if CPP_COMPILER_MSVC
	unsigned long index;
	_BitScanForward(&index, bit);
	return index;
#else
	return gBitIndexMap[bit];
#endif
}
int FBitUtility::ToIndex8(unsigned bit)
{
	CHECK((bit & 0xff) == bit);
	CHECK(IsOneBitSet(bit));
#if CPP_COMPILER_MSVC
	unsigned long index;
	_BitScanForward(&index, bit);
	return index;
#else
	int result = 0;
	if (bit & 0xf0) { result += 4; bit >>= 4; }
	return result + gBitIndexMap[bit];
#endif
}

int FBitUtility::ToIndex16(unsigned bit)
{
	CHECK((bit & 0xffff) == bit);
	CHECK(IsOneBitSet(bit));
#if CPP_COMPILER_MSVC
	unsigned long index;
	_BitScanForward(&index, bit);
	return index;
#else
	int result = 0;
	if (bit & 0x0000ff00) { result += 8; bit >>= 8; }
	if (bit & 0x000000f0) { result += 4; bit >>= 4; }
	return result + gBitIndexMap[bit];
#endif
}

int FBitUtility::ToIndex32(unsigned bit)
{
	CHECK((bit & 0xffffffff) == bit);
	CHECK(IsOneBitSet(bit));
#if CPP_COMPILER_MSVC
	unsigned long index;
	_BitScanForward(&index, bit);
	return index;
#else
	int result = 0;
	if (bit & 0xffff0000) { result += 16; bit >>= 16; }
	if (bit & 0x0000ff00) { result += 8; bit >>= 8; }
	if (bit & 0x000000f0) { result += 4; bit >>= 4; }
	return result + gBitIndexMap[bit];
#endif
}

int FBitUtility::ToIndex64(uint64 bit)
{
	CHECK((bit & 0xffffffffffffffffULL) == bit);
	CHECK(IsOneBitSet(bit));
#if CPP_COMPILER_MSVC
	unsigned long index;
	_BitScanForward64(&index, bit);
	return index;
#else
	int result = 0;
	if (bit & 0xffffffff00000000ULL) { result += 32; bit >>= 32; }
	if (bit & 0x00000000ffff0000ULL) { result += 16; bit >>= 16; }
	if (bit & 0x000000000000ff00ULL) { result += 8; bit >>= 8; }
	if (bit & 0x00000000000000f0ULL) { result += 4; bit >>= 4; }
	return result + gBitIndexMap[bit];
#endif
}

unsigned FBitUtility::RotateRight(unsigned bits, unsigned offset, unsigned numBit)
{
	CHECK(offset <= numBit);
	unsigned mask = (1 << numBit) - 1;
	return ((bits >> offset) | (bits << (numBit - offset))) & mask;
}

unsigned FBitUtility::RotateLeft(unsigned bits, unsigned offset, unsigned numBit)
{
	CHECK(offset <= numBit);
	unsigned mask = (1 << numBit) - 1;
	return ((bits << offset) | (bits >> (numBit - offset))) & mask;
}
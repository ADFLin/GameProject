#include "BitUtility.h"
#include "CompilerConfig.h"
#include "MacroCommon.h"

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
	if (n == 0)
		return 0;
	return 1u << (32 - CountLeadingZeros(n - 1));
}

uint32 FBitUtility::Reverse(uint32 n)
{
#if CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_bitreverse32(n);
#else
	n = ((n & 0xaaaaaaaa) >> 1) | ((n & 0x55555555) << 1);
	n = ((n & 0xcccccccc) >> 2) | ((n & 0x33333333) << 2);
	n = ((n & 0xf0f0f0f0) >> 4) | ((n & 0x0f0f0f0f) << 4);
	n = ((n & 0xff00ff00) >> 8) | ((n & 0x00ff00ff) << 8);
	return (n >> 16) | (n << 16);
#endif
}

uint64 FBitUtility::Reverse(uint64 n)
{
#if CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_bitreverse64(n);
#else
	n = ((n & 0xaaaaaaaaaaaaaaaaULL) >> 1) | ((n & 0x5555555555555555ULL) << 1);
	n = ((n & 0xccccccccccccccccULL) >> 2) | ((n & 0x3333333333333333ULL) << 2);
	n = ((n & 0xf0f0f0f0f0f0f0f0ULL) >> 4) | ((n & 0x0f0f0f0f0f0f0f0fULL) << 4);
	n = ((n & 0xff00ff00ff00ff00ULL) >> 8) | ((n & 0x00ff00ff00ff00ffULL) << 8);
	n = ((n & 0xffff0000ffff0000ULL) >> 16) | ((n & 0x0000ffff0000ffffULL) << 16);
	return (n >> 32) | (n << 32);
#endif
}
static int const gBitIndexMap[9] = 
{ 
	0 , 
	0, 1, 1, 2, 2, 2, 2, 3, 
};

const int gMultipleyDeBruijn32[32] = 
{
	0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
	31, 27, 13, 23, 21, 19, 16, 7, 26, 11, 18, 6, 9, 5, 10, 12
};

const int gMultipleyDeBruijn64[64] =
{
	0, 1, 48, 2, 57, 49, 28, 3, 61, 58, 50, 42, 38, 29, 17, 4,
	62, 55, 59, 36, 53, 51, 43, 22, 45, 39, 33, 30, 24, 18, 12, 5,
	63, 47, 56, 27, 60, 41, 37, 16, 54, 35, 52, 21, 44, 32, 23, 11,
	46, 26, 40, 15, 34, 20, 31, 10, 25, 14, 19, 9, 13, 8, 7, 6
};

int FBitUtility::ToIndex4(unsigned bit)
{
	CHECK((bit & 0xf) == bit);
	CHECK(IsOneBitSet(bit));
#if CPP_COMPILER_MSVC
	unsigned long index;
	_BitScanForward(&index, bit);
	return index;
#elif CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_ctz(bit);
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
#elif CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_ctz(bit);
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
#elif CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_ctz(bit);
#else
	// De Bruijn optimization is overkill for 16-bit, but consistent.
	// Fallback to 32-bit logic or kept simple. simple branching is fine for 16 bit.
	int result = 0;
	if (bit & 0xff00) { result += 8; bit >>= 8; }
	if (bit & 0x00f0) { result += 4; bit >>= 4; }
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
#elif CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_ctz(bit);
#else
	// De Bruijn Multiplication
	return gMultipleyDeBruijn32[((uint32)(bit * 0x077CB531U)) >> 27];
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
#elif CPP_COMPILER_GCC || CPP_COMPILER_CLANG
	return __builtin_ctzll(bit);
#else
	// De Bruijn Multiplication
	return gMultipleyDeBruijn64[((uint64)(bit * 0x03f79d71b4ca8b09ULL)) >> 58];
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
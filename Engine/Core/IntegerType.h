#ifndef IntegerType_h__
#define IntegerType_h__

#include "CompilerConfig.h"

typedef unsigned char byte;
typedef unsigned int  uint;

#if CPP_COMPILER_MSVC

typedef __int64  int64;
typedef __int32  int32;
typedef __int16  int16;
typedef __int8   int8;

typedef unsigned __int64 uint64;
typedef unsigned __int32 uint32;
typedef unsigned __int16 uint16;
typedef unsigned __int8  uint8;

#elif CPP_COMPILER_GCC || CPP_COMPILER_CLANG

#include "stdint.h"

typedef int64_t  int64;
typedef int32_t  int32;
typedef int16_t  int16;
typedef int8_t   int8;

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;
#else

#error "Compiler No Support!"

#endif

constexpr int8 MaxInt8 = 0x7F;
constexpr int8 MinInt8 = int8(0x80u);
constexpr int16 MaxInt16 = 0x7FFF;
constexpr int16 MinInt16 = int16(0x8000u);
constexpr int32 MaxInt32 = 0x7FFFFFFF;
constexpr int32 MinInt32 = int32(0x80000000u);
constexpr int64 MaxInt64 = 0x7FFFFFFFFFFFFFFFLL;
constexpr int64 MinInt64 = int64(0x8000000000000000LLU);
constexpr uint8 MaxUint8 = 0xFFU;
constexpr uint16 MaxUint16 = 0xFFFFU;
constexpr uint32 MaxUint32 = 0xFFFFFFFFU;
constexpr uint64 MaxUint64 = 0xFFFFFFFFFFFFFFFFLLU;

template< uint32 Size >
struct TIntegerTraits {};

template<>
struct TIntegerTraits< 1 >
{
	using Type = int8;
	using UnsignedType = uint8;
};
template<>
struct TIntegerTraits< 2 >
{
	using Type = int16;
	using UnsignedType = uint16;
}; 
template<>
struct TIntegerTraits< 4 >
{
	using Type = int32;
	using UnsignedType = uint32;
}; 
template<>
struct TIntegerTraits< 8 >
{
	using Type = int64;
	using UnsignedType = uint64;
};




#endif // IntegerType_h__

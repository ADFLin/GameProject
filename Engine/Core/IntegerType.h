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
constexpr int8 MinInt8 = 0x80;
constexpr int16 MaxInt16 = 0x7FFF;
constexpr int16 MinInt16 = 0x8000;
constexpr int32 MaxInt32 = 0x7FFFFFFF;
constexpr int32 MinInt32 = 0x80000000;
constexpr int64 MaxInt64 = 0x7FFFFFFFFFFFFFFFLL;
constexpr int64 MinInt64 = 0x8000000000000000LL;
constexpr uint8 MaxUint8 = 0xFFU;
constexpr uint16 MaxUint16 = 0xFFFFU;
constexpr uint32 MaxUint32 = 0xFFFFFFFFU;
constexpr uint64 MaxUint64 = 0xFFFFFFFFFFFFFFFFLLU;

#endif // IntegerType_h__

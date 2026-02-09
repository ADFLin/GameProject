#pragma once
#ifndef MacroCommon_H_1C050617_9CB9_4AEF_A36E_73BF931E68B9
#define MacroCommon_H_1C050617_9CB9_4AEF_A36E_73BF931E68B9

#include "CompilerConfig.h"
#include "Core/IntegerType.h"
#include <cassert>
#include <cstddef>


#ifndef BIT
#define BIT( n ) ( 1 << ( n ) )
#endif 
#ifndef BIT64
#define BIT64( n ) ( 1ULL << ( n ) )
#endif 

#define INDEX_NONE (-1)

template< class T, size_t N >
constexpr size_t array_size(T(&ar)[N]) { return N; }
#define ARRAY_SIZE( var ) array_size( var )

#define MAKE_STRING_INNER(A) #A
#define MAKE_STRING(A) MAKE_STRING_INNER(A)

#define COMMA_SEPARATED(first, second, ...) first, second, ##__VA_ARGS__

#define MARCO_NAME_COMBINE_2_INNER(s1, s2) s1##s2
#define MARCO_NAME_COMBINE_2(s1, s2) MARCO_NAME_COMBINE_2_INNER(s1, s2)

#define MARCO_NAME_COMBINE_3_INNER(s1, s2 ,s3) s1##s2##s3
#define MARCO_NAME_COMBINE_3(s1, s2 , s3) MARCO_NAME_COMBINE_3_INNER(s1, s2, s3)

#ifdef __COUNTER__
#define ANONYMOUS_VARIABLE(str) MARCO_NAME_COMBINE_2(str, __COUNTER__)
#else
#define ANONYMOUS_VARIABLE(str) MARCO_NAME_COMBINE_2(str, __LINE__)
#endif 

#define MAKE_VERSION( a , b , c ) ( uint32( uint8(a)<< 24 ) | uint32( uint8(b)<< 16 ) | uint32(uint16(c)) )

#define MAKE_MAGIC_ID(a,b,c,d) ( (uint32(d)<<24) | (uint32(c)<<16) | (uint32(b)<<8) | (a) )

#if _DEBUG
#define CHECK( EXPR ) assert( EXPR )
#define NEVER_REACH( STR ) assert( !STR )
#else
#define CHECK( EXPR ) ASSUME_BUILTIN( EXPR )
#define NEVER_REACH( STR ) ASSUME_BUILTIN( 0 )
#endif

#define FAIL_LOG_MSG_GENERATE( CODE , FILE , LINE )\
	LogWarning(1, "Init Fail => File: %s (%s) : %s ", FILE, #LINE, #CODE)

#define VERIFY_FAILCODE_INNEAR( CODE , FAILCODE , FILE , LINE  )\
	{ if( !( CODE ) ) { FAIL_LOG_MSG_GENERATE( CODE, FILE , LINE ); FAILCODE; } }

#define VERIFY_RETURN_FALSE( CODE ) VERIFY_FAILCODE_INNEAR( CODE, return false; ,  __FILE__ , __LINE__ )
#define VERIFY_FAILCODE( CODE , FAILCODE ) VERIFY_FAILCODE_INNEAR( CODE, FAILCODE , __FILE__ , __LINE__ )

#define CODE_STRING_INNER( CODE_TEXT ) R###CODE_TEXT
#define CODE_STRING( CODE_TEXT ) CODE_STRING_INNER( CODE_STRING_(##CODE_TEXT)CODE_STRING_ )

#define SUPPORT_ENUM_FLAGS_OPERATION(TYPE)\
	FORCEINLINE TYPE operator | (TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return TYPE(T(a) | T(b)); }\
	FORCEINLINE TYPE operator & (TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return TYPE(T(a) & T(b)); }\
	FORCEINLINE TYPE operator ^ (TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return TYPE(T(a) ^ T(b)); }\
	FORCEINLINE TYPE& operator |= (TYPE& a, TYPE b) { using T = std::underlying_type_t<TYPE>; a = a | b; return a; }\
	FORCEINLINE TYPE& operator &= (TYPE& a, TYPE b) { using T = std::underlying_type_t<TYPE>; a = a & b; return a; }\
	FORCEINLINE TYPE& operator ^= (TYPE& a, TYPE b) { using T = std::underlying_type_t<TYPE>; a = a ^ b; return a; }\
	FORCEINLINE bool HaveBits(TYPE a, TYPE b) { using T = std::underlying_type_t<TYPE>; return !!(T(a) & T(b)); }


#if CPP_COMPILER_MSVC
#define PRAGMA_ENABLE_OPTIMIZE_ACTUAL __pragma(optimize( "", on ))
#define PRAGMA_DISABLE_OPTIMIZE_ACTUAL __pragma(optimize( "", off ))
#else
#define PRAGMA_ENABLE_OPTIMIZE_ACTUAL
#define PRAGMA_DISABLE_OPTIMIZE_ACTUAL
#endif

#if _DEBUG
#define PRAGMA_ENABLE_OPTIMIZE PRAGMA_ENABLE_OPTIMIZE_ACTUAL
#define PRAGMA_DISABLE_OPTIMIZE PRAGMA_DISABLE_OPTIMIZE_ACTUAL
#else
#define PRAGMA_ENABLE_OPTIMIZE PRAGMA_ENABLE_OPTIMIZE_ACTUAL
#define PRAGMA_DISABLE_OPTIMIZE PRAGMA_DISABLE_OPTIMIZE_ACTUAL
#endif

#endif // MacroCommon_H_1C050617_9CB9_4AEF_A36E_73BF931E68B9

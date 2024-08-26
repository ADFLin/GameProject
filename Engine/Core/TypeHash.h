#pragma once
#ifndef TypeHash_h__
#define TypeHash_h__

#include "Core/IntegerType.h"
#include "Core/CRC.h"
#include "CString.h"

#include <type_traits>


FORCEINLINE uint32 HashValue(char const* str)
{
	return FCString::StrHash(str);
}

FORCEINLINE uint32 HashValue(char const* str, int num)
{
	return FCString::StrHash(str, num);
}

FORCEINLINE uint32 HashValue(void const* pData, int num)
{
	return FCRC::Value32((uint8 const*)pData, num);
}

template< class T >
FORCEINLINE uint32 HashValue(T const& v)
{
	std::hash<T> hasher;
	return (uint32)hasher(v);
}

FORCEINLINE uint32 CombineHash(uint32 a, uint32 b)
{
	return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
}

template <class T>
FORCEINLINE uint32 HashCombine(uint32 seed, T const& v)
{
	return CombineHash(seed, HashValue(v));
}

template< typename T, typename ...TArgs >
FORCEINLINE uint32 HashCombine(uint32 seed, T const& v, TArgs const& ...args)
{
	seed = HashCombine(seed, v);
	return HashCombine(seed, args...);
}

template< typename T>
FORCEINLINE uint32 HashValues(T const& v)
{
	return HashValue(v);
}

template< typename T, typename ...TArgs >
FORCEINLINE uint32 HashValues(T const& v, TArgs const& ...args)
{
	return HashCombine(HashValue(v), args...);
}

struct MemberFuncHasher
{
	template< class T >
	uint32 operator()(T const& value) const noexcept { return value.getTypeHash(); }
};


#define DEFINE_TYPE_HASH_TO_STD( TYPE , FUNC_CODE )\
		namespace std {\
			template<> struct hash<TYPE>\
			{\
				static auto HashFunc FUNC_CODE\
				size_t operator() (TYPE const& value) const { return HashFunc(value); }\
			};\
		}

#define EXPORT_MEMBER_HASH_TO_STD( TYPE )\
	   DEFINE_TYPE_HASH_TO_STD( TYPE , ( TYPE const& value ){ return value.getTypeHash(); })

#endif // TypeHash_h__
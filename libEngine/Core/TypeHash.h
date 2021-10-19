#pragma once
#ifndef TypeHash_h__
#define TypeHash_h__

#include "Core/IntegerType.h"
#include "Core/CRC.h"
inline uint32 HashValue(char const* str)
{
	int32 hash = 5381;
	int c;

	while( c = *str++ )
	{
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

inline uint32 HashValue(char const* str, int num)
{
	uint32 hash = 5381;

	while( num )
	{
		uint32 c = (uint32)*str++;
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		--num;
	}
	return hash;
}

inline uint32 HashValue(void const* pData, int num)
{
	return FCRC::Value32((uint8 const*)pData, num);
}

template< class T >
inline uint32 HashValue(T const& v)
{
	std::hash<T> hasher;
	return hasher(v);
}

template <class T>
inline void HashCombine(uint32& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline uint32 CombineHash(uint32 a, uint32 b)
{
	a ^= b + 0x9e3779b9 + (a << 6) + (a >> 2);
	return a;
}

struct MemberFuncHasher
{
	template< class T >
	uint32 operator()(T const& value) const noexcept { return value.getTypeHash(); }
};

#define EXPORT_MEMBER_HASH_TO_STD( TYPE )\
	namespace std { template<> struct hash<TYPE> { size_t operator()(TYPE const& value) const { return value.getTypeHash(); } }; }

#endif // TypeHash_h__
#pragma once
#ifndef TypeHash_h__
#define TypeHash_h__

#include "Core/IntegerType.h"

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

template< class T >
inline uint32 HashValue(T const& v)
{
	std::hash<T> hasher;
	return hasher(v);
}


struct MemberFunHasher
{
	template< class T >
	uint32 operator()(T const& value) const noexcept { return value.getHash(); }
};

template <class T>
inline void HashCombine(uint32& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

#endif // TypeHash_h__
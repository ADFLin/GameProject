#pragma once
#ifndef TypeHash_h__
#define TypeHash_h__

#include "Core/IntegerType.h"

namespace Type
{
	inline uint32 Hash(char const* str)
	{
		int32 hash = 5381;
		int c;

		while( c = *str++ )
		{
			hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
		}
		return hash;
	}

	inline uint32 Hash(char const* str , int num )
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
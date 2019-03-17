#pragma once
#ifndef FNV1a_H_D05A267D_A207_495F_B791_A7B39212C5AF
#define FNV1a_H_D05A267D_A207_495F_B791_A7B39212C5AF

#include "Core/IntegerType.h"

struct Uint128
{
	uint64 low;
	uint64 high;
};

class FNV1a
{
public:
	template< class T >
	struct Table{};

	template<>
	struct Table< uint32 >
	{
		static uint32 constexpr Prime = 16777619u;
		static uint32 constexpr OffsetBias = 0x811c9dc5u;
	};

	template<>
	struct Table< uint64 >
	{
		static uint64 constexpr Prime = 1099511628211LLU;
		static uint64 constexpr OffsetBias = 0xcbf29ce484222325LLU;
	};

	template<>
	struct Table< Uint128 >
	{
		static Uint128 constexpr Prime = { 0x000000000000013bLLU , 0x0000000001000000LLU };
		static Uint128 constexpr OffsetBias = { 0x62B821756295C58DLLU , 0x6C62272E07BB0142LLU  };
	};

	template< class T >
	static T MakeStringHash(char const* pStr, T offsetBias = Table<T>::OffsetBias )
	{
		T result = offsetBias;
		while( *pStr )
		{
			result ^= uint8(*pStr);
			result *= Table<T>::Prime;
			++pStr;
		}
		return result;
	}

	template< class T >
	static T MakeHash(uint8 const* pValue, int num , T offsetBias = Table<T>::OffsetBias )
	{
		T result = offsetBias;
		for( ; num ; --num )
		{
			result ^= uint8(*pValue);
			result *= Table<T>::Prime;
			++pValue;
		}
		return result;
	}
};


#endif // FNV1a_H_D05A267D_A207_495F_B791_A7B39212C5AF

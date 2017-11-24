#pragma once
#ifndef FNV1a_H_D05A267D_A207_495F_B791_A7B39212C5AF
#define FNV1a_H_D05A267D_A207_495F_B791_A7B39212C5AF

#include "Core/IntegerType.h"

class FNV1a
{
public:
	template< class T >
	struct Table{};

	template<>
	struct Table< uint32 >
	{
		static uint32 const Prime = 16777619u;
		static uint32 const OffsetBias = 0x811c9dc5u;
	};

	template<>
	struct Table< uint64 >
	{
		static uint64 const Prime = 1099511628211LLU;
		static uint64 const OffsetBias = 0xcbf29ce484222325LLU;
	};

	template< class T >
	static T MakeStringHash(char const* pStr, T offsetBias = Table<T>::OffsetBias )
	{
		uint32 result = offsetBias;
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
		uint32 result = offsetBias;
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

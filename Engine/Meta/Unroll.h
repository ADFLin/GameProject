#pragma once
#ifndef Unroll_H_BBC444C6_E3B5_44B5_AE82_8817E975BA95
#define Unroll_H_BBC444C6_E3B5_44B5_AE82_8817E975BA95

#include "IndexList.h"
#include "CompilerConfig.h"

#define UNROLL_USE_FOLD_EXPRESSIONS 0

template< size_t N, typename TFunc >
FORCEINLINE void Unroll(TFunc&& func)
{
	Unroll(func, TIndexRange<0, N>());
}

template< size_t N, typename TFunc >
FORCEINLINE bool UnrollAny(TFunc&& func)
{
	return UnrollAny(func, TIndexRange<0, N>());
}

template< size_t N, typename TFunc >
FORCEINLINE bool UnrollAll(TFunc&& func)
{
	return UnrollAll(func, TIndexRange<0, N>());
}

#if UNROLL_USE_FOLD_EXPRESSIONS

template< typename TFunc, size_t ...Is >
FORCEINLINE void Unroll(TFunc&& func, TIndexList<Is...>)
{
	(..., func(Is));
}

template< typename TFunc, size_t ...Is >
FORCEINLINE bool UnrollAny(TFunc&& func, TIndexList<Is...>)
{
	return (... || func(Is));
}

template< typename TFunc, size_t ...Is >
FORCEINLINE bool UnrollAll(TFunc&& func, TIndexList<Is...>)
{
	return (... && func(Is));
}

#else

template< typename TFunc, size_t Index>
FORCEINLINE void Unroll(TFunc&& func, TIndexList<Index>)
{
	func(Index);
}

template< typename TFunc, size_t Index, size_t ...Is >
FORCEINLINE void Unroll(TFunc&& func, TIndexList<Index, Is...>)
{
	func(Index);
	Unroll(func, TIndexList<Is...>());
}

template< typename TFunc, size_t Index>
FORCEINLINE bool UnrollAny(TFunc&& func, TIndexList<Index>)
{
	return func(Index);
}

template< typename TFunc, size_t Index, size_t ...Is >
FORCEINLINE bool UnrollAny(TFunc&& func, TIndexList<Index, Is...>)
{
	if (func(Index))
		return true;
	return UnrollAny(func, TIndexList<Is...>());
}

template< typename TFunc, size_t Index>
FORCEINLINE bool UnrollAll(TFunc&& func, TIndexList<Index>)
{
	return func(Index);
}

template< typename TFunc, size_t Index, size_t ...Is >
FORCEINLINE bool UnrollAll(TFunc&& func, TIndexList<Index, Is...>)
{
	if (!func(Index))
		return false;
	return UnrollAll(func, TIndexList<Is...>());
}

#endif

#endif // Unroll_H_BBC444C6_E3B5_44B5_AE82_8817E975BA95

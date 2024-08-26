#pragma once
#ifndef ArrayView_H_90099AAD_213F_44A6_8A18_900243D39026
#define ArrayView_H_90099AAD_213F_44A6_8A18_900243D39026

#include "DataStructure/Array.h"

#include <type_traits>
#include <vector>

template< class T >
class TArrayView
{
public:
	using BaseType = std::remove_const_t<T>;

	TArrayView()
		:mData(0), mNum(0)
	{
	}

	TArrayView(T* inData, size_t inNum)
		:mData(inData), mNum(inNum)
	{
	}

	template< size_t N >
	TArrayView(T(&inData)[N])
		: mData(inData), mNum(N)
	{
	}

	template< typename A >
	TArrayView(TArray<BaseType, A> const& inArray)
		: mData((T*)inArray.data()), mNum(inArray.size())
	{

	}

	TArrayView(std::initializer_list<T> inList)
		:mData((T*)inList.begin()), mNum(inList.size())
	{
	}

	int findIndex(T const& value) const
	{
		for (int index = 0; index < mNum; ++index)
		{
			if (mData[index] == value)
				return index;
		}
		return INDEX_NONE;
	}

	template< typename TFunc >
	int findIndexPred(TFunc&& func) const
	{
		for (int index = 0; index < mNum; ++index)
		{
			if (func(mData[index]))
				return index;
		}
		return INDEX_NONE;
	}

	bool isValidIndex(int index) const
	{
		return 0 <= index && (size_t)index < mNum;
	}

	operator T* () { return mData; }
	operator T const* () const { return mData; }


	using iterator = T*;
	using const_iterator = T const*;

	iterator begin() { return mData; }
	iterator end()   { return mData + mNum; }

	const_iterator begin() const { return mData; }
	const_iterator end() const { return mData + mNum; }

	//T&        operator[](size_t idx) { assert(idx < mNum); return mData[idx]; }
	//T const&  operator[](size_t idx) const { assert(idx < mNum); return mData[idx]; }

	T*     data() { return mData; }
	size_t size() const { return mNum; }

	T const* data() const { return mData; }
protected:
	T*     mData;
	size_t mNum;
};

template< class T , int N >
TArrayView<T> MakeView(T(&data)[N]) { return TArrayView<T>(data); }

template< class T, int N >
TArrayView<T const> MakeView(T const(&data)[N]) { return TArrayView<T const>(data); }

template< class T >
TArrayView<T> MakeView(TArray< T >& v) { return TArrayView<T>(v.data(), v.size()); }
template< class T >
TArrayView<T> MakeView(std::vector< T >& v) { return TArrayView<T>(v.data(), v.size()); }

template< class T >
TArrayView<T const> MakeConstView(std::vector< T >& v) { return TArrayView<T const>(v.data(), v.size()); }
template< class T >
TArrayView<T const> MakeConstView(std::vector< T > const& v) { return TArrayView<T const>(v.data(), v.size()); }


template< class T >
TArrayView<T const> MakeConstView(TArray< T >& v) { return TArrayView<T const>(v.data(), v.size()); }
template< class T >
TArrayView<T const> MakeConstView(TArray< T > const& v) { return TArrayView<T const>(v.data(), v.size()); }

#define ARRAY_VIEW_REAONLY_DATA( TYPE , ... ) \
	[](){ static TYPE const data[] = { __VA_ARGS__ }; return TArrayView< TYPE const >(data); }()


#endif // ArrayView_H_90099AAD_213F_44A6_8A18_900243D39026
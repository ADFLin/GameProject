#pragma once
#ifndef ArrayView_H_90099AAD_213F_44A6_8A18_900243D39026
#define ArrayView_H_90099AAD_213F_44A6_8A18_900243D39026

#include <vector>
#include "DataStructure/Array.h"

template< class T >
class TArrayView
{
public:
	TArrayView()
		:mData(0), mNum(0)
	{
	}

	TArrayView(T* inData, int inNum)
		:mData(inData), mNum(inNum)
	{
	}

	template< int N >
	TArrayView(T(&inData)[N])
		: mData(inData), mNum(N)
	{
	}

	TArrayView(std::initializer_list<T> const& inList)
		:mData(inList.begin()), mNum(inList.size())
	{
	}



	operator T* () { return mData; }
	operator T const* () const { return mData; }


	typedef T* iterator;
	iterator begin() { return mData; }
	iterator end()   { return mData + mNum; }

	typedef T const* const_iterator;
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
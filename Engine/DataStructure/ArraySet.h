#pragma once
#ifndef ArraySet_H_2E6300B5_BD63_4817_AEB4_C5EE7D3A47D5
#define ArraySet_H_2E6300B5_BD63_4817_AEB4_C5EE7D3A47D5

#include "Array.h"
#include <algorithm>


template< class T , class CmpFunc = std::less<> >
class TArraySet
{
	typedef TArray< T > ImplType;
public:
	typedef typename ImplType::iterator       iterator;
	typedef typename ImplType::const_iterator const_iterator;

	iterator begin(){ return mImpl.begin(); }
	iterator end(){ return mImpl.end(); }
	const_iterator begin() const { return mImpl.begin(); }
	const_iterator end() const { return mImpl.end(); }

	void clear(){ mImpl.clear(); }
	bool empty() const { return mImpl.empty(); }
	int  findIndex( T const& value )
	{
		iterator iter = find( value );
		if ( iter == mImpl.end() )
			return INDEX_NONE;
		return iter - mImpl.begin();
	}
	iterator find( T const& value )
	{
		ImplType::iterator iter = std::lower_bound( mImpl.begin() , mImpl.end() , value ,  CmpFunc() );
		if ( iter != mImpl.end() )
		{
			if ( value == *iter )
				return iter;
		}
		return mImpl.end();
	}

	void insert( T const& value )
	{
		ImplType::iterator iter = std::lower_bound( mImpl.begin() , mImpl.end() , value ,  CmpFunc() );
		if ( iter != mImpl.end() )
		{
			if ( value == *iter )
				return;
		}
		mImpl.insert( iter , value );
	}
	int  size(){ return mImpl.size(); }
	T&   operator[]( int idx ){ return mImpl[idx]; }

private:
	TArray< T > mImpl;

};

#endif // ArraySet_H_2E6300B5_BD63_4817_AEB4_C5EE7D3A47D5

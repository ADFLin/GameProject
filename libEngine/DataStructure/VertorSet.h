#ifndef VertorSet_h__
#define VertorSet_h__

#include <vector>
#include <algorithm>

template< class T , class CmpFun = std::less< T >  >
class TVectorSet
{
	typedef std::vector< T > ImplType;
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
			return -1;
		return iter - mImpl.begin();
	}
	iterator find( T const& value )
	{
		ImplType::iterator iter = std::lower_bound( mImpl.begin() , mImpl.end() , value ,  CmpFun() );
		if ( iter != mImpl.end() )
		{
			if ( value == *iter )
				return iter;
		}
		return mImpl.end();
	}

	void insert( T const& value )
	{
		ImplType::iterator iter = std::lower_bound( mImpl.begin() , mImpl.end() , value ,  CmpFun() );
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
	std::vector< T > mImpl;

};

#endif // VertorSet_h__

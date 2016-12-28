#pragma once
#ifndef StdUtility_h__
#define StdUtility_h__

template< class T >
bool AddUnique(std::vector< T >& v, T const& val)
{
	for( int i = 0; i < v.size(); ++i )
	{
		if( v[i] == val )
			return false;
	}
	v.push_back(val);
	return true;
}

template < class T >
bool IsValueUnique(std::vector< T > const& v)
{
	int size = v.size();

	for( int i = 0; i < size; ++i )
	{
		for( int j = i + 1; j < size; ++j )
		{
			if( v[i] == v[j] )
				return false;
		}
	}
	return true;
}

template< class T >
void MakeValueUnique(std::vector< T >& v, int idxStart = 0)
{
	int idxLast = v.size() - 1;
	for( int i = idxStart; i <= idxLast; ++i )
	{
		for( int j = 0; j < i; ++j )
		{
			if( v[i] == v[j] )
			{
				if( i != idxLast )
				{
					std::swap(v[i], v[idxLast]);
					--i;
				}
				--idxLast;
			}
		}
	}
	if( idxLast + 1 != v.size() )
	{
		v.resize(idxLast + 1);
	}

	assert(IsValueUnique(v));
}

template< class T >
class TIterator
{
public:
	TIterator(T& c) :cur(c.begin()), end(c.end()) {}
	operator bool()  const { return cur != end; }
	auto& operator *() { return *cur; }
	auto operator->() { return cur.operator ->(); }
	TIterator& operator ++() { ++cur; return *this; }
	TIterator  operator ++(int) { TIterator temp = *this;  ++cur; return temp; }
private:
	typedef typename T::iterator IterImpl;
	typename IterImpl cur;
	typename IterImpl end;
};

template< class T >
class TConstIterator
{
public:
	TConstIterator(T const& c) :cur(c.begin()), end(c.end()) {}
	operator bool()  const { return cur != end; }
	auto const& operator *() { return *cur; }
	auto operator->() { return cur.operator ->();  }
	TConstIterator& operator ++() { ++cur; return *this; }
	TConstIterator  operator ++(int) { TIterator temp = *this;  ++cur; return temp; }
private:
	typedef typename T::const_iterator IterImpl;
	typename IterImpl cur;
	typename IterImpl end;
};

template< class T >
auto MakeIterator(T& c) { return TIterator<T>(c); }
template< class T >
auto MakeIterator(T const& c) { return TConstIterator<T>(c); }

#endif // StdUtility_h__
#pragma once
#ifndef StdUtility_h__
#define StdUtility_h__

#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>

template< typename T, typename A >
void RemoveValueChecked(std::vector< T, A >& v, T const& val)
{
	auto iter = std::find(v.begin(), v.end(), val);
	CHECK(iter != v.end());
	v.erase(iter);
}

template< typename T, typename A >
bool RemoveValue(std::vector< T , A >& v, T const& val)
{
	auto iter = std::find(v.begin(), v.end(), val);

	if( iter == v.end() )
		return false;

	v.erase(iter);
	return true;
}

template< typename K, typename V, typename C, typename A >
void RemoveValueChecked(std::map< K, V, C, A >& v, K const& val)
{
	auto iter = v.find(val);
	CHECK(iter != v.end());
	v.erase(iter);
}

template< typename K, typename V, typename C, typename A >
bool RemoveValue(std::map< K, V, C, A >& v, K const& val)
{
	auto iter = v.find(val);

	if (iter == v.end())
		return false;

	v.erase(iter);
	return true;
}


template< typename K, typename V, typename H, typename KE, typename A >
bool RemoveValue(std::unordered_map< K , V , H , KE , A >& v, K const& val)
{
	auto iter = v.find(val);

	if (iter == v.end())
		return false;

	v.erase(iter);
	return true;
}


template< typename T, typename A >
void RemoveIndexSwap(std::vector< T, A >& v, int index)
{
	CHECK(IsValidIndex(v, index));
	if (v.size() - 1 != index)
	{
		std::swap(v[index], v.back());
	}
	v.pop_back();
}

template< typename T, typename A >
bool AddUniqueValue(std::vector< T , A >& v, T const& val)
{
	for( int i = 0; i < v.size(); ++i )
	{
		if( v[i] == val )
			return false;
	}
	v.push_back(val);
	return true;
}

template < typename T, typename A >
bool IsValueUnique(std::vector< T , A > const& v)
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

template< typename T, typename A  >
void MakeValueUnique(std::vector< T , A >& v, int idxStart = 0)
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


template< typename T, typename A >
FORCEINLINE bool IsValidIndex(std::vector<T, A> const& v, int index)
{
	return 0 <= index && index < v.size();
}

template< typename T, typename A, typename TFunc >
FORCEINLINE bool RemovePred(std::vector<T, A>& v, TFunc func)
{
	auto iter = v.begin();
	while (iter != v.end())
	{
		if (func(*iter))
		{
			iter = v.erase(iter);
			return true;
		}
		else
		{
			++iter;
		}
	}
	return false;
}

template< typename T, typename A , typename TFunc >
FORCEINLINE int RemoveAllPred(std::vector<T, A>& v, TFunc func )
{
	int result = 0;
	auto iter = v.begin();
	while (iter != v.end())
	{
		if (func(*iter) )
		{
			iter = v.erase(iter);
			++result;
		}
		else
		{
			++iter;
		}
	}
	return result;
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

template< typename It , typename V >
int FindIndex(It start, It end , V const& value )
{
	It cur = start;
	while (cur != end)
	{
		if (*cur == value)
			return cur - start;

		++cur;
	}

	return INDEX_NONE;
}

template< typename T, typename A, typename TFunc >
FORCEINLINE int FindIndexPred(std::vector<T, A>& v, TFunc func)
{
	auto start = std::begin(v);
	auto end = std::end(v);
	auto cur = start;
	while (cur != end)
	{
		if (func(*cur))
			return cur - start;
		++cur;
	}
	return INDEX_NONE;
}

#endif // StdUtility_h__
#pragma once
#ifndef FixedArray_H_C315539D_E69B_425C_BD11_1AE933764C8D
#define FixedArray_H_C315539D_E69B_425C_BD11_1AE933764C8D

#if 1

#include "Array.h"

template< class T, size_t N >
using TFixedArray = TArray< T, TFixedAllocator<N> >;

#else

#include "TypeConstruct.h"

template< class T, size_t N >
class TFixedArray
{
public:
	typedef T*        iterator;
	typedef T const*  const_iterator;
	typedef T         value_type;
	typedef T&        reference;
	typedef T const & const_refernece;
	typedef T*        pointer;
	typedef T const*  const_pointer;

	TFixedArray()
	{  
		mNext = mEle; 
	}

	TFixedArray( size_t num , T val = T() )
	{
		TypeDataHelper::Construct((T*)mEle, num, val);
		mNext = mEle + num;
	}
	~TFixedArray(){  clear();  }

	bool     empty()    const { return mEle == mNext; }
	size_t   size()     const { return mNext - mEle; }
	size_t   max_size() const { return N; }
	void     clear(){  eraseToEnd( begin() );  }

	const_iterator begin() const { return (T*)mEle; }
	iterator       begin()       { return (T*)mEle; }
	const_iterator end()   const { return (T*)mNext; }
	iterator       end()         { return (T*)mNext; }

	void    resize( size_t num );
	void    resize( size_t num , value_type const& value );

	void push_back( T const& val ){  assert( mNext != mEle + N ); TypeDataHelper::Construct((T*)mNext , val ); ++mNext; }
	void pop_back() { assert(size() != 0); --mNext; TypeDataHelper::Destruct((T*)mNext); }

	const_refernece front() const { return castType( mEle[ 0 ] ); }
	reference       front()       { return castType( mEle[ 0 ] ); }
	const_refernece back() const { return castType( *(mNext - 1) ); }
	reference       back()       { return castType( *(mNext - 1) ); }

	const_refernece at( size_t idx ) const  { checkRange( begin() + idx ) ; return castType( mEle[ idx ] ); }
	reference       at( size_t idx )        { checkRange( begin() + idx ) ; return castType( mEle[ idx ] ); }

	const_refernece operator[]( size_t idx ) const  { return castType( mEle[ idx ] ); }
	reference       operator[]( size_t idx )        { return castType( mEle[ idx ] ); }

	iterator erase( iterator iter )
	{
		checkRange( iter );
		TypeDataHelper::Destruct(iter);
		
		moveToEnd( iter , iter + 1 );
		return iter;
	}

	iterator erase( iterator from , iterator to )
	{
		checkRange( from );
		checkRange( to );
		assert( to > from );
		TypeDataHelper::Destruct(from, to - from);
		moveToEnd( from , to );
		return from;
	}	

#if 0
	operator const_pointer () const {  return getHead();  } 
	operator pointer       ()       {  return getHead();  } 
#endif

private:
	T*        getHead()       { return reinterpret_cast< T* >( mEle[0] ); }
	T const*  getHead() const { return reinterpret_cast< T const* >( mEle[0] ); }
	void      moveToEnd( iterator where , iterator src )
	{
		size_t num = (T*)mNext - src;
		if( num )
		{
			TypeDataHelper::Move(where, num, src);
		}
		mNext = reinterpret_cast< Storage*>( where + num );
	}

	void  eraseToEnd( iterator is )
	{
		TypeDataHelper::Destruct(is, end() - is);
		mNext = reinterpret_cast< Storage*>( is );
	}

	struct Storage
	{
		char s[sizeof( T )];
	};
	Storage  mEle[ N ];
	Storage* mNext;

	static T&       castType( Storage& s )      { return *((T*)&s ); }
	static T const& castType( Storage const& s ){ return *((T const*)&s ); }
	void   checkRange( const_iterator it ) const { assert( begin() <= it && it < end() ); }
};


template< class T , size_t N >
void  TFixedArray< T , N >::resize( size_t num )
{ 
	CHECK( num < N  ); 
	if ( num < size() )
	{
		erase( begin() + num , end() );
	}
	else
	{
		TypeDataHelper::Construct(mNext, num);
		mNext += num;
	}
}


template< class T , size_t N >
void  TFixedArray< T , N >::resize( size_t num , value_type const& value )
{
	CHECK( num < N  );
	if ( num < size() )
	{
		erase( begin() + num , end() );
	}
	else
	{
		TypeDataHelper::Construct(mNext, num, value );
		mNext += num;
	}
}
#endif

#endif // FixedArray_H_C315539D_E69B_425C_BD11_1AE933764C8D

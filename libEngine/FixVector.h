#ifndef FixVector_h__
#define FixVector_h__

#include <cassert>


template< class T , size_t N >
class FixVector
{
public:
	typedef T*        iterator;
	typedef T const*  const_iterator;
	typedef T         value_type;
	typedef T&        reference;
	typedef T const & const_refernece;
	typedef T*        pointer;
	typedef T const*  const_pointer;

	FixVector()
	{  
		mNext = mEle; 
	}

	FixVector( size_t num , T val = T() )
	{
		mNext = mEle;
		Storage* pEnd = mEle + num;
		for( ; mNext != pEnd ; ++mNext )
			_contruct( mNext , val );
	}
	~FixVector(){  clear();  }

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

	void push_back( T const& val ){  assert( mNext != mEle + N ); _contruct( mNext , val ); ++mNext; }
	void pop_back(){  assert( size() != 0 ); --mNext; _castType( *mNext ).~T();  }

	const_refernece front() const { return _castType( mEle[ 0 ] ); }
	reference       front()       { return _castType( mEle[ 0 ] ); }
	const_refernece back() const { return _castType( *(mNext - 1) ); }
	reference       back()       { return _castType( *(mNext - 1) ); }

	const_refernece at( size_t idx ) const  { _checkRange( begin() + idx ) ; return _castType( mEle[ idx ] ); }
	reference       at( size_t idx )        { _checkRange( begin() + idx ) ; return _castType( mEle[ idx ] ); }

	const_refernece operator[]( size_t idx ) const  { return _castType( mEle[ idx ] ); }
	reference       operator[]( size_t idx )        { return _castType( mEle[ idx ] ); }

	iterator erase( iterator iter )
	{
		_checkRange( iter );
		iter->~T();
		_moveToEnd( iter , iter + 1 );
		return iter;
	}

	iterator erase( iterator from , iterator to )
	{
		_checkRange( from );
		_checkRange( to );
		assert( to > from );
		_destroy( from , to );
		_moveToEnd( from ,  to );
		return from;
	}	

#if 0
	operator const_pointer () const {  return _getHead();  } 
	operator pointer       ()       {  return _getHead();  } 
#endif

private:
	T*        _getHead()       { return reinterpret_cast< T* >( mEle[0] ); }
	T const*  _getHead() const { return reinterpret_cast< T const* >( mEle[0] ); }
	void _moveToEnd( iterator from , iterator to  )
	{
		while( to != (T*)mNext )
		{
			_contruct( from , *to );
			to->~T();

			++from;
			++to;
		}
		mNext = reinterpret_cast< Storage*>( from );
	}

	void  eraseToEnd( iterator is )
	{
		_destroy( is , end() );
		mNext = reinterpret_cast< Storage*>( is );
	}

	void  _destroy( iterator is , iterator ie )
	{
		for (; is != ie ; ++is )
			is->~T();
	}

	void _contruct( void* ptr )
	{
		::new ( ptr ) T;  
	}
	void _contruct( void* ptr , T const& val )
	{
		::new ( ptr ) T( val );  
	}
	struct Storage
	{
		char s[sizeof( T )];
	};
	Storage  mEle[ N ];
	Storage* mNext;

	static T&       _castType( Storage& s )      { return *((T*)&s ); }
	static T const& _castType( Storage const& s ){ return *((T const*)&s ); }
	void   _checkRange( const_iterator it ) const { assert( begin() <= it && it < end() ); }
};


template< class T , size_t N >
void  FixVector< T , N >::resize( size_t num )
{ 
	assert( num < N  ); 
	if ( num < size() )
	{
		erase( begin() + num , end() );
	}
	else
	{
		Storage* pEnd = mEle + num;
		for( ; mNext != pEnd ; ++mNext )
			_contruct( mNext );
	}
}


template< class T , size_t N >
void  FixVector< T , N >::resize( size_t num , value_type const& value )
{
	assert( num < N  ); 
	if ( num < size() )
	{
		erase( begin() + num , end() );
	}
	else
	{
		Storage* pEnd = mEle + num;
		for( ; mNext != pEnd ; ++mNext )
			_contruct( mNext , value );
	}
}

#endif // FixVector_h__

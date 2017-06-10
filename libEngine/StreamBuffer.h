#ifndef StreamBuffer_h__
#define StreamBuffer_h__

#include "MetaBase.h"

#include <exception>
#include <memory>
#include <cassert>
#include <vector>

class CheckPolicy
{
	static bool checkFill( char* data , size_t max , size_t cur ,  size_t num );
	static bool checkTake( char* data , size_t max , size_t cur ,  size_t num );
};

class BufferException : public std::exception
{
public:
	BufferException( char const* msg ):std::exception( msg ){}
};

struct AssertCheckPolicy 
{
	static bool checkFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		assert( cur + num <= max );
		return true;
	}
	static bool checkTake( char* data , size_t max , size_t cur ,  size_t num )
	{
		assert(cur + num <= max);
		return true;
	}
};

struct  FailCheckPolicy
{
	static bool checkFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		if ( cur + num > max )
			return false;
		return true;
	}
	static bool checkTake( char* data , size_t max , size_t cur ,  size_t num  )
	{
		if ( cur + num > max )
			return false;
		return true;
	}
};

struct  ThrowCheckPolicy
{
	static bool checkFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		if ( cur + num > max )
			throw BufferException( "Overflow" );
		return true;
	}
	static bool checkTake( char* data , size_t max , size_t cur ,  size_t num )
	{
		if ( cur + num > max )
			throw BufferException( "No Enough Data" );
		return true;
	}
};


template< class CheckPolicy = AssertCheckPolicy >
class TStreamBuffer
{
	typedef CheckPolicy CP;
public:
	TStreamBuffer(){  setEmpty();  }
	TStreamBuffer( char* data , int maxSize )
	{
		mData = data;
		mMaxSize = maxSize;
		clear();
	}

	char*   getData()          const { return mData; }

	size_t  getFillSize()      const { return mFillSize; }
	size_t  getUseSize()       const { return mUseSize;  }
	size_t  getMaxSize()       const { return mMaxSize; }
	size_t  getAvailableSize() const { assert( mFillSize >= mUseSize ); return mFillSize - mUseSize; }
	size_t  getFreeSize()      const { assert( mMaxSize >= mFillSize ) ; return mMaxSize - mFillSize; }

	void clear()
	{
		mFillSize = 0;
		mUseSize  = 0;
	}

	void fill( void const* ptr , size_t num )
	{
		if ( !checkFill( num ) )
			return;

		memcpy( mData + mFillSize , ptr , num );
		mFillSize += num;

	}

	void fillValue(int value , size_t num)
	{
		if( !checkFill(num) )
			return;

		memset(mData + mFillSize, value, num);
		mFillSize += num;
	}

	void take( void* ptr , size_t num )
	{
		if ( !checkTake( num ) )
			return;

		memcpy( ptr , mData + mUseSize , num );
		mUseSize += num;
	}


	template< class Q >
	void fill( TStreamBuffer< Q >& buffer , size_t num )
	{
		assert( num <= buffer.getAvailableSize() );
		fill(  (void*)( buffer.getData() + buffer.mUseSize ) , num  );
		buffer.mUseSize += num;
	}

	template < class T >
	void fill( T const& val )
	{ 
		if ( !checkFill( sizeof( T ) ) )
			return;

		typedef typename Meta::Select< 
			(sizeof(T) > 8) && Meta::IsPod< T >::Result ,
			MemcpyStrategy , AssignStrategy 
		>::ResultType Strategy;

		Strategy::fill( mData + mFillSize , val );
		mFillSize += sizeof( T );
	}

	template < class T >
	void take( T& val )
	{ 
		if ( !checkTake( sizeof( T ) ) )
			return;

		typedef typename Meta::Select< 
			( sizeof( T ) > 8 ) && Meta::IsPod< T >::Result , 
			MemcpyStrategy , AssignStrategy 
		>::ResultType Strategy;

		Strategy::take( mData + mUseSize , val );
		mUseSize += sizeof( T );
	}

	void swap( TStreamBuffer& buffer )
	{
		using std::swap;
		swap( mData    , buffer.mData );
		swap( mMaxSize , buffer.mMaxSize );
		swap( mUseSize , buffer.mUseSize );
		swap( mFillSize, buffer.mFillSize );
	}

	template< class T >
	struct TArrayData
	{
		T*     ptr;
		size_t length;
	};

	template< class T >
	static TArrayData< T > MakeArrayData(T* ptr, size_t length)
	{
		TArrayData< T > result;
		result.ptr = ptr;
		result.length = length;
		return result;
	}

	template< class T >
	void take(TArrayData< T >& data)
	{
		if ( data.length )
			takeArray(data , Meta::IsPod< T >() );
	}

	template< class T >
	void takeArray(TArrayData< T >& data, Meta::TrueType)
	{
		take((void*)data.ptr, sizeof(T) * data.length);
	}

	template< class T >
	void takeArray(TArrayData< T >& data, Meta::FalseType)
	{
		for( size_t i = 0; i < data.length; ++i )
		{
			take(*(data.ptr + i));
		}
	}

	template< class T >
	void fill(TArrayData< T > const& data)
	{
		if( data.length )
			fillArray(data , Meta::IsPod< T >() );
	}

	template< class T >
	void fillArray(TArrayData< T > const& data, Meta::TrueType )
	{
		fill((void*)data.ptr, sizeof(T) * data.length);
	}
	template< class T >
	void fillArray(TArrayData< T > const& data, Meta::FalseType )
	{
		for( size_t i = 0; i < data.length; ++i )
		{
			fill(*(data.ptr + i));
		}
	}

	template < class T >
	void fill(std::vector< T > const& data)
	{
		size_t num = data.size();
		fill(num);
		if( num )
		{
			fill(MakeArrayData(&data[0], num));
		}
	}

	template < class T >
	void take(std::vector< T >& data)
	{
		size_t num;
		take(num);
		if ( num )
		{
			data.resize(num);
			take(MakeArrayData(&data[0], num));
		}
	}

	template< class Q >
	void copy( TStreamBuffer< Q >& buffer , size_t num )
	{
		assert( num <= buffer.getAvailableSize() );
		fill(  (void*)( buffer.getData() + buffer.mUseSize ) , num );
	}

	void setUseSize( size_t size )
	{
		//assert( size >= 0 );
		assert( size <= mFillSize );
		mUseSize = size;
	}
	void setFillSize( size_t size )
	{
		assert( size >= mUseSize );
		assert( size <= mMaxSize );
		mFillSize = size;
	}

	void shiftUseSize( int num )
	{
		//assert( mUseSize + num >= 0 );
		assert( mUseSize + num <= mFillSize );
		mUseSize = (int)mUseSize + num;
	}
	void shiftFillSize( int num )
	{
		assert( mFillSize + num >= mUseSize );
		assert( mFillSize + num <  mMaxSize );
		mFillSize = (int)mFillSize + num;
	}

	void removeUseData()
	{
		if( mUseSize == 0 )
			return;

		size_t size = getAvailableSize();
		char* dst = mData;
		char* src = mData + mUseSize;
		::memmove(dst, src, size);
		mUseSize = 0;
		mFillSize = size;
	}

private:
	bool checkFill( size_t num )
	{
		return CP::checkFill( mData , mMaxSize , mFillSize  , num  );
	}
	bool checkTake( size_t num )
	{
		return CP::checkTake( mData , mFillSize  , mUseSize ,  num );
	}

	struct AssignStrategy
	{
		template < class T >
		static void fill( char* data , T  c ){  *((T*) data ) = c;  }
		template < class T >
		static void take( char* data , T& c ){  c = *((T*) data );  }
	};

	struct MemcpyStrategy
	{
		template < class T >
		static void fill( char* data , T const& c ){  memcpy( data , (char const*)&c , sizeof(c) );  }
		template < class T >
		static void take( char* data , T& c ){  memcpy( (char*) &c , data , sizeof(c) );  }
	};

protected:
	void  setEmpty()
	{
		mData = NULL;
		mMaxSize = 0;
		clear();
	}

	template< class Q >
	friend class TStreamBuffer;
	char*    mData;
	size_t   mMaxSize;
	size_t   mUseSize;
	size_t   mFillSize;

};



#endif // StreamBuffer_h__
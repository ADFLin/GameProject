#ifndef StreamBuffer_h__
#define StreamBuffer_h__

#include "MetaBase.hpp"

#include <exception>
#include <memory>
#include <cassert>

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

struct NoCheckPolicy 
{
	static bool checkFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		return true;
	}
	static bool checkTake( char* data , size_t max , size_t cur ,  size_t num )
	{
		return true;
	}
};

struct  FalseCheckPolicy
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
template< class CheckPolicy = NoCheckPolicy >
class StreamBuffer
{
	typedef CheckPolicy CP;
public:
	StreamBuffer(){  setEmpty();  }
	StreamBuffer( char* data , int maxSize )
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

	void take( void* ptr , size_t num )
	{
		if ( !checkTake( num ) )
			return;

		memcpy( ptr , mData + mUseSize , num );
		mUseSize += num;
	}


	template< class Q >
	void fill( StreamBuffer< Q >& buffer , size_t num )
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
			( sizeof( T ) > 8 ) , 
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
			( sizeof( T ) > 8 ) , 
			MemcpyStrategy , AssignStrategy 
		>::ResultType Strategy;

		Strategy::take( mData + mUseSize , val );
		mUseSize += sizeof( T );
	}

	void swap( StreamBuffer& buffer )
	{
		using std::swap;
		swap( mData    , buffer.mData );
		swap( mMaxSize , buffer.mMaxSize );
		swap( mUseSize , buffer.mUseSize );
		swap( mFillSize, buffer.mFillSize );
	}

	template< class Q >
	void copy( StreamBuffer< Q >& buffer , size_t num )
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
	friend class StreamBuffer;
	char*    mData;
	size_t   mMaxSize;
	size_t   mUseSize;
	size_t   mFillSize;

};



#endif // StreamBuffer_h__
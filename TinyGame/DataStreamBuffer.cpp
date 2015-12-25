#include "TinyGamePCH.h"
#include "DataStreamBuffer.h"

bool GrowThrowPolicy::checkFill( char*& data , size_t& max , size_t cur , size_t num  )
{
	if ( cur + num > max )
	{
		size_t nSize = std::max( 2 * max , max + num );
		char*  nData = new char[ nSize ];
		if ( data )
		{
			memcpy( nData , data , cur );
			delete[] data;
		}
		data =  nData;
		max  =  nSize;
	}
	return true;
}

bool GrowThrowPolicy::checkTake(char*& data , size_t& max , size_t cur , size_t num )
{
	if ( cur + num > max )
	{
		throw BufferException( "Overflow" );
	}
	return true;

}

DataStreamBuffer::DataStreamBuffer()
{

}

DataStreamBuffer::DataStreamBuffer( size_t size )
{
	mData = new char[ size ];
	mMaxSize = size;
	clear();
}

void DataStreamBuffer::resize( size_t size )
{
	cleanup();
	mData = new char[ size ];
	mMaxSize = size;
}

void DataStreamBuffer::cleanup()
{
	delete[] mData;
	mData = NULL;
	mMaxSize = 0;
	clear();
}

DataStreamBuffer::~DataStreamBuffer()
{
	delete [] mData;
}

void DataStreamBuffer::fill( char const* str )
{
	size_t len = strlen( str ) + 1;
	fill( (void*)str , len );
}

void DataStreamBuffer::fill( char const* str , size_t max )
{
	size_t len = strlen( str ) + 1;
	fill( (void*)str , std::min( len , max ) );
}

void DataStreamBuffer::take( char* str )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mUseSize , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	strcpy_s( str , len , mData + mUseSize );
	mUseSize += len;
}

void DataStreamBuffer::take( char* str , size_t max )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mUseSize , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );
	if ( len > max )
		throw BufferException( "Take error : str storage too small" );

	strcpy_s( str , len , mData + mUseSize );
	mUseSize += len;
}

void DataStreamBuffer::copy( DataStreamBuffer const& rhs )
{
	if ( mMaxSize < rhs.mFillSize )
	{
		delete [] mData;
		mData = new char [ rhs.mFillSize ];
		mMaxSize = rhs.mFillSize;
	}
	::memcpy( mData , rhs.mData , rhs.mFillSize );
	mFillSize = rhs.mFillSize;
	mUseSize  = rhs.mUseSize;
}

void DataStreamBuffer::move( DataStreamBuffer& other )
{
	mData     = other.mData;
	mFillSize = other.mFillSize;
	mMaxSize  = other.mMaxSize;
	mUseSize  = other.mUseSize;

	other.setEmpty();
}


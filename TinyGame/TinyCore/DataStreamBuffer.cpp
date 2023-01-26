#include "TinyGamePCH.h"
#include "DataStreamBuffer.h"

#include <algorithm>

bool GrowThrowPolicy::CheckFill( char*& data , size_t& max , size_t cur , size_t num  )
{
	if ( cur + num > max )
	{
		size_t nSize = std::max( 2 * max , max + num );
		char*  nData = new char[ nSize ];
		if ( data )
		{
			::memcpy( nData , data , cur );
			delete[] data;
		}
		data =  nData;
		max  =  nSize;
	}
	return true;
}

bool GrowThrowPolicy::CheckTake(char*& data , size_t& max , size_t cur , size_t num )
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
	if (mData)
	{
		delete[] mData;
		mData = nullptr;
	}
	mMaxSize = 0;
	clear();
}

DataStreamBuffer::~DataStreamBuffer()
{
	delete [] mData;
}

void DataStreamBuffer::fill( char const* str )
{
	size_t len = FCString::Strlen( str ) + 1;
	fill( (void*)str , len );
}

void DataStreamBuffer::fill( char const* str , size_t max )
{
	size_t len = strnlen( str , max );
	fill( (void*)str , len );
}

void DataStreamBuffer::take( char* str )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mSizeUsed , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	strcpy_s( str , len , mData + mSizeUsed );
	mSizeUsed += len;
}

void DataStreamBuffer::take( char* str , size_t max )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mSizeUsed , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );
	if ( len > max )
		throw BufferException( "Take error : str storage too small" );

	strcpy_s( str , len , mData + mSizeUsed );
	mSizeUsed += len;
}

void DataStreamBuffer::copy( DataStreamBuffer const& rhs )
{
	if ( mMaxSize < rhs.mSizeFilled )
	{
		delete [] mData;
		mData = new char [ rhs.mSizeFilled ];
		mMaxSize = rhs.mSizeFilled;
	}
	::memcpy( mData , rhs.mData , rhs.mSizeFilled );
	mSizeFilled = rhs.mSizeFilled;
	mSizeUsed  = rhs.mSizeUsed;
}

void DataStreamBuffer::moveData( DataStreamBuffer& other )
{
	mData     = other.mData;
	mSizeFilled = other.mSizeFilled;
	mMaxSize  = other.mMaxSize;
	mSizeUsed  = other.mSizeUsed;

	other.setEmpty();
}


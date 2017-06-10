#include "TinyGamePCH.h"
#include "DataSteamBuffer.h"

#include <algorithm>

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

DataSteamBuffer::DataSteamBuffer()
{

}

DataSteamBuffer::DataSteamBuffer( size_t size )
{
	mData = new char[ size ];
	mMaxSize = size;
	clear();
}

void DataSteamBuffer::resize( size_t size )
{
	cleanup();
	mData = new char[ size ];
	mMaxSize = size;
}

void DataSteamBuffer::cleanup()
{
	delete[] mData;
	mData = NULL;
	mMaxSize = 0;
	clear();
}

DataSteamBuffer::~DataSteamBuffer()
{
	delete [] mData;
}

void DataSteamBuffer::fill( char const* str )
{
	size_t len = strlen( str ) + 1;
	fill( (void*)str , len );
}

void DataSteamBuffer::fill( char const* str , size_t max )
{
	size_t len = strnlen( str , max );
	fill( (void*)str , len );
}

void DataSteamBuffer::take( char* str )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mUseSize , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	strcpy_s( str , len , mData + mUseSize );
	mUseSize += len;
}

void DataSteamBuffer::take( char* str , size_t max )
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

void DataSteamBuffer::copy( DataSteamBuffer const& rhs )
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

void DataSteamBuffer::move( DataSteamBuffer& other )
{
	mData     = other.mData;
	mFillSize = other.mFillSize;
	mMaxSize  = other.mMaxSize;
	mUseSize  = other.mUseSize;

	other.setEmpty();
}


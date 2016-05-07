#include "SocketBuffer.h"

#include "TSocket.h"
#include <cstring>


SBuffer::SBuffer( size_t maxSize ) 
{
	resize( maxSize );
}

SBuffer::SBuffer()
{

}

SBuffer::~SBuffer()
{
	delete [] mData;
}


void SBuffer::fill( char const* str )
{
	size_t len = strnlen( str , getFreeSize() ) + 1;

	if ( len > getFreeSize() )
		 throw BufferException( "Fill error : Free size too small" );

	strcpy_s( mData + mFillSize , getFreeSize() , str );
	mFillSize += len;
}

void SBuffer::fill( char const* str , size_t maxSize )
{
	size_t len = strnlen( str , maxSize ) + 1;

	if ( len > getFreeSize() )
		 throw BufferException( "Fill error : Free size too small" );

	strcpy_s( mData + mFillSize , getFreeSize() , str );
	mFillSize += len;
}

int SBuffer::fill( TSocket& socket , size_t len )
{
	if ( len > getFreeSize()  )
		throw BufferException( "Fill error : Free size too small" );

	int num = socket.recvData( mData + mFillSize , len );

	if ( num != SOCKET_ERROR )
	{
		mFillSize += num;
		return num;
	}
	return 0;
}

int SBuffer::fill( TSocket& socket , size_t len , NetAddress& addr )
{
	if  ( len > getFreeSize()  )
		throw BufferException( "Fill error : Free size too small" );

	int num =  socket.recvData( mData + mFillSize , len , addr );
	if ( num != SOCKET_ERROR )
	{
		mFillSize += num;
		return num;
	}
	return 0;
}

int SBuffer::fill( TSocket& socket , size_t len , sockaddr* addr , unsigned addrLen )
{
	if  ( len > getFreeSize() )
		throw BufferException( "Fill error : Free size too small" );

	int num = socket.recvData( mData + mFillSize , len , addr , addrLen );
	if ( num != SOCKET_ERROR )
	{
		mFillSize += num;
		return num;
	}
	return 0;

}

void SBuffer::append( SBuffer const& buffer )
{
	fill(  (void*)buffer.getData() , buffer.getFillSize()  );
}



void SBuffer::take( char* str )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mUseSize , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	strcpy_s( str , len , mData + mUseSize );
	mUseSize += len;
}

void SBuffer::take( char* str , size_t maxSize )
{
	size_t len = strnlen( mData + mUseSize , getAvailableSize() ) + 1;

	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	if ( len > maxSize )
		throw BufferException( "Take error : Storage don't have enough space" );

	strcpy_s( str , maxSize , mData + mUseSize );
	mUseSize += len;
}



int SBuffer::take( TSocket& socket )
{
	int numSend = socket.sendData( mData + mUseSize , mFillSize - mUseSize );
	
	if ( numSend == SOCKET_ERROR )
		return 0;

	mUseSize += numSend;
	return numSend;
}

int SBuffer::take( TSocket& socket , size_t num )
{
	int numSend = socket.sendData( mData + mUseSize , num );

	if ( numSend == SOCKET_ERROR )
		return 0;

	mUseSize += numSend;
	return numSend;
}


int SBuffer::take( TSocket& socket , size_t num , NetAddress& addr )
{
	if ( num < getAvailableSize() )
		return false;

	int numSend = socket.sendData( mData + mUseSize , num , addr );
	
	if ( numSend == SOCKET_ERROR )
		return 0;

	mUseSize += numSend;
	return numSend;
}

int SBuffer::take( TSocket& socket  , NetAddress& addr )
{
	int numSend = socket.sendData( mData + mUseSize , mFillSize - mUseSize , addr );
	
	if ( numSend == SOCKET_ERROR )
		return 0;

	mUseSize += numSend;
	return numSend;
}



void SBuffer::resize( size_t size )
{
	delete [] mData;

	mMaxSize = size;
	mData = new char[ size ];
	clear();
}

void SBuffer::grow( size_t newSize )
{
	if ( newSize <= mMaxSize )
		return;

	char* newData = new char[ newSize ];
	memcpy( newData , mData , mFillSize );
	delete [] mData;
	mData = newData;
	mMaxSize = newSize;
}

void SBuffer::clearUseData()
{
	if ( mUseSize == 0 )
		return;

	size_t size = getAvailableSize();

	char* dst = mData;
	char* src = mData + mUseSize;
	if ( size > mUseSize )
	{
		for( size_t i = 0 ; i < size ; ++i )
			(*dst++) = (*src++);
	}
	else
	{
		::memmove( dst , src , size );
	}
	mUseSize  = 0;
	mFillSize = size;
}


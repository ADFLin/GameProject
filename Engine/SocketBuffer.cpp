#include "SocketBuffer.h"

#include "NetSocket.h"
#include "LogSystem.h"
#include <cstring>
#include <algorithm>

SocketBuffer::SocketBuffer( size_t maxSize ) 
{
	resize( maxSize );
}

SocketBuffer::SocketBuffer()
{

}

SocketBuffer::~SocketBuffer()
{
	delete [] mData;
}


void SocketBuffer::fill( char const* str )
{
	size_t len = strnlen( str , getFreeSize() ) + 1;

	if ( len > getFreeSize() )
		 throw BufferException( "Fill error : Free size too small" );

	strcpy_s( mData + mSizeFilled , getFreeSize() , str );
	mSizeFilled += len;
}

void SocketBuffer::fill( char const* str , size_t maxSize )
{
	size_t len = strnlen( str , maxSize ) + 1;

	if ( len > getFreeSize() )
		 throw BufferException( "Fill error : Free size too small" );

	strcpy_s( mData + mSizeFilled , getFreeSize() , str );
	mSizeFilled += len;
}

int SocketBuffer::fill( NetSocket& socket , size_t len )
{
	if ( len > getFreeSize()  )
		throw BufferException( "Fill error : Free size too small" );

	int num = socket.recvData( mData + mSizeFilled , len );

	if ( num != SOCKET_ERROR )
	{
		mSizeFilled += num;
		return num;
	}
	return 0;
}

int SocketBuffer::fill( NetSocket& socket , size_t len , NetAddress& addr )
{
	if  ( len > getFreeSize()  )
		throw BufferException( "Fill error : Free size too small" );

	int num =  socket.recvData( mData + mSizeFilled , len , addr );
	if ( num != SOCKET_ERROR )
	{
		mSizeFilled += num;
		return num;
	}
	return 0;
}

int SocketBuffer::fill( NetSocket& socket , size_t len , sockaddr* addr , unsigned addrLen )
{
	if  ( len > getFreeSize() )
		throw BufferException( "Fill error : Free size too small" );

	int num = socket.recvData( mData + mSizeFilled , len , addr , addrLen );
	if ( num != SOCKET_ERROR )
	{
		mSizeFilled += num;
		return num;
	}
	return 0;

}

void SocketBuffer::append( SocketBuffer const& buffer )
{
	fill(  (void*)buffer.getData() , buffer.getFillSize()  );
}

void SocketBuffer::take( char* str )
{
	size_t avialable = getAvailableSize();

	size_t len = strnlen( mData + mSizeUsed , avialable ) + 1;
	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	strcpy_s( str , len , mData + mSizeUsed );
	mSizeUsed += len;
}

void SocketBuffer::take( char* str , size_t maxSize )
{
	size_t len = strnlen( mData + mSizeUsed , getAvailableSize() ) + 1;

	if ( len > getAvailableSize()  )
		throw BufferException( "Take error : error string format" );

	if ( len > maxSize )
		throw BufferException( "Take error : Storage don't have enough space" );

	strcpy_s( str , maxSize , mData + mSizeUsed );
	mSizeUsed += len;
}



int SocketBuffer::take( NetSocket& socket )
{
	int numSend = socket.sendData( mData + mSizeUsed , mSizeFilled - mSizeUsed );
	
	if ( numSend == SOCKET_ERROR )
		return 0;

	mSizeUsed += numSend;
	return numSend;
}

int SocketBuffer::take( NetSocket& socket , size_t num )
{
	int numSend = socket.sendData( mData + mSizeUsed , num );

	if ( numSend == SOCKET_ERROR )
		return 0;

	mSizeUsed += numSend;
	return numSend;
}


int SocketBuffer::take( NetSocket& socket , size_t num , NetAddress& addr )
{
	if ( num < getAvailableSize() )
		return false;

	int numSend = socket.sendData( mData + mSizeUsed , num , addr );
	
	if ( numSend == SOCKET_ERROR )
		return 0;

	mSizeUsed += numSend;
	return numSend;
}

int SocketBuffer::take( NetSocket& socket  , NetAddress& addr )
{
	size_t MAX_UDP_DATA_SIZE = 65507u;
	size_t sendSize = std::min(mSizeFilled - mSizeUsed, MAX_UDP_DATA_SIZE);

	int numSend = socket.sendData( mData + mSizeUsed , sendSize , addr );
	
	if( numSend == SOCKET_ERROR )
	{
		LogWarning(0, "Socket can't send UDP data");
		return 0;
	}

	mSizeUsed += numSend;
	return numSend;
}



void SocketBuffer::resize( size_t size )
{
	delete [] mData;

	mMaxSize = size;
	mData = new char[ size ];
	clear();
}

void SocketBuffer::grow( size_t newSize )
{
	if ( newSize <= mMaxSize )
		return;

	char* newData = new char[ newSize ];
	FMemory::Copy( newData , mData , mSizeFilled );
	delete [] mData;
	mData = newData;
	mMaxSize = newSize;
}


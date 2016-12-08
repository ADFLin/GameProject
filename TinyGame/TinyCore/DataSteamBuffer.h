#ifndef DataStreamBuffer_h__
#define DataStreamBuffer_h__

#include "StreamBuffer.h"
#include "DataStream.h"

#include "CppVersion.h"

struct  GrowThrowPolicy
{
	static bool checkFill( char*& data , size_t& max , size_t cur ,  size_t num );
	static bool checkTake( char*& data , size_t& max , size_t cur ,  size_t num );
};


class  DataSteamBuffer : public TStreamBuffer< GrowThrowPolicy >
{
public:
	DataSteamBuffer();
	DataSteamBuffer( size_t size );

	DataSteamBuffer( DataSteamBuffer const& rhs ){ move( const_cast< DataSteamBuffer& >( rhs ) ); }
	DataSteamBuffer& operator = ( DataSteamBuffer const& rhs ){ cleanup(); move( const_cast< DataSteamBuffer& >( rhs ) ); return *this;  }

#if CPP_RVALUE_REFENCE_SUPPORT
	DataSteamBuffer( DataSteamBuffer&& rhs ){ move( rhs ); }
#endif

	~DataSteamBuffer();

	void resize( size_t size );
	void cleanup();

	using TStreamBuffer< GrowThrowPolicy >::fill;
	using TStreamBuffer< GrowThrowPolicy >::take;

	void fill( char const* str );
	void fill( char const* str , size_t max );
	void take( char* str );
	void take( char* str , size_t max );
	void copy( DataSteamBuffer const& rhs );

private:
	void move( DataSteamBuffer& other );
};


template < class BufferType >
class TBufferDataStream : public DataStream
{
public:
	TBufferDataStream(BufferType& buffer)
		:mBuffer(buffer)
	{
	}

	virtual void read(void* ptr, size_t num) override
	{
		mBuffer.take(ptr, num);
	}

	virtual void write(void const* ptr, size_t num) override
	{
		mBuffer.fill(ptr, num);
	}
private:
	BufferType& mBuffer;
};

template< class BufferType >
auto MakeBufferDataSteam(BufferType& buffer)
{
	return TBufferDataStream< BufferType >(buffer);
}

#endif // DataStreamBuffer_h__

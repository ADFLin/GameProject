#ifndef DataStreamBuffer_h__
#define DataStreamBuffer_h__

#include "StreamBuffer.h"
#include "DataStream.h"

#include "CppVersion.h"

struct  GrowThrowPolicy
{
	static bool CheckFill( char*& data , size_t& max , size_t cur ,  size_t num );
	static bool CheckTake( char*& data , size_t& max , size_t cur ,  size_t num );
};


class  DataSteamBuffer : public TStreamBuffer< GrowThrowPolicy >
{
public:
	DataSteamBuffer();
	DataSteamBuffer( size_t size );

#if 0
	DataSteamBuffer( DataSteamBuffer const& rhs ){ moveData( const_cast< DataSteamBuffer& >( rhs ) ); }
	DataSteamBuffer& operator = ( DataSteamBuffer const& rhs ){ cleanup(); moveData( const_cast< DataSteamBuffer& >( rhs ) ); return *this;  }
#else
	DataSteamBuffer(DataSteamBuffer const& rhs) = delete;
	DataSteamBuffer& operator = (DataSteamBuffer const& rhs) = delete;
#endif

#if CPP_RVALUE_REFENCE_SUPPORT
	DataSteamBuffer(DataSteamBuffer&& rhs) { moveData(rhs); }
	DataSteamBuffer& operator = (DataSteamBuffer&& rhs) { cleanup(); moveData(rhs); return *this; }
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
	void moveData( DataSteamBuffer& other );
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

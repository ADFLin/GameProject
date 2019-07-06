#ifndef DataStreamBuffer_h__
#define DataStreamBuffer_h__

#include "Serialize/StreamBuffer.h"
#include "Serialize/DataStream.h"

#include "CppVersion.h"

struct  GrowThrowPolicy
{
	static bool CheckFill( char*& data , size_t& max , size_t cur ,  size_t num );
	static bool CheckTake( char*& data , size_t& max , size_t cur ,  size_t num );
};


class DataStreamBuffer : public TStreamBuffer< GrowThrowPolicy >
{
public:
	DataStreamBuffer();
	DataStreamBuffer( size_t size );

#if 0
	DataSteamBuffer( DataSteamBuffer const& rhs ){ moveData( const_cast< DataSteamBuffer& >( rhs ) ); }
	DataSteamBuffer& operator = ( DataSteamBuffer const& rhs ){ cleanup(); moveData( const_cast< DataSteamBuffer& >( rhs ) ); return *this;  }
#else
	DataStreamBuffer(DataStreamBuffer const& rhs) = delete;
	DataStreamBuffer& operator = (DataStreamBuffer const& rhs) = delete;
#endif

#if CPP_RVALUE_REFENCE_SUPPORT
	DataStreamBuffer(DataStreamBuffer&& rhs) { moveData(rhs); }
	DataStreamBuffer& operator = (DataStreamBuffer&& rhs) { cleanup(); moveData(rhs); return *this; }
#endif

	~DataStreamBuffer();

	void resize( size_t size );
	void cleanup();

	using TStreamBuffer< GrowThrowPolicy >::fill;
	using TStreamBuffer< GrowThrowPolicy >::take;

	void fill( char const* str );
	void fill( char const* str , size_t max );
	void take( char* str );
	void take( char* str , size_t max );
	void copy( DataStreamBuffer const& rhs );

private:
	void moveData( DataStreamBuffer& other );
};



#endif // DataStreamBuffer_h__

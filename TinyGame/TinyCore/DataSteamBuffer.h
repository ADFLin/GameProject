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



#endif // DataStreamBuffer_h__

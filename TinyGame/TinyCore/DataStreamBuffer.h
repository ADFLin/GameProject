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

class  DataStreamBuffer : public StreamBuffer< GrowThrowPolicy >
	                    , public DataStream
{
public:
	DataStreamBuffer();
	DataStreamBuffer( size_t size );

	DataStreamBuffer( DataStreamBuffer const& rhs ){ move( const_cast< DataStreamBuffer& >( rhs ) ); }
	DataStreamBuffer& operator = ( DataStreamBuffer const& rhs ){ cleanup(); move( const_cast< DataStreamBuffer& >( rhs ) ); return *this;  }

#if CPP_RVALUE_REFENCE_SUPPORT
	DataStreamBuffer( DataStreamBuffer&& rhs ){ move( rhs ); }
#endif

	~DataStreamBuffer();

	void resize( size_t size );
	void cleanup();

	using StreamBuffer< GrowThrowPolicy >::fill;
	using StreamBuffer< GrowThrowPolicy >::take;

	void fill( char const* str );
	void fill( char const* str , size_t max );
	void take( char* str );
	void take( char* str , size_t max );
	void copy( DataStreamBuffer const& rhs );

	virtual void read( void* ptr , size_t num )       { take( ptr , num ); }
	virtual void write( void const* ptr , size_t num ){ fill( ptr, num ); }
private:
	void move( DataStreamBuffer& other );
};


#endif // DataStreamBuffer_h__

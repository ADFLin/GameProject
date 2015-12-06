#ifndef SocketBuffer_h__
#define SocketBuffer_h__

#include "StreamBuffer.h"

#include <cassert>
#include <memory>


class  TSocket;
class  NetAddress;
struct sockaddr;



class SBuffer : public StreamBuffer< ThrowCheckPolicy >
{
	typedef StreamBuffer< ThrowCheckPolicy > BaseClass;
public:
	SBuffer();
	SBuffer( size_t maxSize );
	~SBuffer();

	void  resize( size_t size );
	void  grow( size_t newSize );

	using BaseClass::fill;
	using BaseClass::take;

	
	void fill( char const* str );
	void fill( char const* str , size_t maxSize );

	void clearUseData();


	void take( char* str );
	void take( char* str , size_t maxSize );

	int fill( TSocket& socket , size_t len  );
	int fill( TSocket& socket , size_t len , NetAddress& addr );
	int fill( TSocket& socket , size_t len , sockaddr* addr , unsigned addrLen );
	
	//Tcp
	int  take( TSocket& socket );
	int  take( TSocket& socket , size_t num );
	//Udp
	int  take( TSocket& socket , size_t num , NetAddress& addr );
	int  take( TSocket& socket , NetAddress& addr );

	void append( SBuffer const& buffer );

	struct GData
	{
		GData( void* ptr , size_t size ):ptr(ptr),size(size){}
		void*  ptr;
		size_t size;
	};

	void take( GData& data ){ take( data.ptr , data.size ); }
	void fill( GData const& data ){ fill( data.ptr , data.size ); }

	template< class T >
	SBuffer& operator >> ( T& val ){ take( val ); return *this;  }
	template< class T >
	SBuffer& operator << ( T const & val ){ fill( val ); return *this;  }

	struct Take
	{
		typedef Take ThisType;
		enum { IsTake = 1 , IsFill = 0 };
		Take( SBuffer& buffer ): buffer( buffer ){}
		template< class T >
		inline ThisType& operator & ( T&  val ){  buffer >> val; return *this;  }
		template< int N >
		inline ThisType& operator & ( char (&str)[N] ){  buffer.take( str , N ); return *this;  }
		inline ThisType& operator & ( SBuffer& buf )
		{
			unsigned size;
			buffer.take( size );
			if ( size > buf.getFreeSize() )
			{
				buf.grow( buf.getMaxSize() + ( size - buf.getFreeSize() ) );
			}
			buf.fill( buffer , size );
			return *this;
		}
		SBuffer& buffer;
	};


	struct Fill
	{
		typedef Fill ThisType;
		enum { IsTake = 0 , IsFill = 1 };
		Fill( SBuffer& buffer ): buffer( buffer ){}
		template< class T >
		inline ThisType& operator & ( T&  val ){  buffer << val; return *this;  }
		template< int N >
		inline ThisType& operator & ( char (&str)[N] ){  buffer.fill( str , N ); return *this;  }
		inline ThisType& operator & ( SBuffer& buf )
		{
			size_t size = buf.getAvailableSize();
			buffer.fill( size );
			buffer.fill( buf , size );
			return *this;
		}

		SBuffer& buffer;
	};
};




#endif // SocketBuffer_h__
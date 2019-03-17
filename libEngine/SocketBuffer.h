#ifndef SocketBuffer_h__
#define SocketBuffer_h__

#include "Serialize/StreamBuffer.h"

#include <cassert>
#include <memory>


class  NetSocket;
class  NetAddress;
struct sockaddr;

class SocketBuffer : public TStreamBuffer< ThrowCheckPolicy >
{
	typedef TStreamBuffer< ThrowCheckPolicy > BaseClass;
public:
	SocketBuffer();
	SocketBuffer( size_t maxSize );
	~SocketBuffer();

	void  resize( size_t size );
	void  grow( size_t newSize );

	using BaseClass::fill;
	using BaseClass::take;

	
	void fill( char const* str );
	void fill( char const* str , size_t maxSize );

	void take( char* str );
	void take( char* str , size_t maxSize );

	int fill( NetSocket& socket , size_t len  );
	int fill( NetSocket& socket , size_t len , NetAddress& addr );
	int fill( NetSocket& socket , size_t len , sockaddr* addr , unsigned addrLen );
	
	//Tcp
	int  take( NetSocket& socket );
	int  take( NetSocket& socket , size_t num );
	//Udp
	int  take( NetSocket& socket , size_t num , NetAddress& addr );
	int  take( NetSocket& socket , NetAddress& addr );

	void append( SocketBuffer const& buffer );

	struct RawData
	{
		RawData( void* ptr , size_t size ):ptr(ptr),size(size){}
		void*  ptr;
		size_t size;
	};

	void take( RawData& data ){ take( data.ptr , data.size ); }
	void fill( RawData const& data ){ fill( data.ptr , data.size ); }


	template< class T >
	SocketBuffer& operator >> ( T& val ){ take( val ); return *this;  }
	template< class T >
	SocketBuffer& operator << ( T const & val ){ fill( val ); return *this;  }

	struct Take
	{
		typedef Take ThisType;
		enum { IsTake = 1 , IsFill = 0 };
		Take( SocketBuffer& buffer ): buffer( buffer ){}
		template< class T >
		inline ThisType& operator & ( T&  val ){  buffer >> val; return *this;  }
		template< int N >
		inline ThisType& operator & ( char (&str)[N] ){  buffer.take( str , N ); return *this;  }
		inline ThisType& operator & ( SocketBuffer& buf )
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
		SocketBuffer& buffer;
	};


	struct Fill
	{
		typedef Fill ThisType;
		enum { IsTake = 0 , IsFill = 1 };
		Fill( SocketBuffer& buffer ): buffer( buffer ){}
		template< class T >
		inline ThisType& operator & ( T&  val ){  buffer << val; return *this;  }
		template< int N >
		inline ThisType& operator & ( char (&str)[N] ){  buffer.fill( str , N ); return *this;  }
		inline ThisType& operator & ( SocketBuffer& buf )
		{
			size_t size = buf.getAvailableSize();
			buffer.fill( size );
			buffer.fill( buf , size );
			return *this;
		}

		SocketBuffer& buffer;
	};
};




#endif // SocketBuffer_h__
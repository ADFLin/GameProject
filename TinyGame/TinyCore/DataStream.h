#ifndef DataStream_h__
#define DataStream_h__

#include <vector>

class DataStream
{
public:
	virtual void read( void* ptr , size_t num ) = 0;
	virtual void write( void const* ptr , size_t num ) = 0;
};

#include <fstream>

class InputFileStream : public DataStream
{
public:
	bool open( char const* path )
	{
		using namespace std;
		mFS.open( path , ios::binary );
		return mFS.is_open();
	}
	virtual void read( void* ptr , size_t num )
	{
		mFS.read( (char*)ptr , ( std::streamsize ) num );
	}
	virtual void write( void const* ptr , size_t num )
	{

	}
protected:
	std::ifstream mFS;
};

class OutputFileStream : public DataStream
{
public:
	bool open( char const* path )
	{
		using namespace std;
		mFS.open( path , ios::binary );
		return mFS.is_open();
	}
	virtual void read( void* ptr , size_t num )
	{
		
	}
	virtual void write( void const* ptr , size_t num )
	{
		mFS.write( (char const*)ptr , ( std::streamsize ) num );
	}
protected:
	std::ofstream mFS;
};


class FileStream : public DataStream
{
public:
	bool open( char const* path )
	{
		using namespace std;
		mFS.open( path , ios::binary | ios::out | ios::in );
		return mFS.is_open();
	}
	virtual void read( void* ptr , size_t num )
	{
		mFS.read( (char*)ptr , ( std::streamsize ) num );
	}
	virtual void write( void const* ptr , size_t num )
	{
		mFS.write( (char const*)ptr , ( std::streamsize ) num );
	}
protected:
	std::fstream mFS;
};


class DataSerializer
{
public:
	DataSerializer( DataStream& stream ):mStream( stream ){}

	struct PODTag {};
	struct ObjectTag {};

	template< class T >
	void write( T const& value )
	{
		mStream.write( &value , sizeof( value ) );
	}
	template< class T >
	void read( T& value )
	{
		mStream.read( &value , sizeof( value ) );
	}

	template < class T >
	void read( T* ptr , size_t num , PODTag )
	{
		mStream.read( ptr , num * sizeof( T ) );
	}

	template< class T >
	void write( T const* ptr , size_t num , PODTag )
	{
		mStream.write( ptr , num * sizeof( T ) );
	}

	template < class T >
	void read( T* ptr , size_t num , ObjectTag )
	{
		for( size_t i = 0 ; i < num ; ++i )
		{
			(*this) >> ptr[i];
		}
	}

	template< class T >
	void write( T const* ptr , size_t num , ObjectTag )
	{
		for( size_t i = 0 ; i < num ; ++i )
		{
			(*this) << ptr[i];
		}
	}

	template< class T >
	void write( std::vector< T > const& vec )
	{
		size_t size = vec.size();
		this->write( size );
		this->write( &vec[0] , size , ObjectTag() );
	}

	template< class T >
	void read( std::vector< T > const& vec )
	{
		size_t size;
		this->read( size );
		vec.resize( size );
		this->read( &vec[0] , size , ObjectTag() );
	}

	template< class T >
	DataSerializer& operator >> ( T& value )      { read( value ); return *this; }
	template< class T >
	DataSerializer& operator << ( T const& value ){ write( value ); return *this; }

	DataStream& mStream;

	class ReadOp
	{
	public:
		enum { isRead = 1 , isWrite = 0 };
		ReadOp( DataSerializer& s ):serializer(s){}
		typedef ReadOp ThisOp;
		template< class T >
		ThisOp& operator & ( T& value ){ serializer >> value; return *this;  }
		DataSerializer& serializer;
	};

	class WriteOp
	{
	public:
		enum { isRead = 0 , isWrite = 1 };
		typedef WriteOp ThisOp;
		WriteOp( DataSerializer& s ):serializer(s){}
		template< class T >
		ThisOp& operator & ( T const& value ){ serializer << value; return *this;  }
		DataSerializer& serializer;
	};
};


#endif // DataStream_h__

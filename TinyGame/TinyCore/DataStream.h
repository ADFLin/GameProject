#ifndef DataStream_h__
#define DataStream_h__

#include <vector>
#include "DataBitSerialize.h"
#include "MetaBase.h"

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
	bool open( char const* path );
	virtual void read( void* ptr , size_t num ) override;
	virtual void write( void const* ptr , size_t num ) override;
protected:
	std::ifstream mFS;
};

class OutputFileStream : public DataStream
{
public:
	bool open( char const* path );
	virtual void read( void* ptr , size_t num ) override;
	virtual void write( void const* ptr , size_t num ) override;
protected:
	std::ofstream mFS;
};

class FileStream : public DataStream
{
public:
	bool open( char const* path , bool bRemoveOldData = false );
	virtual void read( void* ptr , size_t num );
	virtual void write( void const* ptr , size_t num );

	size_t getSize();
protected:
	std::fstream mFS;
};

class DataSerializer;
SUPPORT_BINARY_OPERATOR(HaveSerializerOutput, operator<< , &, const&);
SUPPORT_BINARY_OPERATOR(HaveSerializerInput, operator>> , &, &);
SUPPORT_BINARY_OPERATOR(HaveBitDataOutput, operator<< , &, const &);
SUPPORT_BINARY_OPERATOR(HaveBitDataInput, operator>> , &, &);

class DataSerializer
{
public:
	DataSerializer( DataStream& stream ):mStream( stream ){}

	DataStream& getStream() { return mStream;  }

	struct StreamDataPolicy
	{
		typedef DataStream DataType;
		static void Fill(DataType& buffer, uint8 value)
		{
			buffer.write(&value, 1);
		}
		static void Take(DataType& buffer, uint8& value)
		{
			buffer.read(&value, 1);
		}
	};

	typedef TDataBitWriter< StreamDataPolicy > BitWriter;
	typedef TDataBitReader< StreamDataPolicy > BitReader;

	template< class T >
	struct TArrayBitData
	{
		T*     ptr;
		size_t num;
		size_t length;
		uint32 numBit;
	};

	template< class T >
	typename Meta::EnableIf< HaveBitDataOutput< BitWriter , T >::Value >::Type
	write(TArrayBitData<T> const& data)
	{
		if( data.length )
		{
			BitWriter writer(mStream);
			for( size_t i = 0; i < data.length; ++i )
			{
				writer << data.ptr[i];
			}
			writer.finalize();
		}
	}

	template< class T >
	typename Meta::EnableIf< !HaveBitDataOutput< BitWriter , T >::Value >::Type
    write(TArrayBitData<T> const& data )
	{
		if( data.length )
		{
			BitWriter writer(mStream);
			writer.fillArray(data.ptr, data.num, data.length, data.numBit);
			writer.finalize();
		}
	}

	template< class T>
	typename Meta::EnableIf< HaveBitDataInput< BitReader , T >::Value >::Type
	read(TArrayBitData<T> const& data)
	{
		if( data.length )
		{
			BitReader reader(mStream);
			for( size_t i = 0; i < data.length; ++i )
			{
				reader >> data.ptr[i];
			}
		}
	}

	template< class T>
	typename Meta::EnableIf< !HaveBitDataInput< BitReader, T >::Value >::Type
	read(TArrayBitData<T> const& data)
	{
		if( data.length )
		{
			BitReader reader(mStream);
			reader.takeArray(data.ptr, data.num, data.length, data.numBit);
		}
	}

	template< class T >
	static TArrayBitData<T> MakeArrayBit(T* data, size_t length , uint32 numBit )
	{
		return { data , sizeof(T) , length , numBit };
	}

	template< class T >
	static TArrayBitData<T> MakeArrayBit(std::vector< T >& data, uint32 numBit)
	{
		if( data.empty() )
		{
			return{ nullptr , sizeof(T) , 0 , numBit };
		}
		else
		{
			return{ &data[0] , sizeof(T) , data.size() , numBit };
		}
	}

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



	template< class T >
	struct CanUseInputSequence : Meta::HaveResult< 
		(!HaveSerializerInput<DataSerializer , T>::Value) && Meta::IsPod<T>::Value >
	{};

	template< class T >
	struct CanUseOutputSequence : Meta::HaveResult<
		(!HaveSerializerOutput<DataSerializer, T>::Value) && Meta::IsPod<T>::Value >
	{};

	template < class T >
	typename Meta::EnableIf< CanUseInputSequence<T>::Value >::Type
	read( T* ptr , size_t num  )
	{
		mStream.read( ptr , num * sizeof(T) );
	}

	template < class T >
	typename Meta::EnableIf< !CanUseInputSequence<T>::Value >::Type
	read(T* ptr, size_t num)
	{
		for( size_t i = 0; i < num; ++i )
		{
			(*this) >> ptr[i];
		}
	}

	template< class T >
	typename Meta::EnableIf< CanUseOutputSequence<T>::Value >::Type
	write(T const* ptr, size_t num)
	{
		mStream.write(ptr, num * sizeof(T));
	}

	template< class T >
	typename Meta::EnableIf< !CanUseOutputSequence<T>::Value >::Type
	write(T const* ptr, size_t num)
	{
		for( size_t i = 0; i < num; ++i )
		{
			(*this) << ptr[i];
		}
	}

	template< class T >
	void write( std::vector< T > const& vec )
	{
		size_t size = vec.size();
		this->write( size );
		if( size )
		{
			this->write(&vec[0], size);
		}
	}

	template< class T >
	void read( std::vector< T >& vec )
	{
		size_t size;
		this->read( size );
		if ( size )
		{
			vec.resize(size);
			this->read(&vec[0], size);
		}
	}

	template< class T >
	DataSerializer& operator >> (T& value)
	{
		this->read(value);
		return *this;
	}

	template< class T >
	DataSerializer& operator << (T const& value)
	{
		this->write(value);
		return *this;
	}

	DataStream& mStream;

	class ReadOp
	{
	public:
		enum { isRead = 1 , isWrite = 0 };
		ReadOp( DataSerializer& s ):serializer(s){}
		typedef ReadOp ThisOp;
		template< class T >
		ThisOp& operator & ( T& value ){ serializer >> value; return *this;  }
		template< class T >
		ThisOp& operator << (T& value) { return *this; }
		template< class T >
		ThisOp& operator >> (T& value) { serializer >> value; return *this; }
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
		template< class T >
		ThisOp& operator << (T const& value) { serializer << value; return *this; }
		template< class T >
		ThisOp& operator >> (T const& value) { return *this; }

		DataSerializer& serializer;
	};
};



#endif // DataStream_h__

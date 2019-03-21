#ifndef DataStream_h__
#define DataStream_h__

#include "SerializeFwd.h"
#include "DataBitSerialize.h"
#include "MetaBase.h"

#include <vector>
#include <map>
#include <string>


class IStreamSerializer
{
public:
	virtual ~IStreamSerializer() {}
	//virtual bool isValid() const = 0;
	virtual void read(void* ptr, size_t num) = 0;
	virtual void write(void const* ptr, size_t num) = 0;

	struct StreamDataPolicy
	{
		typedef IStreamSerializer DataType;
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
	typename Meta::EnableIf< HaveBitDataOutput< BitWriter, T >::Value >::Type
	write(TArrayBitData<T> const& data)
	{
		if( data.length )
		{
			BitWriter writer(*this);
			for( size_t i = 0; i < data.length; ++i )
			{
				writer << data.ptr[i];
			}
			writer.finalize();
		}
	}

	template< class T >
	typename Meta::EnableIf< !HaveBitDataOutput< BitWriter, T >::Value >::Type
	write(TArrayBitData<T> const& data)
	{
		if( data.length )
		{
			BitWriter writer(*this);
			writer.fillArray(data.ptr, data.num, data.length, data.numBit);
			writer.finalize();
		}
	}

	template< class T>
	typename Meta::EnableIf< HaveBitDataInput< BitReader, T >::Value >::Type
	read(TArrayBitData<T> const& data)
	{
		if( data.length )
		{
			BitReader reader(*this);
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
			BitReader reader(*this);
			reader.takeArray(data.ptr, data.num, data.length, data.numBit);
		}
	}

	template< class T >
	static TArrayBitData<T> MakeArrayBit(T* data, size_t length, uint32 numBit)
	{
		return{ data , sizeof(T) , length , numBit };
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
	template < class T >
	typename Meta::EnableIf< TTypeSupportSerializeOPFunc<T>::Value >::Type
	write(T const& value)
	{
		const_cast<T&>(value).serialize(WriteOp(*this));
	}

	template < class T >
	typename Meta::EnableIf< TTypeSupportSerializeOPFunc<T>::Value >::Type
	read(T& value)
	{
		value.serialize(ReadOp(*this));
	}


	template < class T >
	typename Meta::EnableIf< !TTypeSupportSerializeOPFunc<T>::Value >::Type
	write(T const& value)
	{
		static_assert( std::is_pod<T>::value || std::is_trivial<T>::value || TTypeSerializeAsRawData<T>::Value, "Pleasse overload serilize operator");
		write(&value, sizeof(value));
	}


	template < class T >
	typename Meta::EnableIf< !TTypeSupportSerializeOPFunc<T>::Value >::Type
	read(T& value)
	{
		static_assert( std::is_pod<T>::value || std::is_trivial<T>::value || TTypeSerializeAsRawData<T>::Value, "Pleasse overload serilize operator");
		read(&value, sizeof(value));
	}

	template< class T >
	struct CanUseInputSequence : Meta::HaveResult< Meta::IsPod<T>::Value &&
		 !( HaveSerializerInput<IStreamSerializer, T>::Value || TTypeSupportSerializeOPFunc<T>::Value) >
	{
	};

	template< class T >
	struct CanUseOutputSequence : Meta::HaveResult < Meta::IsPod<T>::Value &&
		! (HaveSerializerOutput<IStreamSerializer, T>::Value || TTypeSupportSerializeOPFunc<T>::Value) >
	{
	};

	template < class T >
	typename Meta::EnableIf< CanUseInputSequence<T>::Value >::Type
	readSequence(T* ptr, size_t num)
	{
		read((void*)ptr, num * sizeof(T));
	}

	template < class T >
	typename Meta::EnableIf< !CanUseInputSequence<T>::Value >::Type
	readSequence(T* ptr, size_t num)
	{
		for( size_t i = 0; i < num; ++i )
		{
			(*this) >> ptr[i];
		}
	}

	template< class T >
	typename Meta::EnableIf< CanUseOutputSequence<T>::Value >::Type
	writeSequence(T const* ptr, size_t num)
	{
		write((void const*)ptr, num * sizeof(T));
	}

	template< class T >
	typename Meta::EnableIf< !CanUseOutputSequence<T>::Value >::Type
	writeSequence(T const* ptr, size_t num)
	{
		for( size_t i = 0; i < num; ++i )
		{
			(*this) << ptr[i];
		}
	}

	template<  class T, class A >
	void write(std::vector< T, A > const& value)
	{
		size_t size = value.size();
		this->write(size);
		if( size )
		{
			this->writeSequence(value.data(), size);
		}
	}

	template< class T, class A >
	void read(std::vector< T, A >& value)
	{
		size_t size = 0;
		this->read(size);
		if( size )
		{
			value.resize(size);
			this->readSequence(value.data(), size);
		}
	}

	template< class T>
	void write(std::basic_string<T> const& value)
	{
		size_t size = value.size();
		this->write(size);
		if( size )
		{
			this->write(value.data(), size);
		}
	}

	template< class T>
	void read(std::basic_string<T>& value)
	{
		size_t size = 0;
		this->read(size);
		if( size )
		{
			value.resize(size);
			this->readSequence(&value[0], size);
		}
	}
	template< class K, class V, class KF, class A >
	void write(std::map< K, V, KF, A > const& mapValue)
	{
		size_t size = mapValue.size();
		this->write(size);
		if( size )
		{
			for( auto const& pair : mapValue )
			{
				(*this) << pair.first;
				(*this) << pair.second;
			}
		}
	}

	template< class K, class V, class KF, class A >
	void read(std::map< K, V, KF, A >& mapValue)
	{
		size_t size = 0;
		this->read(size);
		if( size )
		{
			for( size_t i = 0; i < size; ++i )
			{
				std::pair< K, V > value;
				(*this) >> value.first;
				(*this) >> value.second;
				mapValue[value.first] = value.second;
			}
		}
	}

	template< class T >
	IStreamSerializer& operator >> (T& value)
	{
		this->read(value);
		return *this;
	}

	template< class T >
	IStreamSerializer& operator << (T const& value)
	{
		this->write(value);
		return *this;
	}

	class ReadOp
	{
	public:
		enum { IsLoading = 1, IsSaving = 0 };
		ReadOp(IStreamSerializer& s) :serializer(s) {}
		typedef ReadOp ThisOp;
		template< class T >
		ThisOp& operator & (T& value) { serializer >> value; return *this; }
		template< class T >
		ThisOp& operator << (T& value) { return *this; }
		template< class T >
		ThisOp& operator >> (T& value) { serializer >> value; return *this; }

		IStreamSerializer& serializer;
	};

	class WriteOp
	{
	public:
		enum { IsLoading = 0, IsSaving = 1 };
		typedef WriteOp ThisOp;
		WriteOp(IStreamSerializer& s) :serializer(s) {}
		template< class T >
		ThisOp& operator & (T const& value) { serializer << value; return *this; }
		template< class T >
		ThisOp& operator << (T const& value) { serializer << value; return *this; }
		template< class T >
		ThisOp& operator >> (T const& value) { return *this; }

		IStreamSerializer& serializer;
	};
};




#endif // DataStream_h__

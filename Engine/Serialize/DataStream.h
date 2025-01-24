#ifndef DataStream_h__
#define DataStream_h__

#include "SerializeFwd.h"
#include "DataBitSerialize.h"
#include "Meta/MetaBase.h"
#include "Meta/EnableIf.h"
#include "Meta/Concept.h"

#include "HashString.h"
#include "DataStructure/Array.h"
#include "Template/ArrayView.h"

#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <string>

#if CPP_COMPILER_MSVC
struct CGlobalInputSteamable
{
	template< typename TStream, typename T >
	static auto Requires(void(*result)(TStream&, T&)) -> decltype
	(
		result = &operator >>
	);
};


struct CGlobalOutputSteamable
{
	template< typename TStream, typename T >
	static auto Requires(void(*result)(TStream&, T const&)) -> decltype
	(
		result = &operator <<
	);
};
#else
struct CGlobalInputSteamable
{
	template< typename TStream, typename T >
	static auto Requires(TStream& s, T const& t) -> decltype
	(
		s >> t
	);
};


struct CGlobalOutputSteamable
{
	template< typename TStream, typename T >
	static auto Requires(TStream& s, T const& t) -> decltype
	(
		s << t
	);
};

#endif

class IStreamSerializer
{
public:
	virtual ~IStreamSerializer() {}
	//virtual bool isValid() const = 0;
	virtual void read(void* ptr, size_t num) = 0;
	virtual void write(void const* ptr, size_t num) = 0;
	virtual void registerVersion(HashString name, int32 version ){}
	virtual void redirectVersion(HashString from, HashString to) {}
	virtual int32 getVersion(HashString name) { return 0; }


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
 	void write(TArrayBitData<T> const& data)
	{
		if constexpr ( TCheckConcept< CGlobalOutputSteamable, BitWriter , T >::Value )
		{
			if (data.length)
			{
				BitWriter writer(*this);
				for (size_t i = 0; i < data.length; ++i)
				{
					writer << data.ptr[i];
				}
				writer.finalize();
			}
		}
		else
		{
			if (data.length)
			{
				BitWriter writer(*this);
				writer.fillArray(data.ptr, data.num, data.length, data.numBit);
				writer.finalize();
			}
		}
	}

	template< class T >
	void read(TArrayBitData<T> const& data)
	{
		if constexpr ( TCheckConcept< CGlobalInputSteamable, BitWriter, T >::Value )
		{
			if (data.length)
			{
				BitReader reader(*this);
				for (size_t i = 0; i < data.length; ++i)
				{
					reader >> data.ptr[i];
				}
			}
		}
		else
		{
			if (data.length)
			{
				BitReader reader(*this);
				reader.takeArray(data.ptr, data.num, data.length, data.numBit);
			}
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

	struct CSerializeCallable
	{
		template< typename T, typename OP>
		static auto Requires(T& value, OP& op) -> decltype
		(
			value.serialize(op)
		);
	};


	struct CGlobalSerializeCallable
	{
		template< typename T, typename OP>
		static auto Requires(T& value, OP& op) -> decltype
		(
			StreamSerializer::Serialize(value, op)
		);
	};

	template< typename T , typename OP >
	struct TUseSerializeFunc
	{
		enum 
		{
			Value = TTypeSupportSerializeOPFunc<T>::Value || (TCheckConcept< CSerializeCallable, T, OP >::Value && !TTypeSupportSerializeOPFunc<T>::Defined),
		};
	};

	template< typename T, typename OP >
	struct TUseGlobalSerializeFunc
	{
		enum
		{
			Value = TTypeSupportSerializeOPFunc<T>::Value || (TCheckConcept< CGlobalSerializeCallable, T, OP >::Value && !TTypeSupportSerializeOPFunc<T>::Defined),
		};
	};

	template < class T >
	void write(T const& value)
	{
		if constexpr (TUseSerializeFunc<T, WriteOp>::Value)
		{
			const_cast<T&>(value).serialize(WriteOp(*this));
		}
		else if constexpr (TUseGlobalSerializeFunc<T, WriteOp>::Value)
		{
			using namespace StreamSerializer;
			Serialize(const_cast<T&>(value), WriteOp(*this));
		}
		else
		{
			static_assert(std::is_pod<T>::value || std::is_trivial<T>::value || TTypeSerializeAsRawData<T>::Value, "Pleasse overload serilize operator");
			write(&value, sizeof(value));
		}
	}

	template < class T >
	void read(T& value)
	{
		if constexpr (TUseSerializeFunc<T, ReadOp>::Value)
		{
			value.serialize(ReadOp(*this));
		}
		else if constexpr (TUseGlobalSerializeFunc<T, ReadOp>::Value)
		{
			using namespace StreamSerializer;
			Serialize(value, ReadOp(*this));
		}
		else
		{
			static_assert(std::is_pod<T>::value || std::is_trivial<T>::value || TTypeSerializeAsRawData<T>::Value, "Pleasse overload serilize operator");
			read(&value, sizeof(value));
		}	
	}

	template< typename T, typename OP >
	struct TUseSequenceAccess
	{
		enum
		{
			Value = Meta::IsPod<T>::Value && !(TCheckConcept< CGlobalOutputSteamable, IStreamSerializer, T >::Value || TUseSerializeFunc<T, OP>::Value),
		};
	};

	template < class T >
	void readSequence(T* ptr, size_t num)
	{
		if constexpr (TUseSequenceAccess<T, ReadOp>::Value)
		{
			read((void*)ptr, num * sizeof(T));
		}
		else
		{
			for (size_t i = 0; i < num; ++i)
			{
				(*this) >> ptr[i];
			}
		}
	}


	template < class T >
	void writeSequence(T const* ptr, size_t num)
	{
		if constexpr (TUseSequenceAccess<T, WriteOp>::Value)
		{
			write((void const*)ptr, num * sizeof(T));
		}
		else
		{
			for (size_t i = 0; i < num; ++i)
			{
				(*this) << ptr[i];
			}
		}
	}

	template< class T >
	struct TDataSequence
	{
		T const* data;
		int num;
	};
	template< class T >
	static TDataSequence<T> MakeSequence(T const* data, int num)
	{
		return{ data , num };
	}
	template< class T>
	void write(TDataSequence<T> const& dataSequence)
	{
		writeSequence(dataSequence.data, dataSequence.num);
	}

	template< class T>
	void read(TDataSequence<T>& dataSequence)
	{
		readSequence( const_cast< T* >( dataSequence.data ), dataSequence.num);
	}

	template< class T, int N >
	void write(T const(&value)[N])
	{
		writeSequence(&value[0], N);
	}

	template< class T, int N >
	void read(T(&value)[N])
	{
		readSequence(&value[0], N);
	}

	using SizeType = uint32;

	template<  class T, class A >
	void write(TArray< T, A > const& value)
	{
		SizeType size = (SizeType)value.size();
		this->write(size);
		if (size)
		{
			this->writeSequence(value.data(), size);
		}
	}

	template< class T >
	void write(TArrayView< T const > const& value)
	{
		SizeType size = (SizeType)value.size();
		this->write(size);
		if (size)
		{
			this->writeSequence(value.data(), size);
		}
	}

	template<  class T, class A >
	void write(std::vector< T, A > const& value)
	{
		SizeType size = (SizeType)value.size();
		this->write(size);
		if( size )
		{
			this->writeSequence(value.data(), size);
		}
	}

	template< class T, class A >
	void read(TArray< T , A >& value)
	{
		SizeType size = 0;
		this->read(size);
		if (size)
		{
			value.resize(size);
			this->readSequence(value.data(), size);
		}
	}

	template< class T, class A >
	void read(std::vector< T, A >& value)
	{
		SizeType size = 0;
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
		SizeType size = (SizeType)value.size();
		this->write(size);
		if( size )
		{
			this->writeSequence(value.data(), size);
		}
	}

	template< class T>
	void read(std::basic_string<T>& value)
	{
		SizeType size = 0;
		this->read(size);
		if( size )
		{
			value.resize(size);
			this->readSequence(&value[0], size);
		}
	}
	template< class K, class V, class KF, class A >
	void write(std::map< K, V, KF, A > const& mapValue) { writeMap(mapValue); }
	template< class K, class V, class H , class KF, class A >
	void write(std::unordered_map< K, V, H , KF, A > const& mapValue) { writeMap(mapValue); }

	template< class MapType >
	void writeMap(MapType const& mapValue)
	{
		SizeType size = (SizeType)mapValue.size();
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
	void read(std::map< K, V, KF, A >& mapValue) { readMap<K,V>(mapValue); }
	template< class K, class V, class H, class KF, class A >
	void read(std::unordered_map< K, V, H, KF, A >& mapValue) { readMap<K,V>(mapValue); }

	template<class K,class V, class MapType >
	void readMap(MapType& mapValue)
	{
		mapValue.clear();
		SizeType size = 0;
		this->read(size);
		if( size )
		{
			for( uint32 i = 0; i < size; ++i )
			{
				K k;
				V v;
				(*this) >> k;
				(*this) >> v;
				mapValue[k] = v;
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

	class OpBase
	{
	public:
		OpBase(IStreamSerializer& s) :serializer(s) {}

		int32 version(HashString name = EName::None) { return serializer.getVersion(name); }
		void  redirectVersion(HashString from, HashString to) { serializer.redirectVersion(from, to); }

		IStreamSerializer& serializer;
	};

	class ReadOp : public OpBase
	{
	public:
		enum { IsLoading = 1, IsSaving = 0 };
		typedef ReadOp ThisOp;

		ReadOp(IStreamSerializer& s) :OpBase(s) {}

		template< class T >
		ThisOp& operator & (T& value) { serializer >> value; return *this; }
		template< class T >
		ThisOp& operator << (T& value) { return *this; }
		template< class T >
		ThisOp& operator >> (T& value) { serializer >> value; return *this; }
	};

	class WriteOp : public OpBase
	{
	public:
		enum { IsLoading = 0, IsSaving = 1 };
		typedef WriteOp ThisOp;
		
		WriteOp(IStreamSerializer& s) :OpBase(s) {}

		template< class T >
		ThisOp& operator & (T const& value) { serializer << value; return *this; }
		template< class T >
		ThisOp& operator << (T const& value) { serializer << value; return *this; }
		template< class T >
		ThisOp& operator >> (T const& value) { return *this; }
	};
};




#endif // DataStream_h__

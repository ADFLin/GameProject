#ifndef StreamBuffer_h__
#define StreamBuffer_h__

#include "DataStream.h"

#include "CompilerConfig.h"
#include "Meta/MetaBase.h"
#include "Meta/Select.h"
#include "Template/ArrayView.h"
#include "Core/Memory.h"

#include <exception>
#include <cassert>
#include <vector>

class CheckPolicy
{
	static bool CheckFill( char* data , size_t max , size_t cur ,  size_t num );
	static bool CheckTake( char* data , size_t max , size_t cur ,  size_t num );
};

class BufferException : public std::exception
{
public:
	STD_EXCEPTION_CONSTRUCTOR_WITH_WHAT( BufferException )
};

struct AssertCheckPolicy 
{
	static bool CheckFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		assert( cur + num <= max );
		return true;
	}
	static bool CheckTake( char* data , size_t max , size_t cur ,  size_t num )
	{
		assert(cur + num <= max);
		return true;
	}
};

struct  FailCheckPolicy
{
	static bool CheckFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		if ( cur + num > max )
			return false;
		return true;
	}
	static bool CheckTake( char* data , size_t max , size_t cur ,  size_t num  )
	{
		if ( cur + num > max )
			return false;
		return true;
	}
};

struct  ThrowCheckPolicy
{
	static bool CheckFill( char* data , size_t max , size_t cur ,  size_t num )
	{
		if ( cur + num > max )
			throw BufferException( "Overflow" );
		return true;
	}
	static bool CheckTake( char* data , size_t max , size_t cur ,  size_t num )
	{
		if ( cur + num > max )
			throw BufferException( "No Enough Data" );
		return true;
	}
};


template< class CheckPolicy = AssertCheckPolicy >
class TStreamBuffer
{
	typedef CheckPolicy CP;
public:
	TStreamBuffer(){  setEmpty();  }
	TStreamBuffer( char* data , int maxSize )
	{
		mData = data;
		mMaxSize = maxSize;
		clear();
	}

	char*   getData()          const { return mData; }

	size_t  getFillSize()      const { return mSizeFilled; }
	size_t  getUseSize()       const { return mSizeUsed;  }
	size_t  getMaxSize()       const { return mMaxSize; }
	size_t  getAvailableSize() const { assert( mSizeFilled >= mSizeUsed ); return mSizeFilled - mSizeUsed; }
	size_t  getFreeSize()      const { assert( mMaxSize >= mSizeFilled ) ; return mMaxSize - mSizeFilled; }

	void clear()
	{
		mSizeFilled = 0;
		mSizeUsed  = 0;
	}

	void fill( void const* ptr , size_t num )
	{
		if ( !checkFill( num ) )
			return;

		FMemory::Copy( mData + mSizeFilled , ptr , num );
		mSizeFilled += num;

	}

	void fillValue(int value , size_t num)
	{
		if( !checkFill(num) )
			return;

		memset(mData + mSizeFilled, value, num);
		mSizeFilled += num;
	}

	void take( void* ptr , size_t num )
	{
		if ( !checkTake( num ) )
			return;

		FMemory::Copy( ptr , mData + mSizeUsed , num );
		mSizeUsed += num;
	}

	template< class ConvType , class T >
	void fillConv(T const& val)
	{
		ConvType temp = val;
		fill(temp);
	}

	template< class ConvType, class T >
	void takeConv(T& val)
	{
		ConvType temp;
		take(temp);
		val = temp;
	}

	template< class Q >
	void fill( TStreamBuffer< Q >& buffer , size_t num )
	{
		assert( num <= buffer.getAvailableSize() );
		fill(  (void*)( buffer.getData() + buffer.mSizeUsed ) , num  );
		buffer.mSizeUsed += num;
	}

	template < class T >
	void fill( T const& val )
	{ 
		if ( !checkFill( sizeof( T ) ) )
			return;

		typedef typename TSelect< 
			(sizeof(T) > 8) && ( Meta::IsPod< T >::Value || TTypeSerializeAsRawData<T>::Value ) ,
			MemcpyStrategy , AssignStrategy 
		>::Type Strategy;

		Strategy::Fill( mData + mSizeFilled , val );
		mSizeFilled += sizeof( T );
	}

	template < class T >
	void take( T& val )
	{ 
		if ( !checkTake( sizeof( T ) ) )
			return;

		typedef typename TSelect< 
			( sizeof( T ) > 8 ) && (Meta::IsPod< T >::Value || TTypeSerializeAsRawData<T>::Value),
			MemcpyStrategy , AssignStrategy 
		>::Type Strategy;

		Strategy::Take( mData + mSizeUsed , val );
		mSizeUsed += sizeof( T );
	}

	void swap( TStreamBuffer& buffer )
	{
		using std::swap;
		swap( mData    , buffer.mData );
		swap( mMaxSize , buffer.mMaxSize );
		swap( mSizeUsed , buffer.mSizeUsed );
		swap( mSizeFilled, buffer.mSizeFilled );
	}

	template< class T >
	struct TArrayData
	{
		T*     ptr;
		size_t length;
	};

	template< class T >
	static TArrayData< T > MakeArrayData(T* ptr, size_t length)
	{
		TArrayData< T > result;
		result.ptr = ptr;
		result.length = length;
		return result;
	}

	template< class T >
	void take(TArrayData< T >& data)
	{
		if ( data.length )
			takeArray(data , Meta::IsPod< T >() );
	}

	template< class T >
	void takeArray(TArrayData< T >& data, Meta::TureType)
	{
		take((void*)data.ptr, sizeof(T) * data.length);
	}

	template< class T >
	void takeArray(TArrayData< T >& data, Meta::FalseType)
	{
		for( size_t i = 0; i < data.length; ++i )
		{
			take(*(data.ptr + i));
		}
	}

	template< class T >
	void fill(TArrayData< T > const& data)
	{
		if( data.length )
			fillArray(data , Meta::IsPod< T >() );
	}

	template< class T >
	void fillArray(TArrayData< T > const& data, Meta::TureType )
	{
		fill((void*)data.ptr, sizeof(T) * data.length);
	}
	template< class T >
	void fillArray(TArrayData< T > const& data, Meta::FalseType )
	{
		for( size_t i = 0; i < data.length; ++i )
		{
			fill(*(data.ptr + i));
		}
	}

	template < class T >
	void fill(std::vector< T > const& data)
	{
		size_t num = data.size();
		fill(num);
		if( num )
		{
			fill(MakeArrayData(&data[0], num));
		}
	}

	template < class T >
	void take(std::vector< T >& data)
	{
		size_t num;
		take(num);
		if ( num )
		{
			data.resize(num);
			take(MakeArrayData(&data[0], num));
		}
	}

	template< class Q >
	void copy( TStreamBuffer< Q >& buffer , size_t num )
	{
		assert( num <= buffer.getAvailableSize() );
		fill(  (void*)( buffer.getData() + buffer.mSizeUsed ) , num );
	}

	void setUseSize( size_t size )
	{
		//assert( size >= 0 );
		assert( size <= mSizeFilled );
		mSizeUsed = size;
	}
	void setFillSize( size_t size )
	{
		assert( size >= mSizeUsed );
		assert( size <= mMaxSize );
		mSizeFilled = size;
	}

	void shiftUseSize( int num )
	{
		//assert( mSizeUsed + num >= 0 );
		assert( mSizeUsed + num <= mSizeFilled );
		mSizeUsed = (int)mSizeUsed + num;
	}
	void shiftFillSize( int num )
	{
		assert( mSizeFilled + num >= mSizeUsed );
		assert( mSizeFilled + num <  mMaxSize );
		mSizeFilled = (int)mSizeFilled + num;
	}

	void removeUsedData()
	{
		if( mSizeUsed == 0 )
			return;

		size_t size = getAvailableSize();
		char* dst = mData;
		char* src = mData + mSizeUsed;
		FMemory::Move(dst, src, size);
		mSizeUsed = 0;
		mSizeFilled = size;
	}

private:
	bool checkFill( size_t num )
	{
		return CP::CheckFill( mData , mMaxSize , mSizeFilled  , num  );
	}
	bool checkTake( size_t num )
	{
		return CP::CheckTake( mData , mSizeFilled  , mSizeUsed ,  num );
	}

	struct AssignStrategy
	{
		template < class T >
		static void Fill( char* data , T  c ){  *((T*) data ) = c;  }
		template < class T >
		static void Take( char* data , T& c ){  c = *((T*) data );  }
	};

	struct MemcpyStrategy
	{
		template < class T >
		static void Fill( char* data , T const& c ){ FMemory::Copy( data , (char const*)&c , sizeof(c) );  }
		template < class T >
		static void Take( char* data , T& c ){ FMemory::Copy( (char*) &c , data , sizeof(c) );  }
	};

protected:
	void  setEmpty()
	{
		mData = NULL;
		mMaxSize = 0;
		clear();
	}

	template< class Q >
	friend class TStreamBuffer;
	char*    mData;
	size_t   mMaxSize;
	size_t   mSizeUsed;
	size_t   mSizeFilled;

};

template < class BufferType , bool bRef >
class TBufferSerializer : public IStreamSerializer
{
public:
	TBufferSerializer(BufferType& buffer)
		:mBuffer(std::forward<BufferType>(buffer))
	{
	}

	template< class ...Args >
	TBufferSerializer(Args&& ...args)
		: mBuffer(std::forward<Args>(args)...)
	{
	}
	//virtual bool isValid() const override { return true; }
	virtual void read(void* ptr, size_t num) override
	{
		mBuffer.take(ptr, num);
	}

	virtual void write(void const* ptr, size_t num) override
	{
		mBuffer.fill(ptr, num);
	}
	using IStreamSerializer::read;
	using IStreamSerializer::write;
private:
	typedef typename TSelect< bRef, BufferType&, BufferType >::Type HoldType;
	HoldType mBuffer;

	virtual int32 getVersion(HashString name) override
	{
		return MaxInt32;
	}

};

template< class BufferType >
auto CreateSerializer(BufferType& buffer)
{
	return TBufferSerializer< BufferType , true >(buffer);
}

template< class BufferType , class ...Args >
auto CreateBufferSerializer(Args&& ...args)
{
	return TBufferSerializer< BufferType , false >(std::forward<Args>(args)...);
}

class VectorWriteBuffer
{
public:
	VectorWriteBuffer(std::vector<uint8>& inData)
		:mData(inData)
	{
	}

	void take(void* ptr, size_t num)
	{

	}

	void fill(void const* ptr, size_t num)
	{
		mData.insert(mData.end(), static_cast<uint8 const*>(ptr), static_cast<uint8 const*>(ptr) + num);
	}

	std::vector<uint8>& mData;
};


class SimpleReadBuffer
{
public:
	SimpleReadBuffer(TArrayView< uint8 const > inData)
		:mData(inData)
	{
	}

	void take(void* ptr, size_t num)
	{
		assert(mReadPos + num <= mData.size());
		uint8 const* cur = mData.data() + mReadPos;
		std::copy(cur, cur + num, (uint8*)ptr);
		mReadPos += num;
	}

	void fill(void const* ptr, size_t num)
	{

	}

	size_t mReadPos = 0;
	TArrayView< uint8 const > mData;
};


#endif // StreamBuffer_h__
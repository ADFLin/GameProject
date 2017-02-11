#pragma once

#ifndef DataBitSerialize_h__
#define DataBitSerialize_h__

#include "IntegerType.h"
#include "MetaBase.h"

#include <cassert>

#if 0
struct DataPolicy
{
	class DataType;
	static void fill(DataType& buffer, uint8 value);
	static void take(DataType& buffer, uint8& value);
};
#endif

#define BIT_MASK( numBit ) ( BIT(numBit) - 1 )

template< class DataPolicy >
class TDataBitWriter;

template< class DataPolicy >
class TDataBitWriter
{
public:
	typedef typename DataPolicy::DataType DataType;
	TDataBitWriter(DataType& buffer)
		:mBuffer(buffer)
	{
		curFill = 0;
		curFillLen = 0;
	}

	~TDataBitWriter()
	{

	}
	static uint8 const ByteLength = 8;

	template < class T >
	void fill(T const value[], size_t num, uint32 numBit)
	{
		//#TODO: Optimize ME
		for( size_t i = 0; i < num; ++i )
		{
			fill(value[i], numBit);
		}
	}

	template < class T >
	void fill(T value, uint32 numBit)
	{
		assert(numBit <= sizeof(value) * ByteLength);

		while( 1 )
		{
			if( curFillLen + numBit <= ByteLength )
			{
				curFill |= value << curFillLen;
				curFillLen += numBit;
				break;
			}
			else
			{
				uint8 fillLen = ByteLength - curFillLen;
				DataPolicy::fill(mBuffer, curFill | ((BIT_MASK(fillLen) & value) << curFillLen));
				numBit -= fillLen;
				value >>= fillLen;
				curFill = 0;
				curFillLen = 0;
			}
		}

		if( curFillLen == ByteLength )
		{
			DataPolicy::fill(mBuffer, curFill);
			curFill = 0;
			curFillLen = 0;
		}
	}

	void fill(uint8 value, uint32 numBit)
	{
		assert(numBit <= sizeof(value) * ByteLength);

		if( curFillLen + numBit <= ByteLength )
		{
			curFill |= value << curFillLen;
			curFillLen += numBit;
		}
		else
		{
			uint8 fillLen = ByteLength - curFillLen;
			DataPolicy::fill(mBuffer, curFill | ((BIT_MASK(fillLen) & value) << curFillLen));
			numBit -= fillLen;
			value >>= fillLen;
			curFill = value;
			curFillLen = numBit;
		}

		if( curFillLen == ByteLength )
		{
			DataPolicy::fill(mBuffer, curFill);
			curFill = 0;
			curFillLen = 0;
		}
	}

	void fillByteInternal(uint8 value)
	{
		assert(curFillLen != 0);
		uint8 fillLen = ByteLength - curFillLen;
		DataPolicy::fill(mBuffer, curFill | (( value & BIT_MASK(fillLen)) << curFillLen ) );
		curFill = value >> fillLen;
	}
	
	void fillArray(void* ptr, size_t valueSize, size_t length, uint32 numBit)
	{
		assert(numBit <= valueSize * ByteLength);

		uint8* data = (uint8*)ptr;

		size_t numByteNeed = numBit / ByteLength;
		uint32 remainBit = numBit % ByteLength;
		for( size_t i = 0; i < length; ++i )
		{
			fillInternal(data, numByteNeed, remainBit);
			data += valueSize;
		}
	}
	void fillInternal( void* ptr, size_t numByteNeed, uint32 remainBit)
	{
		if ( numByteNeed )
		{
			if( curFillLen == 0 )
			{
				for( size_t i = 0; i < numByteNeed; ++i )
				{
					DataPolicy::fill(mBuffer, *((uint8*)(ptr)+i));
				}
			}
			else
			{
				for( size_t i = 0; i < numByteNeed; ++i )
				{
					fillByteInternal(*((uint8*)(ptr)+i));
				}
			}
		}

		if( remainBit )
		{
			fill(*((uint8*)(ptr)+numByteNeed), remainBit);
		}
	}

	void finalize()
	{
		if( curFillLen != 0 )
		{
			DataPolicy::fill( mBuffer , curFill);
			curFillLen = 0;
		}
	}

	uint8  curFill;
	uint8  curFillLen;
	DataType& mBuffer;
};

template< class DataPolicy >
class TDataBitReader
{
public:
	typedef typename DataPolicy::DataType DataType;

	TDataBitReader(DataType& buffer)
		:mBuffer(buffer)
	{
		curTake = 0;
		numCanUse = 0;
	}

	static uint8 const ByteLength = 8;

	uint8  takeBit()
	{
		if( numCanUse == 0 )
		{
			mBuffer.take(curTake);
			numCanUse = sizeof(uint8) * ByteLength;
		}
		--numCanUse;
		uint8 result = (curTake & 0x1);
		curTake >>= 1;
		return result;
	}

	void take(uint32 value[], size_t num, uint32 numBit)
	{
		//#TODO: Optimize ME
		for( size_t i = 0; i < num; ++i )
		{
			take(value[i] , numBit);
		}
	}

	void takeArray(void* ptr, size_t valueSize, size_t length, uint32 numBit)
	{
		assert(numBit <= valueSize * ByteLength);

		uint8* data = (uint8*)ptr;

		size_t numByteNeed = numBit / ByteLength;
		uint32 remainBit = numBit % ByteLength;
		for( size_t i = 0; i < length; ++i )
		{
			takeInternal(data, numByteNeed, remainBit);
			data += valueSize;
		}
	}

	void takeInternal(void* ptr, size_t numByteNeed , uint32 remainBit)
	{
		if ( numByteNeed )
		{
			if( numCanUse == 0 )
			{
				for( size_t i = 0; i < numByteNeed; ++i )
				{
					DataPolicy::take(mBuffer, *((uint8*)(ptr)+i));
				}
			}
			else
			{
				for( size_t i = 0; i < numByteNeed; ++i )
				{
					*((uint8*)(ptr)+i) = takeByteInternal();
				}
			}
		}

		if(  remainBit )
		{
			take(*((uint8*)(ptr)+numByteNeed) , remainBit );
		}
	}

	uint32 takeByteInternal()
	{
		assert(numCanUse != ByteLength);
		uint8 result = curTake;
		DataPolicy::take(mBuffer, curTake);
		uint8 numFill = ByteLength - numCanUse;
		result |= ((curTake & BIT_MASK(numFill)) << numCanUse);
		curTake >>= numFill;
		return result;
	}

	template< class T >
	void take(T& value , uint32 numBit)
	{
		assert(numBit <= sizeof(T) * ByteLength);
		
		if( numCanUse == 0 )
		{
			DataPolicy::take(mBuffer, curTake);
			numCanUse = ByteLength;
		}

		value = 0;
		uint32 numFill = 0;
		while( 1 )
		{
			if( numBit > numCanUse )
			{
				value |= ( curTake << numFill );
				numFill += numCanUse;
				numBit  -= numCanUse;
				DataPolicy::take(mBuffer , curTake);
				numCanUse = ByteLength;
			}
			else
			{
				value |= ( BIT_MASK(numBit) & curTake) << numFill;
				curTake >>= numBit;
				numCanUse -= numBit;
				break;
			}
		}

		return value;
	}


	void take(uint8& value , uint32 numBit)
	{
		assert(numBit <= sizeof(uint8) * ByteLength);

		if( numCanUse == 0 )
		{
			DataPolicy::take(mBuffer, curTake);
			numCanUse = ByteLength;
		}

		value = 0;

		if( numBit > numCanUse )
		{
			value |= curTake;
			numBit -= numCanUse;
			DataPolicy::take(mBuffer, curTake);
			value |= ( BIT_MASK(numBit) & curTake) << numCanUse;
			curTake >>= numBit;
			numCanUse = ByteLength - numBit;
		}
		else
		{
			value |= BIT_MASK(numBit) & curTake;
			curTake >>= numBit;
			numCanUse -= numBit;
		}
	}
	uint8  curTake;
	uint8  numCanUse;
	DataType& mBuffer;
};

#undef BIT_MASK

#endif // DataBitSerialize_h__
